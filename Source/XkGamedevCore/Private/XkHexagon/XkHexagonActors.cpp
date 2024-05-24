// Copyright ©XUKAI. All Rights Reserved.

#include "XkHexagon/XkHexagonActors.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "LandscapeStreamingProxy.h"
#include "Materials/MaterialInstanceDynamic.h"

static const TArray<FLinearColor> GXkHexagonColor = {
	FLinearColor(0.5, 0.5, 0.5),
	FLinearColor(0.0, 0.15, 0.15),
	FLinearColor(0.0, 0.15, 0.0),
	FLinearColor(0.15, 0.1, 0.0)
};

AXkHexagonActor::AXkHexagonActor()
{
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
	ProcMesh->SetCollisionProfileName(FName(TEXT("BlockAll")));
	ProcMesh->SetCastShadow(false);
	SetRootComponent(ProcMesh);

	Radius = 100.0;
	Height = 10.0;
	GapWidth = 0.0;
	BaseInnerGap = 5.0;
	BaseOuterGap = 0.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;

	BaseColor = FLinearColor::White;
	EdgeColor = FLinearColor::White;
}


void AXkHexagonActor::SnapToLandscape()
{
	// Snap spline point to landscape with a Y-axis offset.
	FVector StartVector = GetActorLocation() + FVector(0, 0, 1) * HALF_WORLD_MAX;
	FVector EndVector = GetActorLocation() + FVector(0, 0, -1) * HALF_WORLD_MAX;
	// Shot a Z-Axis ray in the scene.
	FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllObjects);
	TArray<FHitResult> Results;
	bool bHitWorld = GetWorld()->LineTraceMultiByObjectType(Results, StartVector, EndVector, QueryParams);
	if (bHitWorld)
	{
		for (const FHitResult& HitResult : Results)
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor->IsA(ALandscapeStreamingProxy::StaticClass()))
			{
				FVector HitLocation = HitResult.Location;
				FVector HitNormal = HitResult.ImpactNormal;
				SetActorLocation(HitLocation + FVector(0, 0, 25));
			}
		}
	}
}


void AXkHexagonActor::ConstructionScripts()
{
	UpdateMaterial();

	UpdateProcMesh();
}


void AXkHexagonActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConstructionScripts();
}


#if WITH_EDITOR
void AXkHexagonActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	FVector Location = GetActorLocation();
	if (bFinished && CachedHexagonalWorld.IsValid())
	{	
		HexagonNode = CachedHexagonalWorld->GetHexagonNodeByLocation(Location);
		if (HexagonNode)
		{
			FVector4f Position = HexagonNode->Position;
			FVector NewLocation = FVector(Position.X, Position.Y, Location.Z);
			HexagonNode->Position = FVector4f(NewLocation.X, NewLocation.Y, NewLocation.Z, 1.0f);
			SetActorLocation(NewLocation, true);
			CachedHexagonalWorld->UpdateHexagonalWorld();
		}
	}
}
#endif


bool AXkHexagonActor::IsAccessible() const
{
	return ((int32)HexagonType <= AVAILABLEMARK);
}


int32 AXkHexagonActor::CalcCostOffset() const
{
	return (HexagonType == EXkHexagonType::Road) ? -1 : 0;
}


void AXkHexagonActor::SetHexagonWorld(class AXkHexagonalWorldActor* Input)
{
	CachedHexagonalWorld = MakeWeakObjectPtr<AXkHexagonalWorldActor>(Input);
}


void AXkHexagonActor::OnBaseSelecting(bool bSelecting, const FLinearColor& SelectingColor)
{
	if (IsValid(BaseMID))
	{
		if (bSelecting)
		{
			BaseMID->SetVectorParameterValue(FName("Color"), SelectingColor);
		}
		else
		{
			BaseMID->SetVectorParameterValue(FName("Color"), bCachedBaseHighlight ? CachedBaseHighlightColor : BaseColor);
		}
	}
}


void AXkHexagonActor::OnBaseHighlight(bool bHighlight, const FLinearColor& HighlightColor)
{
	if (IsValid(BaseMID))
	{
		bCachedBaseHighlight = bHighlight;
		CachedBaseHighlightColor = HighlightColor;
		BaseMID->SetVectorParameterValue(FName("Color"), bHighlight ? HighlightColor : BaseColor);
		ProcMesh->SetMeshSectionVisible(BASE_SECTION_INDEX, bHighlight ? true : bShowBaseMesh);
	}
}


void AXkHexagonActor::OnEdgeSelecting(bool bSelecting, const FLinearColor& SelectingColor)
{
	if (IsValid(EdgeMID))
	{
		if (bSelecting)
		{
			EdgeMID->SetVectorParameterValue(FName("Color"), SelectingColor);
		}
		else
		{
			EdgeMID->SetVectorParameterValue(FName("Color"), bCachedEdgeHighlight ? CachedEdgeHighlightColor : EdgeColor);
		}
	}
}


void AXkHexagonActor::OnEdgeHighlight(bool bHighlight, const FLinearColor& HighlightColor)
{
	if (IsValid(EdgeMID))
	{
		bCachedEdgeHighlight = bHighlight;
		CachedEdgeHighlightColor = HighlightColor;
		AXkHexagonActor::OnEdgeHighlight(bHighlight, HighlightColor);
		EdgeMID->SetVectorParameterValue(FName("Color"), bHighlight ? HighlightColor : EdgeColor);
		ProcMesh->SetMeshSectionVisible(EDGE_SECTION_INDEX, bHighlight ? true : bShowEdgeMesh);
	}
}


void AXkHexagonActor::UpdateMaterial()
{
	if (!BaseMID && CachedHexagonalWorld.IsValid() && CachedHexagonalWorld->BaseMaterial)
	{
		BaseMID = UMaterialInstanceDynamic::Create(CachedHexagonalWorld->BaseMaterial, this);
	}
	if (BaseMID && IsValid(BaseMID))
	{
		if (BaseColor != FLinearColor::White)
		{
			BaseMID->SetVectorParameterValue(FName("Color"), BaseColor);
		}
		else
		{
			BaseMID->SetVectorParameterValue(FName("Color"), GXkHexagonColor[(int32)HexagonType]);
		}
	}
	if (!EdgeMID && CachedHexagonalWorld.IsValid() && CachedHexagonalWorld->EdgeMaterial)
	{
		EdgeMID = UMaterialInstanceDynamic::Create(CachedHexagonalWorld->EdgeMaterial, this);
	}
	if (EdgeMID && IsValid(EdgeMID))
	{
		EdgeMID->SetVectorParameterValue(FName("Color"), EdgeColor);
	}
}


void AXkHexagonActor::UpdateProcMesh()
{
	TArray<FVector> BaseVertices;
	TArray<int32> BaseIndices;

	TArray<FVector> EdgeVertices;
	TArray<int32> EdgeIndices;

	BuildHexagon(BaseVertices, BaseIndices, EdgeVertices, EdgeIndices, Radius, Height, BaseInnerGap, BaseOuterGap, EdgeInnerGap, EdgeOuterGap);

	ProcMesh->CreateMeshSection(BASE_SECTION_INDEX, BaseVertices, BaseIndices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProcMesh->SetMeshSectionVisible(BASE_SECTION_INDEX, bShowBaseMesh);
	ProcMesh->SetMaterial(BASE_SECTION_INDEX, BaseMID);
	ProcMesh->Bounds = FBoxSphereBounds(BaseVertices, BaseVertices.Num());

	ProcMesh->CreateMeshSection(EDGE_SECTION_INDEX, EdgeVertices, EdgeIndices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProcMesh->SetMeshSectionVisible(EDGE_SECTION_INDEX, bShowEdgeMesh);
	ProcMesh->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
}


AXkHexagonalWorldActor::AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<UXkHexagonArrowComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	HexagonalWorldComponent = CreateDefaultSubobject<UXkHexagonalWorldComponent>(TEXT("HexagonalWorld"));
	HexagonalWorldComponent->SetupAttachment(RootComponent);

	UObject* BaseMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonBase"));
	BaseMaterial = CastChecked<UMaterialInterface>(BaseMaterialObject);
	UObject* EdgeMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonEdge"));
	EdgeMaterial = CastChecked<UMaterialInterface>(EdgeMaterialObject);

	Radius = 100.0;
	Height = 10.0;
	GapWidth = 0.0;
	BaseInnerGap = 5.0;
	BaseOuterGap = 0.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;

	bSpawnActors = false;
	SpawnActorsMaxMhtDist = 10;
	XAxisCount = 10;
	YAxisCount = 10;
	MaxManhattanDistance = 10;
	bShowHexagonActorsBaseMesh = false;
	bShowHexagonActorsBaseMesh = true;
	bShowHexagonalWorldBaseMesh = false;
	bShowHexagonalWorldEdgeMesh = true;
	BaseColor = FLinearColor::White;
	EdgeColor = FLinearColor::White;
	EdgeColor.A = 0.25f;
	PathfindingMaxStep = 9999;
	BacktrackingMaxStep = 9999;

	GeneratingMaxStep = 9999;
	CenterFieldRange = 1;
	PositionRandom = FVector2D(0.0, 0.0);
	PositionScale = 1.0;
	FalloffCenter = FVector2D::ZeroVector;
	FalloffExtent = FVector2D(3000.0);
	FalloffCornerRadii = 1500.0;
}


void AXkHexagonalWorldActor::GenerateHexagons()
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

	HexagonalWorldComponent->HexagonalWorldNodes.Empty();
	for (int32 X = -XAxisCount; X < (XAxisCount + 1); X++)
	{
		for (int32 Y = -YAxisCount; Y < (YAxisCount + 1); Y++)
		{
			float Dist = HexagonalWorldComponent->Radius + HexagonalWorldComponent->GapWidth;
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

			FXkHexagonNode HexagonNode = FXkHexagonNode(Position, FLinearColor::White, 0, HexagonCoord);
			HexagonalWorldComponent->HexagonalWorldNodes.Add(HexagonCoord, HexagonNode);
			if (bSpawnActors && ManhattanDistanceToCenter < SpawnActorsMaxMhtDist)
			{
				FActorSpawnParameters ActorSpawnParameters;
				AXkHexagonActor* HexagonActor = GetWorld()->SpawnActor<AXkHexagonActor>(AXkHexagonActor::StaticClass(), Location, FRotator(0.0), ActorSpawnParameters);
				HexagonActor->SetRadius(Radius);
				HexagonActor->SetHeight(Height);
				HexagonActor->SetGapWidth(GapWidth);
				HexagonActor->SetBaseInnerGap(BaseInnerGap);
				HexagonActor->SetBaseOuterGap(BaseOuterGap);
				HexagonActor->SetEdgeInnerGap(EdgeInnerGap);
				HexagonActor->SetEdgeOuterGap(EdgeOuterGap);
				HexagonActor->SetShowBaseMesh(bShowHexagonActorsBaseMesh);
				HexagonActor->SetShowEdgeMesh(bShowHexagonActorsEdgeMesh);
				HexagonActor->SetBaseColor(BaseColor);
				HexagonActor->SetEdgeColor(EdgeColor);
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
	HexagonAStarPathfinding.Init(GetWorld());
}


void AXkHexagonalWorldActor::GenerateNoise()
{
	auto RectangleFalloff = [](FVector2D Position, FVector2D Center, FVector2D Extent, float Radius, float CornerRadii) -> float
	{
		FVector2D SafeExtent = Extent - FVector2D(CornerRadii);
		FVector2D FalloffCenter = FVector2D(Extent.X - CornerRadii, Extent.Y - CornerRadii);

		if (FMath::Abs(Position.X) < Extent.X && FMath::Abs(Position.Y) < Extent.Y)
		{
			if (FMath::Abs(Position.X) < SafeExtent.X || FMath::Abs(Position.Y) < SafeExtent.Y)
			{
				return 1.0;
			}
			float Y = FMath::Sqrt(FMath::Square(CornerRadii) - FMath::Square(FMath::Abs(Position.X) - SafeExtent.X)) + SafeExtent.Y;
			if (FMath::Abs(Position.Y) <= Y)
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

	TMap<FIntVector, FXkHexagonNode>& HexagonalWorldNodes = HexagonalWorldComponent->HexagonalWorldNodes;
	uint32 CurrentGeneratingStep = 0;

	TArray<FIntVector> AllNodes;
	HexagonalWorldNodes.GenerateKeyArray(AllNodes);

	TArray<FIntVector> AllLandNodes;
	TArray<FIntVector> TempLandNodes;

	TArray<FIntVector> BeachNodes;

	FVector2D RandomOffset = FVector2D(FMath::RandRange(PositionRandom.X, PositionRandom.Y), FMath::RandRange(PositionRandom.X, PositionRandom.Y));
	for (const FIntVector& NodeCoord : AllNodes)
	{
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Color = FLinearColor::Black;
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
					TempLandNodes.AddUnique(NeighborNodeCoord);
				}
			}
			else
			{
				TempLandNodes.AddUnique(NodeCoord);
			}
		}
		AllLandNodes = TempLandNodes;
		TempLandNodes.Empty();
		CurrentGeneratingStep++;
	}


	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		// 2. noise the border
		for (FIntVector NodeCoord : AllLandNodes)
		{
			TempLandNodes.AddUnique(NodeCoord);
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
						TempLandNodes.AddUnique(NeighborNodeCoord);
					}
				}
			}
		}
		AllLandNodes = TempLandNodes;
		TempLandNodes.Empty();
		CurrentGeneratingStep++;
	}


	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		// 3. find the beach
		for (FIntVector NodeCoord : AllLandNodes)
		{
			TempLandNodes.Add(NodeCoord);
			TArray<FIntVector> NeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NodeCoord);
			if (NotInsideNumber(AllLandNodes, NeighborNodeCoords) != 6)
			{
				for (const FIntVector& NeighborNodeCoord : NeighborNodeCoords)
				{
					if (!AllLandNodes.Contains(NeighborNodeCoord))
					{
						if (!HexagonalWorldNodes.Contains(NeighborNodeCoord))
						{
							continue;
						}
						FXkHexagonNode& Node = HexagonalWorldNodes[NeighborNodeCoord];
						TArray<FIntVector> NeighborsNeighborNodeCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(NeighborNodeCoord);

						TempLandNodes.Add(NeighborNodeCoord);
						if (NotInsideNumber(AllLandNodes, NeighborsNeighborNodeCoords) != 0)
						{
							BeachNodes.Add(NeighborNodeCoord);
						}
					}
				}
			}
		}
		AllLandNodes = TempLandNodes;
		TempLandNodes.Empty();
		CurrentGeneratingStep++;
	}


	// 4. noise the beach
	if (CurrentGeneratingStep < GeneratingMaxStep)
	{
		for (FIntVector NodeCoord : BeachNodes)
		{
			TempLandNodes.AddUnique(NodeCoord);
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
						TempLandNodes.AddUnique(NeighborNodeCoord);
						AllLandNodes.AddUnique(NeighborNodeCoord);
					}
				}
			}
		}
		BeachNodes = TempLandNodes;
		TempLandNodes.Empty();
		CurrentGeneratingStep++;
	}


	// visualize nodes
	for (const FIntVector& NodeCoord : AllLandNodes)
	{
		if (!HexagonalWorldNodes.Contains(NodeCoord))
		{
			continue;
		}
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Position.Z = (100.0);
		Node.Color = FLinearColor::White;
	}
	for (const FIntVector& NodeCoord : BeachNodes)
	{
		if (!HexagonalWorldNodes.Contains(NodeCoord))
		{
			continue;
		}
		FXkHexagonNode& Node = HexagonalWorldNodes[NodeCoord];
		Node.Position.Z = (50.0);
		Node.Color = FLinearColor::Yellow;
	}
}


void AXkHexagonalWorldActor::Pathfinding()
{
	if (!IsValid(HexagonStarter) || !IsValid(HexagonTargeter))
	{
		return;
	}

	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* XkHexagonActor = (*It);
		XkHexagonActor->OnBaseSelecting(false);
		XkHexagonActor->OnBaseHighlight(false);
	}
	HexagonStarter->OnBaseHighlight(true, FLinearColor::Yellow);
	HexagonTargeter->OnBaseHighlight(true, FLinearColor::Yellow);

	TArray<FIntVector> BlockArea;
	for (AXkHexagonActor* HexagonActor : HexagonBlockers)
	{
		BlockArea.Add(HexagonActor->GetCoord());
	}

	TArray<class AXkHexagonActor*> XkHexagonPath;
	HexagonAStarPathfinding.Init(GetWorld());
	HexagonAStarPathfinding.Blocking(BlockArea);
	if (HexagonAStarPathfinding.Pathfinding(HexagonStarter->GetCoord(), HexagonTargeter->GetCoord(), PathfindingMaxStep))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		XkHexagonPath = HexagonAStarPathfinding.FindHexagonActors(BacktrackingList);
	}
	else
	{
		TArray<FIntVector> SearchAreaList = HexagonAStarPathfinding.SearchArea();
		XkHexagonPath = HexagonAStarPathfinding.FindHexagonActors(SearchAreaList);
	}

	for (int32 i = 0; i < XkHexagonPath.Num(); i++)
	{
		float Fade = (float)(i + 1) / (float)XkHexagonPath.Num();
		AXkHexagonActor* XkHexagonActor = XkHexagonPath[i];
		XkHexagonActor->OnBaseHighlight(true, FLinearColor(0.0, 0.0, Fade, 1.0));
	}
}


void AXkHexagonalWorldActor::VisBoundary()
{
	TArray<FIntVector> BlockArea;
	for (AXkHexagonActor* HexagonActor : HexagonBlockers)
	{
		BlockArea.Add(HexagonActor->GetCoord());
	}
	HexagonAStarPathfinding.Init(GetWorld());
	HexagonAStarPathfinding.Blocking(BlockArea);
	TArray<AXkHexagonActor*> Boundary = GetHexagonalWorldBoundary();
	for (AXkHexagonActor* HexagonActor : Boundary)
	{
		HexagonActor->OnBaseHighlight(true, FLinearColor::Red);
	}
}


void AXkHexagonalWorldActor::BeginPlay()
{
	if (!ensure(GetWorld()))
	{
		return;
	}

	HexagonAStarPathfinding.Init(GetWorld());
	Super::BeginPlay();
}


void AXkHexagonalWorldActor::OnConstruction(const FTransform& Transform)
{
#if WITH_EDITOR
	float Distance = Radius + GapWidth;
	SceneRoot->ArrowZOffset = Height;
	SceneRoot->ArrowMarkStep = Radius;
	SceneRoot->SetArrowLength(MaxManhattanDistance * Distance * 1.5);

	HexagonalWorldComponent->Radius = Radius;
	HexagonalWorldComponent->GapWidth = GapWidth;
	HexagonalWorldComponent->BaseInnerGap = BaseInnerGap;
	HexagonalWorldComponent->BaseOuterGap = BaseOuterGap;
	HexagonalWorldComponent->EdgeInnerGap = EdgeInnerGap;
	HexagonalWorldComponent->EdgeOuterGap = EdgeOuterGap;
	HexagonalWorldComponent->MaxManhattanDistance = MaxManhattanDistance;
	HexagonalWorldComponent->bShowBaseMesh = bShowHexagonalWorldBaseMesh;
	HexagonalWorldComponent->bShowEdgeMesh = bShowHexagonalWorldEdgeMesh;
	HexagonalWorldComponent->BaseMaterial = BaseMaterial;
	HexagonalWorldComponent->EdgeMaterial = EdgeMaterial;

	if (IsValid(HexagonStarter))
	{
		HexagonStarter->SetBaseColor(FLinearColor::Red);
	}
	if (IsValid(HexagonTargeter))
	{
		HexagonTargeter->SetBaseColor(FLinearColor::Green);
	}
	// update all HexagonActor settings for hexagonal world
	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* HexagonActor = *It;
		if (IsValid(HexagonActor))
		{
			HexagonActor->SetShowBaseMesh(bShowHexagonActorsBaseMesh);
			HexagonActor->SetShowEdgeMesh(bShowHexagonActorsEdgeMesh);
			HexagonActor->SetBaseColor(BaseColor);
			HexagonActor->SetEdgeColor(EdgeColor);
			HexagonActor->SetHexagonWorld(this);
			HexagonActor->ConstructionScripts();
		}
	}
#endif
	Super::OnConstruction(Transform);
}


bool AXkHexagonalWorldActor::IsHexagonActorsNeighboring(const AXkHexagonActor* A, const AXkHexagonActor* B) const
{
	FIntVector APoint = A->GetCoord();
	FIntVector BPoint = B->GetCoord();
	int32 ManhattanDistance = FXkHexagonAStarPathfinding::CalcManhattanDistance(APoint, BPoint);
	if (ManhattanDistance <= 1)
	{
		return true;
	}
	return false;
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNodeByCoord(const FIntVector& InCoord) const
{
	FXkHexagonNode* Result = nullptr;
	if (HexagonalWorldComponent->HexagonalWorldNodes.Contains(InCoord))
	{
		Result = &HexagonalWorldComponent->HexagonalWorldNodes[InCoord];
	}	
	return Result;
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNodeByLocation(const FVector& InPosition) const
{
	FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(
		InPosition.X, InPosition.Y, (Radius + GapWidth));
	return GetHexagonNodeByCoord(InputCoord);
}


AXkHexagonActor* AXkHexagonalWorldActor::GetHexagonActorByCoord(const FIntVector& InCoord) const
{
	return HexagonAStarPathfinding.FindHexagonActor(InCoord);
}


AXkHexagonActor* AXkHexagonalWorldActor::GetHexagonActorByLocation(const FVector& InPosition) const
{
	FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(
		InPosition.X, InPosition.Y, (Radius + GapWidth));
	return GetHexagonActorByCoord(InputCoord);
}


int32 AXkHexagonalWorldActor::GetHexagonManhattanDistance(const FVector& A, const FVector& B) const
{
	AXkHexagonActor* HexagonA = GetHexagonActorByLocation(A);
	AXkHexagonActor* HexagonB = GetHexagonActorByLocation(B);
	return GetHexagonManhattanDistance(HexagonA, HexagonB);
}


int32 AXkHexagonalWorldActor::GetHexagonManhattanDistance(const AXkHexagonActor* A, const AXkHexagonActor* B) const
{
	if (IsValid(A) && IsValid(B))
	{
		return FXkHexagonAStarPathfinding::CalcManhattanDistance(A->GetCoord(), B->GetCoord());
	}
	return -1;
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::GetHexagonActorNeighbors(const AXkHexagonActor* InputActor) const
{
	TArray<AXkHexagonActor*> Ret;
	TArray<FIntVector> NeighboringXkHexagonCoords = HexagonAStarPathfinding.CalcHexagonNeighboringCoord(InputActor->GetCoord());
	for (const FIntVector NeighboringCoord : NeighboringXkHexagonCoords)
	{
		AXkHexagonActor* HexagonActor = HexagonAStarPathfinding.FindHexagonActor(NeighboringCoord);
		if (IsValid(HexagonActor))
		{
			Ret.Add(HexagonActor);
		}
	}
	return Ret;
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::GetHexagonActorNeighbors(const AXkHexagonActor* InputActor, const uint8 NeighborDistance) const
{
	TArray<AXkHexagonActor*> Results = GetHexagonActorNeighbors(InputActor);
	for (uint8 Index = 1; Index < NeighborDistance; Index++)
	{
		TArray<AXkHexagonActor*> CachedNeighbors = Results;
		for (AXkHexagonActor* CachedNeighbor : CachedNeighbors)
		{
			TArray<AXkHexagonActor*> NewNeighbors = GetHexagonActorNeighbors(CachedNeighbor);
			for (AXkHexagonActor* NewNeighbor : NewNeighbors)
			{
				if (!Results.Contains(NewNeighbor))
				{
					Results.Add(NewNeighbor);
				}
			}
		}
	}
	return Results;
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::GetHexagonActorNeighborsRecursively(const AXkHexagonActor* InputActor, const TArray<FIntVector>& BlockList) const
{
	TArray<AXkHexagonActor*> Result;
	TArray<const AXkHexagonActor*> PendingSearchTargets;
	PendingSearchTargets.Add(InputActor);
	while (Result.Num() == 0)
	{
		TArray<const AXkHexagonActor*> NextPendingSearchTargets;
		for (const AXkHexagonActor* SearchTarget : PendingSearchTargets)
		{
			TArray<AXkHexagonActor*> Neighbors = GetHexagonActorNeighbors(SearchTarget);
			// all neighbors must not in block list
			for (AXkHexagonActor* Neighbor : Neighbors)
			{
				NextPendingSearchTargets.Add(Neighbor);
				FIntVector NeighborCoord = Neighbor->GetCoord();
				if (!BlockList.Contains(NeighborCoord))
				{
					Result.Add(Neighbor);
				}
			}
		}
		PendingSearchTargets = NextPendingSearchTargets;
	}
	return Result;
}


AXkHexagonActor* AXkHexagonalWorldActor::GetHexagonActorRandomNeighbor(const AXkHexagonActor* InputActor, const TArray<FIntVector>& BlockList) const
{
	AXkHexagonActor* Result = nullptr;
	TArray<AXkHexagonActor*> Neighbors = GetHexagonActorNeighborsRecursively(InputActor, BlockList);
	TArray<AXkHexagonActor*> PendingMovementTargets;
	for (AXkHexagonActor* Neighbor : Neighbors)
	{
		FIntVector NeighborCoord = Neighbor->GetCoord();
		if (!BlockList.Contains(NeighborCoord))
		{
			PendingMovementTargets.Add(Neighbor);
		}
	}
	if (PendingMovementTargets.Num() > 0)
	{
		Result = PendingMovementTargets[FMath::RandRange(0, PendingMovementTargets.Num() - 1)];
	}
	return Result;
}


AXkHexagonActor* AXkHexagonalWorldActor::GetHexagonActorNearestNeighbor(const AXkHexagonActor* InputActor, const AXkHexagonActor* TargetActor, const TArray<FIntVector>& BlockList) const
{
	TArray<AXkHexagonActor*> PendingMovementTargets = GetHexagonActorNeighborsRecursively(TargetActor, BlockList);
	AXkHexagonActor* Result = nullptr;
	FIntVector StartPoint = InputActor->GetCoord();
	int32 MaxDist = 9999;
	for (AXkHexagonActor* Target : PendingMovementTargets)
	{
		FIntVector EndPoint = Target->GetCoord();
		int32 ManhattanDistance = FXkHexagonAStarPathfinding::CalcManhattanDistance(StartPoint, EndPoint);
		if (ManhattanDistance < MaxDist)
		{
			Result = Target;
			MaxDist = ManhattanDistance;
		}
	}
	return Result;
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::GetHexagonalWorldBoundary() const
{
	TArray<AXkHexagonActor*> Results;
	for (int32 X = -XAxisCount; X < (XAxisCount + 1); X++)
	{
		for (int32 Y = -YAxisCount; Y < (YAxisCount + 1); Y++)
		{
			if (FMath::Abs(X) == XAxisCount || FMath::Abs(Y) == YAxisCount)
			{
				float Dist = Radius + GapWidth;
				FVector2D Pos = HexagonAStarPathfinding.CalcHexagonPosition(X, Y, Dist);
				FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(Pos.X, Pos.Y, Dist);
				AXkHexagonActor* HexagonActor = HexagonAStarPathfinding.FindHexagonActor(InputCoord);
				if (HexagonActor && IsValid(HexagonActor))
				{
					Results.Add(HexagonActor);
				}
			}
		}
	}
	return Results;
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::PathfindingHexagonActors(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList)
{
	HexagonAStarPathfinding.Reinit();
	HexagonAStarPathfinding.Blocking(BlockList);
	if (HexagonAStarPathfinding.Pathfinding(StartCoord, EndCoord))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		return HexagonAStarPathfinding.FindHexagonActors(BacktrackingList);
	}
	return TArray<AXkHexagonActor*>();
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::PathfindingHexagonActors(const AXkHexagonActor* StartActor, const AXkHexagonActor* EndActor, const TArray<FIntVector>& BlockList)
{
	return PathfindingHexagonActors(StartActor->GetCoord(), EndActor->GetCoord(), BlockList);
}


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::PathfindingHexagonActors(const FVector& StartLocation, const FVector& EndLocation, const TArray<FIntVector>& BlockList)
{
	AXkHexagonActor* StartActor = GetHexagonActorByLocation(StartLocation);
	AXkHexagonActor* EndActor = GetHexagonActorByLocation(EndLocation);
	if (IsValid(StartActor) && IsValid(EndActor))
	{
		return PathfindingHexagonActors(StartActor, EndActor, BlockList);
	}
	return TArray<AXkHexagonActor*>();
}
