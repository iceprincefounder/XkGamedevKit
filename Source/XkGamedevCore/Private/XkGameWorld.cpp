#include "XkGameWorld.h"
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
	bShowSpawnedActorBaseMesh = true;
	bShowSpawnedActorEdgeMesh = true;
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

	ModifyHexagonalWorldNodes().Empty();

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

			FXkHexagonNode HexagonNode = FXkHexagonNode(EXkHexagonType::DeepWater | EXkHexagonType::Unavailable, Position, 0, HexagonCoord);
			ModifyHexagonalWorldNodes().Add(HexagonCoord, HexagonNode);
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
}


void AXkSphericalWorldWithOceanActor::GenerateHexagonalWorld()
{
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


	if (!GetWorld())
	{
		return;
	}

	uint32 CurrentGeneratingStep = 0;

	TArray<FIntVector> AllNodes;
	ModifyHexagonalWorldNodes().GenerateKeyArray(AllNodes);

	TArray<FIntVector> AllLandNodes;

	TArray<FIntVector> AllTempNodes;

	FVector2D RandomOffset = FVector2D(FMath::RandRange(PositionRandom.X, PositionRandom.Y), FMath::RandRange(PositionRandom.X, PositionRandom.Y));
	for (const FIntVector& NodeCoord : AllNodes)
	{
		FXkHexagonNode* Node = GetHexagonNode(NodeCoord);
		if (Node)
		{
			FVector Location = Node->GetLocation();
			if (FXkHexagonAStarPathfinding::CalcManhattanDistance(FIntVector(0, 0, 0), NodeCoord) < (int32)(CenterFieldRange))
			{
				AllLandNodes.AddUnique(NodeCoord);
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
				AllLandNodes.AddUnique(NodeCoord);
			}
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
					FXkHexagonNode* Node = GetHexagonNode(NeighborNodeCoord);
					if (Node)
					{
						AllTempNodes.AddUnique(NeighborNodeCoord);
					}
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

	// final deal with nodes
	for (const FIntVector& NodeCoord : AllLandNodes)
	{
		FXkHexagonNode* Node = GetHexagonNode(NodeCoord);
		if (Node)
		{
			Node->Type = EXkHexagonType::Land;
			float RandomSeed = FVector2D(Node->Position.X, Node->Position.Y).Length();
			for (const FXkHexagonSplat& HexagonSplat : HexagonSplats)
			{
				if (Node->Type == HexagonSplat.TargetType)
				{
					Node->Position.Z = HexagonSplat.Height;
					Node->Splatmap = HexagonSplat.Splats[RandRangeIntMT(RandomSeed, 0, HexagonSplat.Splats.Num() - 1)];
				}
			}
		}
	}
}


void AXkSphericalWorldWithOceanActor::GenerateGameWorld()
{
	TArray<FXkHexagonNode*> HexagonalWorldNodes = GetHexagonalWorldNodes(EXkHexagonType::Land);
	InstancedHexagonComponent->ClearInstances();
	InstancedHexagonComponent->NumCustomDataFloats = 4;
	TArray<FTransform> Transforms;
	TArray<float>& CustomData = InstancedHexagonComponent->PerInstanceSMCustomData;
	CustomData.Init(0.0, InstancedHexagonComponent->NumCustomDataFloats * HexagonalWorldNodes.Num());
	Transforms.Init(FTransform(), InstancedHexagonComponent->NumCustomDataFloats * HexagonalWorldNodes.Num());
	for (int32 i = 0; i < HexagonalWorldNodes.Num(); i++)
	{
		FXkHexagonNode* Node = HexagonalWorldNodes[i];
		FVector4f Position = Node->Position;
		FVector Location = FVector(Position.X, Position.Y, Position.Z);
		Transforms[i] = FTransform(Location);
		CustomData[i * InstancedHexagonComponent->NumCustomDataFloats + 0] = Node->CustomData[0];
		CustomData[i * InstancedHexagonComponent->NumCustomDataFloats + 1] = Node->CustomData[1];
		CustomData[i * InstancedHexagonComponent->NumCustomDataFloats + 2] = Node->CustomData[2];
		CustomData[i * InstancedHexagonComponent->NumCustomDataFloats + 3] = Node->CustomData[3];
	}
	InstancedHexagonComponent->AddInstances(Transforms, false, false);
}


void AXkSphericalWorldWithOceanActor::GenerateCanvas()
{
	// get vertex
	TArray<FVector4f> OutVertices; TArray<uint32> OutIndices;
	BuildHexagonData(OutVertices, OutIndices);

	// get instance
	TArray<FXkHexagonNode> HexagonalWorldNodes;
	ModifyHexagonalWorldNodes().GenerateValueArray(HexagonalWorldNodes);
	int NunInstances = HexagonalWorldNodes.Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceWeightData;
	for (int i = 0; i < NunInstances; i++)
	{
		const FXkHexagonNode& Node = HexagonalWorldNodes[i];
		FVector4f InstancePositionValue = Node.Position;
		FVector4f InstanceWeightValue = FVector4f(FVector3f(1.0), (float)Node.Splatmap / 255.0f);

		InstancePositionData.Add(InstancePositionValue);
		InstanceWeightData.Add(InstanceWeightValue);
	}

	FVector2D Resolution = CanvasRendererComponent->GetCanvasSize();
	FVector2D HexagonalWorldExtent = GetHexagonalWorldExtent();
	FVector2D UnscaledPatchCoverage = FVector2D(FMath::Max(HexagonalWorldExtent.X, HexagonalWorldExtent.Y) * 2.0);
	FVector2D FullUnscaledWorldSize = GetFullUnscaledWorldSize(UnscaledPatchCoverage, Resolution);
	FVector4f CanvasExtent = CanvasRendererComponent->GetCanvasExtent();
	CanvasExtent.X = FullUnscaledWorldSize.X;
	CanvasExtent.Y = FullUnscaledWorldSize.Y;
	CanvasRendererComponent->SetCanvasExtent(CanvasExtent);
	CanvasRendererComponent->CreateBuffers(OutVertices, OutIndices, InstancePositionData, InstanceWeightData);
	CanvasRendererComponent->DrawCanvas();
}


void AXkSphericalWorldWithOceanActor::RegenerateAll()
{
	GenerateHexagons();
	GenerateHexagonalWorld();
	GenerateGameWorld();
	GenerateCanvas();
	RegenerateHexagonalWorldContext();
}