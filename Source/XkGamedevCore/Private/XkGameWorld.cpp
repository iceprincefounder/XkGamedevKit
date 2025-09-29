// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkGameWorld.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkLandscape/XkLandscapeComponents.h"
#include "XkRenderer/XkRendererComponents.h"
#include "EngineUtils.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Materials/MaterialParameterCollection.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(XkGameWorld)

AXkSphericalWorldWithOceanActor::AXkSphericalWorldWithOceanActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SphericalLandscapeComponent = CreateDefaultSubobject<UXkSphericalLandscapeWithWaterComponent>(TEXT("SphericalLandscape"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder(TEXT("/XkGamedevKit/Materials/M_SphericalWorld"));
	SphericalLandscapeComponent->Material = ObjectFinder.Object;
	SphericalLandscapeComponent->WaterMaterial = ObjectFinder.Object;
	SphericalLandscapeComponent->bNeverDistanceCull = true;
	SphericalLandscapeComponent->LDMaxDrawDistance = 0.0f;
	SphericalLandscapeComponent->bUseAsOccluder = true;
	SphericalLandscapeComponent->SetupAttachment(RootComponent);

	CanvasRendererComponent = CreateDefaultSubobject<UXkCanvasRendererComponent>(TEXT("CanvasRenderer"));

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> ObjectFinder2(TEXT("/XkGamedevKit/Materials/MPC_HexagonalWorld"));
	HexagonMPC = ObjectFinder2.Object;

	GroundManhattanDistance = 28;
	ShorelineManhattanDistance = 4;
	ShorelineEachStepHeight = 25.0f;
	bSpawnActors = false;
	SpawnActorsMaxMhtDist = 10;
	bShowSpawnedActorBaseMesh = true;
	bShowSpawnedActorEdgeMesh = true;
	PositionRandomRange = FVector2D(0.0, 0.0);

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
	CanvasRendererComponent->HorizonHeight = HorizonHeight;
}


void AXkSphericalWorldWithOceanActor::GenerateHexagons()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AXkSphericalWorldWithOceanActor::GenerateHexagons);
	
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

	for (int32 X = -MaxManhattanDistance; X < (MaxManhattanDistance + 1); X++)
	{
		for (int32 Y = -MaxManhattanDistance; Y < (MaxManhattanDistance + 1); Y++)
		{
			float Dist = Radius + GapWidth;
			FVector2D Pos = FXkHexagonAStarPathfinding::CalcHexagonPosition(X, Y, Dist);
			//////////////////////////////////////////////////////////////
			// calculate XkHexagon coordinate
			FIntVector HexagonCoord = FXkHexagonAStarPathfinding::CalcHexagonCoord(Pos.X, Pos.Y, Dist);
			int32 ManhattanDistanceToCenter = FXkHexagonAStarPathfinding::CalcManhattanDistance(HexagonCoord, FIntVector(0, 0, 0));
			FVector4f Position = FVector4f(Pos.X, Pos.Y, 0.0, Radius);

			FXkHexagonNode HexagonNode = FXkHexagonNode(EXkHexagonType::Unavailable | EXkHexagonType::Ocean, Position, FVector4f::Zero(), HexagonCoord);
			if (ManhattanDistanceToCenter < (GroundManhattanDistance + ShorelineManhattanDistance))
			{
				ModifyHexagonalWorldNodes().Add(HexagonCoord, HexagonNode);
				if (bSpawnActors && ManhattanDistanceToCenter < SpawnActorsMaxMhtDist)
				{
					FActorSpawnParameters ActorSpawnParameters;
					FVector Location = FVector(Position.X, Position.Y, Position.Z + 200.0);
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
}


void AXkSphericalWorldWithOceanActor::GenerateHexagonalWorld()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AXkSphericalWorldWithOceanActor::GenerateHexagonalWorld);

	if (!GetWorld())
	{
		return;
	}

	TArray<FIntVector> AllNodes;
	ModifyHexagonalWorldNodes().GenerateKeyArray(AllNodes);

	// final deal with nodes splat
	for (const FIntVector& NodeCoord : AllNodes)
	{
		int32 ManhattanDistanceToCenter = FXkHexagonAStarPathfinding::CalcManhattanDistance(NodeCoord, FIntVector(0, 0, 0));
		FXkHexagonNode* Node = GetHexagonNode(NodeCoord);
		if (Node && ManhattanDistanceToCenter < GroundManhattanDistance)
		{
			Node->Type = EXkHexagonType::Ground;
			Node->Position.Z = HorizonHeight;
			Node->Weights = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
		}
		else if (Node && ManhattanDistanceToCenter < (GroundManhattanDistance + ShorelineManhattanDistance))
		{
			Node->Type = EXkHexagonType::Beach;
			int32 ShorelineStep = ManhattanDistanceToCenter - GroundManhattanDistance;
			Node->Position.Z = HorizonHeight - ShorelineStep * ShorelineEachStepHeight;
			Node->Weights = FVector4f(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
}


void AXkSphericalWorldWithOceanActor::GenerateCanvas()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AXkSphericalWorldWithOceanActor::GenerateCanvas);

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
		FVector4f InstanceWeightsValue = Node.Weights;

		InstancePositionData.Add(InstancePositionValue);
		InstanceWeightData.Add(InstanceWeightsValue);
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
	CanvasRendererComponent->DrawHeightWeightCanvas_MultiFrame();
}


void AXkSphericalWorldWithOceanActor::RegenerateWorld()
{
	GenerateHexagons();
	GenerateHexagonalWorld();
	GenerateCanvas();
}