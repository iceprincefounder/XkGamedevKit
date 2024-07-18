#include "XkGameWorld.h"
#include "XkGameWorld.h"
// Copyright ©XUKAI. All Rights Reserved.

#include "XkGameWorld.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkLandscape/XkLandscapeComponents.h"
#include "XkRenderer/XkRendererComponents.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/CollisionProfile.h"
#include "ProceduralMeshComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "LandscapeStreamingProxy.h"
#include "Components/ArrowComponent.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"
// STL
#include <random>

#include UE_INLINE_GENERATED_CPP_BY_NAME(XkGameWorld)

AXkSphericalWorldWithOceanActor::AXkSphericalWorldWithOceanActor()
{
	SphericalLandscapeComponent = CreateDefaultSubobject<UXkSphericalLandscapeWithWaterComponent>(TEXT("SphericalLandscape"));
	SphericalLandscapeComponent->Material = CastChecked<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_SphericalWorld")));
	SphericalLandscapeComponent->WaterMaterial = CastChecked<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_SphericalWorld")));
	SphericalLandscapeComponent->SetupAttachment(RootComponent);

	CanvasRendererComponent = CreateDefaultSubobject<UXkCanvasRendererComponent>(TEXT("CanvasRenderer"));

	HexagonMPC = CastChecked<UMaterialParameterCollection>(
		StaticLoadObject(UMaterialParameterCollection::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/MPC_Hexagon")));

	bSpawnActors = false;
	SpawnActorsMaxMhtDist = 10;
	XAxisCount = 64;
	YAxisCount = 64;
	bShowSpawnedActorBaseMesh = false;
	bShowSpawnedActorEdgeMesh = false;
	BaseColor = FLinearColor::White;
	EdgeColor = FLinearColor::White;
	EdgeColor.A = 0.25f;
	GeneratingMaxStep = 9999;
	CenterFieldRange = 20;
	PositionRandom = FVector2D(0.0, 0.0);
	PositionScale = 0.0002;
	FalloffCenter = FVector2D::ZeroVector;
	FalloffExtent = FVector2D(3000.0);
	FalloffCornerRadii = 1500.0;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


void AXkSphericalWorldWithOceanActor::PostLoad()
{
	Super::PostLoad();
}

void AXkSphericalWorldWithOceanActor::BeginPlay()
{
	UpdateHexagonalWorld();
	Super::BeginPlay();
}


void AXkSphericalWorldWithOceanActor::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}


void AXkSphericalWorldWithOceanActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	// update all HexagonActor settings for hexagonal world
	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* HexagonActor = *It;
		if (IsValid(HexagonActor))
		{
			HexagonActor->SetHexagonWorld(this);
			HexagonActor->ConstructionScripts();
		}
	}
#endif
}


void AXkSphericalWorldWithOceanActor::UpdateHexagonalWorld()
{
	GenerateCanvas();
}


TArray<AXkHexagonActor*> AXkSphericalWorldWithOceanActor::PathfindingToTargetAlways(const AXkHexagonActor* StartActor, const AXkHexagonActor* TargetActor, const TArray<FIntVector>& BlockList)
{
	TArray<AXkHexagonActor*> Results;

	const AXkHexagonActor* TargetHexagon = TargetActor;
	if (BlockList.Contains(TargetActor->GetCoord()))
	{
		if (!IsHexagonActorsNeighboring(StartActor, TargetActor))
		{
			TargetHexagon = GetHexagonActorNearestNeighbor(StartActor, TargetActor, BlockList);
		}
		else
		{
			Results.Add(const_cast<AXkHexagonActor*>(StartActor));
			return Results;
		}
	}
	if (IsValid(TargetHexagon))
	{
		Results = PathfindingHexagonActors(StartActor, TargetHexagon, BlockList);
	}
	ensure(Results.Num() < 255);
	return Results;
}


void AXkSphericalWorldWithOceanActor::GenerateHexagons()
{
	if (!ensure(GetWorld()))
	{
		return;
	}

	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* HexagonActor = *It;
		HexagonActor->Destroy();
	}

	HexagonalWorldTable.Nodes.Empty();

	for (int32 X = -XAxisCount; X < (XAxisCount + 1); X++)
	{
		for (int32 Y = -YAxisCount; Y < (YAxisCount + 1); Y++)
		{
			float Dist = Radius + GapWidth;
			FVector2D Pos = FXkHexagonAStarPathfinding::CalcHexagonPosition(X, Y, Dist);
			//////////////////////////////////////////////////////////////
			// calculate XkHexagon coordinate
			FIntVector HexagonCoord = FXkHexagonAStarPathfinding::CalcHexagonCoord(Pos.X, Pos.Y, Dist);
			int32 ManhattanDistanceToCenter = FXkHexagonAStarPathfinding::CalcManhattanDistance(HexagonCoord, FIntVector(0, 0, 0));
			if (ManhattanDistanceToCenter > MaxManhattanDistance)
			{
				continue;
			}
			FVector Location = FVector(Pos.X, Pos.Y, 0.0);
			FVector4f Position = FVector4f(Pos.X, Pos.Y, 0.0, 1.0);

			FXkHexagonNode HexagonNode = FXkHexagonNode(EXkHexagonType::DeepWater | EXkHexagonType::Unavailable, Position, BaseColor, EdgeColor, 0, HexagonCoord);
			HexagonalWorldTable.Nodes.Add(HexagonCoord, HexagonNode);
			if (bSpawnActors && ManhattanDistanceToCenter < SpawnActorsMaxMhtDist)
			{
				FActorSpawnParameters ActorSpawnParameters;
				AXkHexagonActor* HexagonActor = GetWorld()->SpawnActor<AXkHexagonActor>(AXkHexagonActor::StaticClass(), Location, FRotator(0.0), ActorSpawnParameters);
				HexagonActor->SetFlags(RF_Transient);
				HexagonActor->SetCoord(HexagonCoord);
				HexagonActor->SetHexagonWorld(this);
#if WITH_EDITOR
				FString CoordString = FString::Printf(
					TEXT("HexagonActor(%i, %i, %i)"), HexagonCoord.X, HexagonCoord.Y, HexagonCoord.Z);
				HexagonActor->SetActorLabel(CoordString);
#endif
				HexagonActor->ConstructionScripts();
				HexagonActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}
	}
	HexagonAStarPathfinding.Init(&HexagonalWorldTable);
}


void AXkSphericalWorldWithOceanActor::GenerateWorld()
{
	TMap<FIntVector, FXkHexagonNode>& HexagonalWorldNodes = HexagonalWorldTable.Nodes;

	auto RectangleFalloff = [](FVector2D InPosition, FVector2D InCenter, FVector2D InExtent, float InRadius, float InCornerRadii) -> float
		{
			FVector2D SafeExtent = InExtent - FVector2D(InCornerRadii);
			FVector2D FalloffCenter = FVector2D(InExtent.X - InCornerRadii, InExtent.Y - InCornerRadii);

			if (FMath::Abs(InPosition.X) < InExtent.X && FMath::Abs(InPosition.Y) < InExtent.Y)
			{
				if (FMath::Abs(InPosition.X) < SafeExtent.X || FMath::Abs(InPosition.Y) < SafeExtent.Y)
				{
					return 1.0;
				}
				float Y = FMath::Sqrt(FMath::Square(InCornerRadii) - FMath::Square(FMath::Abs(InPosition.X) - SafeExtent.X)) + SafeExtent.Y;
				if (FMath::Abs(InPosition.Y) <= Y)
				{
					return 1.0;
				}
			}
			return 0.0;
		};

	auto RandRangeMT = [](float seed, int min, int max) -> float
		{
			std::mt19937 gen(seed); // Initialize Mersenne Twister algorithm generator with seed value
			std::uniform_int_distribution<> dis(min, max); // Define a uniform distribution from min to max
			return dis(gen); // Generate random number
		};

	auto NotInsideNumber = [](const TArray<FIntVector>& AllNodes, const TArray<FIntVector>& CheckNodes) -> int32
		{
			int32 Result = 0;
			for (const FIntVector& CheckNode : CheckNodes)
			{
				if (!AllNodes.Contains(CheckNode))
				{
					Result++;
				}
			}
			return Result;
		};

	auto FindBoundary = [](const TArray<FIntVector>& AllNodes) -> TArray<FIntVector>
		{
			TArray<FIntVector> Results;
			for (const FIntVector& NodeCoord : AllNodes)
			{
				TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!AllNodes.Contains(NeighborNodeCoord))
					{
						Results.AddUnique(NodeCoord);
					}
				}
			}
			return Results;
		};

	auto FindBoundaryExpanded = [](const TArray<FIntVector>& AllNodes) -> TArray<FIntVector>
		{
			TArray<FIntVector> Results;
			for (const FIntVector& NodeCoord : AllNodes)
			{
				TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!AllNodes.Contains(NeighborNodeCoord))
					{
						Results.AddUnique(NeighborNodeCoord);
					}
				}
			}
			return Results;
		};

	auto FindBoundaryExpandedWithNoise = [HexagonalWorldNodes, RandRangeMT](const TArray<FIntVector>& AllNodes) -> TArray<FIntVector>
		{
			TArray<FIntVector> Results;
			for (const FIntVector& NodeCoord : AllNodes)
			{
				TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!AllNodes.Contains(NeighborNodeCoord) && HexagonalWorldNodes.Contains(NeighborNodeCoord))
					{
						const FXkHexagonNode Node = HexagonalWorldNodes[NeighborNodeCoord];
						if (RandRangeMT(FVector2D(Node.Position.X, Node.Position.Y).Length(), 0, 1))
						{
							Results.AddUnique(NeighborNodeCoord);
						}
					}
				}
			}
			return Results;
		};

	if (!GetWorld())
	{
		return;
	}

	uint32 CurrentGeneratingStep = 0;

	TArray<FIntVector> AllNodes;
	HexagonalWorldNodes.GenerateKeyArray(AllNodes);

	TArray<FIntVector> AllLandNodes;
	TArray<FIntVector> AllGrassNodes;
	TArray<FIntVector> AllForestNodes;
	TArray<FIntVector> AllBeachNodes;
	TArray<FIntVector> AllBeachNearLandNodes; // beach near land node has same height as land node
	TArray<FIntVector> AllShallowWaterNodes;
	TArray<FIntVector> AllDeepWaterNodes;
	TArray<FIntVector> AllUnavailableNodes;

	TArray<FIntVector> AllTempNodes;

	FVector2D RandomOffset = FVector2D(FMath::RandRange(PositionRandom.X, PositionRandom.Y), FMath::RandRange(PositionRandom.X, PositionRandom.Y));
	for (const FIntVector& NodeCoord : AllNodes)
	{
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		for (const FXkHexagonSplat& HexagonSplat : HexagonSplats)
		{
			if (Node.Type == HexagonSplat.TargetType)
			{
				Node.Position.Z = HexagonSplat.Height;
				Node.BaseColor = HexagonSplat.Color;
				Node.Splatmap = HexagonSplat.Splats[RandRangeMT(FVector2D(Node.Position.X, Node.Position.Y).Length(), 0, HexagonSplat.Splats.Num() - 1)];
			}
		}
		FVector4f Position = Node.Position;

		FVector Location = FVector(Position.X, Position.Y, Position.Z);

		if (FXkHexagonAStarPathfinding::CalcManhattanDistance(FIntVector(0, 0, 0), NodeCoord) < (int32)(CenterFieldRange))
		{
			AllLandNodes.Add(NodeCoord);
			continue;
		}
		FVector2D RandomLocation = (FVector2D(Location.X, Location.Y) + RandomOffset) * PositionScale;
		// noise range of - 1.0 to 1.0
		float Noise = FMath::PerlinNoise2D(RandomLocation);
		FVector2D FalloffLocation = FVector2D(Location.X, Location.Y);
		Noise = FMath::CeilToFloat(Noise);
		float Falloff = RectangleFalloff(FalloffLocation, FalloffCenter, FalloffExtent, FalloffRadius, FalloffCornerRadii);

		if (Noise * Falloff)
		{
			AllLandNodes.Add(NodeCoord);
		}
	}

	// 1. fill thin land
	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		for (FIntVector NodeCoord : AllLandNodes)
		{
			TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
			if (NotInsideNumber(AllLandNodes, NeighborNodeCoords) > 3)
			{
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!HexagonalWorldNodes.Contains(NeighborNodeCoord))
					{
						continue;
					}
					FXkHexagonNode& Node = HexagonalWorldNodes[NeighborNodeCoord];
					AllTempNodes.AddUnique(NeighborNodeCoord);
				}
			}
			else
			{
				AllTempNodes.AddUnique(NodeCoord);
			}
		}
		AllLandNodes = AllTempNodes;
		AllTempNodes.Empty();
		CurrentGeneratingStep++;
	}


	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		// 2. noise the border
		for (FIntVector NodeCoord : AllLandNodes)
		{
			AllTempNodes.AddUnique(NodeCoord);
			TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
			if (NotInsideNumber(AllLandNodes, NeighborNodeCoords) > 0)
			{
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!HexagonalWorldNodes.Contains(NeighborNodeCoord))
					{
						continue;
					}
					FXkHexagonNode& Node = HexagonalWorldNodes[NeighborNodeCoord];
					FVector4f Position = Node.Position;
					FVector2D RandomLocation = (FVector2D(Position.X, Position.Y) + RandomOffset);
					float Noise = FMath::PerlinNoise2D(RandomLocation);
					Noise = FMath::CeilToFloat(Noise);
					if (Noise)
					{
						AllTempNodes.AddUnique(NeighborNodeCoord);
					}
				}
			}
		}
		AllLandNodes = AllTempNodes;
		AllTempNodes.Empty();
		CurrentGeneratingStep++;
	}


	// 3. find the beach and shallow water
	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		AllBeachNearLandNodes = FindBoundaryExpanded(AllLandNodes);

		TArray<FIntVector> AllBeachTempNodes = AllBeachNearLandNodes;
		AllBeachTempNodes.Append(FindBoundaryExpanded(AllBeachTempNodes));
		AllBeachTempNodes.Append(FindBoundaryExpanded(AllBeachTempNodes));
		for (const FIntVector& NodeCoord : AllBeachTempNodes)
		{
			if (!AllLandNodes.Contains(NodeCoord))
			{
				AllBeachNodes.AddUnique(NodeCoord);
			}
		}
		TArray<FIntVector> AllShallowWaterTempNodes = AllBeachNodes;
		AllShallowWaterTempNodes.Append(FindBoundaryExpanded(AllShallowWaterTempNodes));
		AllShallowWaterTempNodes.Append(FindBoundaryExpandedWithNoise(AllShallowWaterTempNodes));
		AllShallowWaterTempNodes.Append(FindBoundaryExpandedWithNoise(AllShallowWaterTempNodes));
		for (const FIntVector& NodeCoord : AllShallowWaterTempNodes)
		{
			if (!AllLandNodes.Contains(NodeCoord) && !AllBeachNodes.Contains(NodeCoord))
			{
				AllShallowWaterNodes.AddUnique(NodeCoord);
			}
		}
		CurrentGeneratingStep++;
	}

	// final deal with nodes
	float LandHeight = 0.0;
	for (const FIntVector& NodeCoord : AllLandNodes)
	{
		if (!HexagonalWorldNodes.Contains(NodeCoord))
		{
			continue;
		}
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Type = EXkHexagonType::Land;
		float RandomSeed = FVector2D(Node.Position.X, Node.Position.Y).Length();
		for (const FXkHexagonSplat& HexagonSplat : HexagonSplats)
		{
			if (Node.Type == HexagonSplat.TargetType)
			{
				Node.Position.Z = HexagonSplat.Height;
				Node.BaseColor = HexagonSplat.Color;
				Node.Splatmap = HexagonSplat.Splats[RandRangeMT(RandomSeed, 0, HexagonSplat.Splats.Num() - 1)];
				LandHeight = HexagonSplat.Height;
			}
		}
	}
	for (const FIntVector& NodeCoord : AllBeachNodes)
	{
		if (!HexagonalWorldNodes.Contains(NodeCoord))
		{
			continue;
		}
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Type = EXkHexagonType::Sand | EXkHexagonType::Unavailable;
		float RandomSeed = FVector2D(Node.Position.X, Node.Position.Y).Length();
		for (const FXkHexagonSplat& HexagonSplat : HexagonSplats)
		{
			if (Node.Type == HexagonSplat.TargetType)
			{
				Node.Position.Z = HexagonSplat.Height;
				if (AllBeachNearLandNodes.Contains(NodeCoord))
				{
					Node.Position.Z = LandHeight;
				}
				Node.BaseColor = HexagonSplat.Color;
				Node.Splatmap = HexagonSplat.Splats[RandRangeMT(RandomSeed, 0, HexagonSplat.Splats.Num() - 1)];
			}
		}
	}

	for (const FIntVector& NodeCoord : AllShallowWaterNodes)
	{
		if (!HexagonalWorldNodes.Contains(NodeCoord))
		{
			continue;
		}
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Type = EXkHexagonType::ShallowWater | EXkHexagonType::Unavailable;
		float RandomSeed = FVector2D(Node.Position.X, Node.Position.Y).Length();
		for (const FXkHexagonSplat& HexagonSplat : HexagonSplats)
		{
			if (Node.Type == HexagonSplat.TargetType)
			{
				Node.Position.Z = HexagonSplat.Height;
				Node.BaseColor = HexagonSplat.Color;
				Node.Splatmap = HexagonSplat.Splats[RandRangeMT(RandomSeed, 0, HexagonSplat.Splats.Num() - 1)];
			}
		}
	}
}


void AXkSphericalWorldWithOceanActor::GenerateInstancedHexagons()
{
	TArray<FXkHexagonNode> HexagonalWorldNodes;
	GetHexagonalWorldNodes().GenerateValueArray(HexagonalWorldNodes);
	InstancedHexagonComponent->ClearInstances();
	for (int32 i = 0; i < HexagonalWorldNodes.Num(); i++)
	{
		const FXkHexagonNode& Node = HexagonalWorldNodes[i];
		FVector4f Position = Node.Position;
		FVector Location = FVector(Position.X, Position.Y, Position.Z);
		if (Node.Type == EXkHexagonType::Land)
		{
			InstancedHexagonComponent->AddInstance(FTransform(Location), false);
		}
	}

	// update hexagon actors
	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* HexagonActor = *It;
		if (HexagonalWorldTable.Nodes.Contains(HexagonActor->GetCoord()))
		{
			const FXkHexagonNode& HexagonNode = HexagonalWorldTable.Nodes[HexagonActor->GetCoord()];
			FVector4f Position = HexagonNode.Position;
			FVector Location = FVector(Position.X, Position.Y, Position.Z + 1.0 /*Fix Z-Fight*/);
			HexagonActor->SetActorLocation(Location);
		}		
	}
}


void AXkSphericalWorldWithOceanActor::GenerateCanvas()
{
	// get vertex
	TArray<FVector4f> OutVertices; TArray<uint32> OutIndices;
	FetchHexagonData(OutVertices, OutIndices);
	VertexBuffer.Positions = OutVertices;
	TArray<FVector2f> UVs; UVs.Init(FVector2f(), OutVertices.Num());
	VertexBuffer.UVs = UVs;
	IndexBuffer.Indices = OutIndices;

	// get instance
	TArray<FXkHexagonNode> HexagonalWorldNodes;
	GetHexagonalWorldNodes().GenerateValueArray(HexagonalWorldNodes);
	int NunInstances = HexagonalWorldNodes.Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceWeightData;
	for (int i = 0; i < NunInstances; i++)
	{
		const FXkHexagonNode& Node = HexagonalWorldNodes[i];
		FVector4f InstancePositionValue = Node.Position;
		FVector4f InstanceWeightValue = FVector4f(FVector3f(Node.BaseColor.R, Node.BaseColor.G, Node.BaseColor.B), (float)Node.Splatmap / 255.0f);

		InstancePositionData.Add(InstancePositionValue);
		InstanceWeightData.Add(InstanceWeightValue);
	}
	InstancePositionBuffer.Data = InstancePositionData;
	InstanceWeightBuffer.Data = InstanceWeightData;

	FVector2D Resolution = CanvasRendererComponent->GetCanvasSize();
	FVector2D HexagonalWorldExtent = GetHexagonalWorldExtent();
	FVector2D UnscaledPatchCoverage = FVector2D(FMath::Max(HexagonalWorldExtent.X, HexagonalWorldExtent.Y) * 2.0);
	FVector2D FullUnscaledWorldSize = GetFullUnscaledWorldSize(UnscaledPatchCoverage, Resolution);
	FVector4f CanvasExtent = CanvasRendererComponent->GetCanvasExtent();
	CanvasExtent.X = FullUnscaledWorldSize.X;
	CanvasExtent.Y = FullUnscaledWorldSize.Y;
	CanvasRendererComponent->SetCanvasExtent(CanvasExtent);
	CanvasRendererComponent->CreateBuffers(&VertexBuffer, &IndexBuffer, &InstancePositionBuffer, &InstanceWeightBuffer);
	CanvasRendererComponent->DrawCanvas(&VertexBuffer, &IndexBuffer, &InstancePositionBuffer, &InstanceWeightBuffer);
}


void AXkSphericalWorldWithOceanActor::RegenerateAll()
{
	GenerateHexagons();
	GenerateWorld();
	GenerateInstancedHexagons();
	GenerateCanvas();
}