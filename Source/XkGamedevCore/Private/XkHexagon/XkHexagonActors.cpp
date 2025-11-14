// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkHexagon/XkHexagonActors.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "LandscapeStreamingProxy.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VT/RuntimeVirtualTexture.h"

static const TArray<FLinearColor> GXkHexagonColor = {
	FLinearColor(0.5, 0.5, 0.5),
	FLinearColor(0.0, 0.15, 0.15),
	FLinearColor(0.0, 0.15, 0.0),
	FLinearColor(0.15, 0.1, 0.0)
};


AXkHexagonActor::AXkHexagonActor(const FObjectInitializer& ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(SceneRoot);

	auto InitStaticMeshComponent = [this](UStaticMeshComponent*& StaticMeshComp)
		{
			StaticMeshComp->SetMobility(EComponentMobility::Movable);
			StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			StaticMeshComp->SetCastShadow(false);
			StaticMeshComp->bAffectDistanceFieldLighting = false;
			StaticMeshComp->bAffectDynamicIndirectLighting = false;
			StaticMeshComp->bAffectIndirectLightingWhileHidden = false;
			StaticMeshComp->SetupAttachment(RootComponent);
		};
	{
		StaticMeshBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshBase"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/XkGamedevKit/Meshes/SM_HexagonBase.SM_HexagonBase"));
		UStaticMesh* StaticMeshObject = ObjectFinder.Object;
		StaticMeshBase->SetStaticMesh(StaticMeshObject);
		InitStaticMeshComponent(StaticMeshBase);
	}
	{
		StaticMeshEdge = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshEdge"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/XkGamedevKit/Meshes/SM_HexagonEdge.SM_HexagonEdge"));
		UStaticMesh* StaticMeshObject = ObjectFinder.Object;
		StaticMeshEdge->SetStaticMesh(StaticMeshObject);
		InitStaticMeshComponent(StaticMeshEdge);
	}
	{
		StaticMeshPivot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshPivot"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/XkGamedevKit/Meshes/SM_HexagonPivot.SM_HexagonPivot"));
		UStaticMesh* StaticMeshObject = ObjectFinder.Object;
		StaticMeshPivot->SetStaticMesh(StaticMeshObject);
		InitStaticMeshComponent(StaticMeshPivot);
	}


#if WITH_EDITORONLY_DATA
	ProcMeshBase = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMeshBase"));
	ProcMeshBase->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProcMeshBase->SetCollisionProfileName(FName(TEXT("NoCollision")));
	ProcMeshBase->SetVisibility(false);
	ProcMeshBase->SetCastShadow(false);
	ProcMeshBase->SetupAttachment(RootComponent);

	ProcMeshEdge = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMeshEdge"));
	ProcMeshEdge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProcMeshEdge->SetCollisionProfileName(FName(TEXT("NoCollision")));
	ProcMeshEdge->SetVisibility(false);
	ProcMeshEdge->SetCastShadow(false);
	ProcMeshEdge->SetupAttachment(RootComponent);
#endif

	BaseMaterial = StaticMeshBase->GetMaterial(BASE_SECTION_INDEX);
	EdgeMaterial = StaticMeshEdge->GetMaterial(EDGE_SECTION_INDEX);
	PivotMaterial = StaticMeshPivot->GetMaterial(PIVOT_SECTION_INDEX);
}


void AXkHexagonActor::ConstructionScripts()
{
	UpdateMaterial();
#if WITH_EDITOR
	UpdateProcMesh();
#endif
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
	if (bFinished && ParentHexagonalWorld.IsValid())
	{	
		FXkHexagonNode* HexagonNode = ParentHexagonalWorld->GetHexagonNode(Location);
		if (HexagonNode)
		{
			FVector4f Position = HexagonNode->Position;
			FVector NewLocation = FVector(Position.X, Position.Y, Location.Z);
			HexagonNode->Position = FVector4f(NewLocation.X, NewLocation.Y, NewLocation.Z, 1.0f);
			SetActorLocation(NewLocation, true);
		}
	}
}
#endif


void AXkHexagonActor::SetHexagonWorld(class AXkHexagonalWorldActor* Input)
{
	ParentHexagonalWorld = MakeWeakObjectPtr<AXkHexagonalWorldActor>(Input);
}


void AXkHexagonActor::OnBaseHighlight(const FLinearColor& InColor)
{
	if (InColor == FLinearColor::Transparent)
	{
		StaticMeshBase->SetVisibility(false);
		StaticMeshBase->MarkRenderStateDirty();
		return;
	}
	StaticMeshBase->SetVisibility(true);
	if (IsValid(BaseMID))
	{
		BaseMID->SetVectorParameterValue(FName("Color"), InColor);
	}
}


void AXkHexagonActor::OnEdgeHighlight(const FLinearColor& InColor)
{
	if (InColor == FLinearColor::Transparent)
	{
		StaticMeshEdge->SetVisibility(false);
		StaticMeshEdge->MarkRenderStateDirty();
		return;
	}
	StaticMeshEdge->SetVisibility(true);
	if (IsValid(EdgeMID))
	{
		EdgeMID->SetVectorParameterValue(FName("Color"), InColor);
	}
}


void AXkHexagonActor::OnPivotHighlight(const FLinearColor& InColor)
{
	if (InColor == FLinearColor::Transparent)
	{
		StaticMeshPivot->SetVisibility(false);
		StaticMeshPivot->MarkRenderStateDirty();
		return;
	}
	StaticMeshPivot->SetVisibility(true);
	if (IsValid(PivotMID))
	{
		PivotMID->SetVectorParameterValue(FName("Color"), InColor);
		StaticMeshPivot->SetMaterial(PIVOT_SECTION_INDEX, PivotMID);
		StaticMeshPivot->MarkRenderStateDirty();
	}
}


void AXkHexagonActor::UpdateMaterial()
{
	if (!BaseMID && BaseMaterial)
	{
		BaseMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		StaticMeshBase->SetMaterial(BASE_SECTION_INDEX, BaseMID);
	}
	if (!EdgeMID && EdgeMaterial)
	{
		EdgeMID = UMaterialInstanceDynamic::Create(EdgeMaterial, this);
		StaticMeshEdge->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
	}
	if (!PivotMID && PivotMaterial)
	{
		PivotMID = UMaterialInstanceDynamic::Create(PivotMaterial, this);
		StaticMeshPivot->SetMaterial(PIVOT_SECTION_INDEX, PivotMID);
	}
}

#if WITH_EDITOR
void AXkHexagonActor::UpdateProcMesh()
{
	TArray<FVector> BaseVertices;
	TArray<int32> BaseIndices;

	TArray<FVector> EdgeVertices;
	TArray<int32> EdgeIndices;

	BuildHexagon(BaseVertices, BaseIndices, EdgeVertices, EdgeIndices,
		HEXAGON_RADIUS,
		HEXAGON_HEIGHT,
		HEXAGON_BASE_INNER_GAP,
		HEXAGON_BASE_OUTER_GAP,
		HEXAGON_EDGE_INNER_GAP,
		HEXAGON_EDGE_OUTER_GAP);

	auto GenerateUV = [](const TArray<FVector>& Vertices, const float Radius) -> TArray<FVector2D>
		{
			TArray<FVector2D> UV0s;
			UV0s.Init(FVector2D::ZeroVector, Vertices.Num());
			for (int32 i = 0; i < Vertices.Num(); i++)
			{
				FVector Position = Vertices[i];
				FVector2D UV = FVector2D(Position.Y, -Position.X);
				UV = (UV + Radius) / (Radius * 2.0);
				UV0s[i] = UV;
			}
			return UV0s;
		};
	TArray<FVector2D> BaseUV0s = GenerateUV(BaseVertices, HEXAGON_RADIUS);
	ProcMeshBase->CreateMeshSection(BASE_SECTION_INDEX, BaseVertices, BaseIndices, TArray<FVector>(), BaseUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProcMeshBase->SetMaterial(BASE_SECTION_INDEX, BaseMID);
	ProcMeshBase->Bounds = FBoxSphereBounds(FBox(BaseVertices));

	TArray<FVector2D> EdgeUV0s = GenerateUV(EdgeVertices, HEXAGON_RADIUS);
	ProcMeshEdge->CreateMeshSection(EDGE_SECTION_INDEX, EdgeVertices, EdgeIndices, TArray<FVector>(), EdgeUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProcMeshEdge->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
}
#endif


void AXkHexagonActor::InitHexagon(const FIntVector& InCoord)
{
	Coord = InCoord;
	if (ParentHexagonalWorld.IsValid())
	{
		FXkHexagonNode* HexagonNode = ParentHexagonalWorld->GetHexagonNode(Coord);
		if (HexagonNode)
		{
			FVector4f Position = HexagonNode->Position;
			FVector NewLocation = FVector(Position.X, Position.Y, Position.Z);
			SetActorLocation(NewLocation, true);
		}
	}
	StaticMeshBase->SetVisibility(true);
	StaticMeshBase->MarkRenderStateDirty();
	StaticMeshEdge->SetVisibility(true);
	StaticMeshEdge->MarkRenderStateDirty();
	StaticMeshPivot->SetVisibility(true);
	StaticMeshPivot->MarkRenderStateDirty();
}


void AXkHexagonActor::FreeHexagon()
{
	StaticMeshBase->SetVisibility(false);
	StaticMeshBase->MarkRenderStateDirty();
	StaticMeshEdge->SetVisibility(false);
	StaticMeshEdge->MarkRenderStateDirty();
	StaticMeshPivot->SetVisibility(false);
	StaticMeshPivot->MarkRenderStateDirty();
	SetActorLocation(FVector(0.0, 0.0, -HALF_WORLD_MAX));
	StaticMeshBase->SetRelativeLocation(FVector::ZeroVector);
	StaticMeshEdge->SetRelativeLocation(FVector::ZeroVector);
	StaticMeshBase->SetRelativeScale3D(FVector::OneVector);
	StaticMeshEdge->SetRelativeScale3D(FVector::OneVector);
	StaticMeshPivot->SetRelativeLocation(FVector::ZeroVector);
	StaticMeshPivot->SetRelativeScale3D(FVector::OneVector);
	// Clear hight light colors
	if (BaseMID && IsValid(BaseMID) && EdgeMID && IsValid(EdgeMID) && PivotMID && IsValid(PivotMID))
	{
		BaseMID->SetVectorParameterValue(FName("Color"), FLinearColor::Transparent);
		EdgeMID->SetVectorParameterValue(FName("Color"), FLinearColor::Transparent);
		PivotMID->SetVectorParameterValue(FName("Color"), FLinearColor::Transparent);
	}
}


AXkHexagonalWorldActor::AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<UXkHexagonArrowComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InstancedHexagonComponent = CreateDefaultSubobject<UXkInstancedHexagonComponent>(TEXT("InstancedHexagons"));
	InstancedHexagonComponent->SetupAttachment(RootComponent);
	InstancedHexagonComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InstancedHexagonComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	InstancedHexagonComponent->SetVisibility(false);
	InstancedHexagonComponent->bRenderInMainPass = false;
	InstancedHexagonComponent->bRenderInDepthPass = false;
	InstancedHexagonComponent->CastShadow = false;
	InstancedHexagonComponent->bCastDynamicShadow = false;
	InstancedHexagonComponent->bCastStaticShadow = false;

	//static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> ObjectFinder(TEXT("/XkGamedevKit/Textures/RVT_InstancedHexagons"));
	//URuntimeVirtualTexture* RVT = ObjectFinder.Object;
	//InstancedHexagonComponent->RuntimeVirtualTextures.Empty();
	//InstancedHexagonComponent->RuntimeVirtualTextures.Add(RVT);

	BaseColor = FLinearColor(1.0, 1.0, 1.0, 0.0);
	EdgeColor = FLinearColor(1.0, 1.0, 1.0, 0.0);
	MaxManhattanDistance = 32;
	HorizonHeight = 100.0;

	PathfindingMaxStep = 9999;
	BacktrackingMaxStep = 9999;
}


void AXkHexagonalWorldActor::DebugPathfinding()
{
#if WITH_EDITOR
	auto FindHexagonActor = [this](const FIntVector& Input) -> AXkHexagonActor*
		{
			for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
			{
				AXkHexagonActor* HexagonActor = *It;
				if (HexagonActor->GetCoord() == Input)
				{
					return HexagonActor;
				}
			}
			return nullptr;
		};

	auto FindHexagonActors = [FindHexagonActor](const TArray<FIntVector>& Inputs) -> TArray<AXkHexagonActor*>
		{
			TArray<class AXkHexagonActor*> Ret;
			for (const FIntVector& CurrentPoint : Inputs)
			{
				if (AXkHexagonActor* HexagonActor = FindHexagonActor(CurrentPoint))
				{
					Ret.Insert(HexagonActor, 0);
				}
			}
			return Ret;
		};

	if (!IsValid(HexagonStarter) || !IsValid(HexagonTargeter))
	{
		return;
	}

	for (TActorIterator<AXkHexagonActor> It(GetWorld()); It; ++It)
	{
		AXkHexagonActor* XkHexagonActor = (*It);
		XkHexagonActor->OnBaseHighlight();
	}
	HexagonStarter->OnBaseHighlight(FLinearColor::Yellow);
	HexagonTargeter->OnBaseHighlight(FLinearColor::Yellow);

	TArray<FIntVector> BlockArea;
	for (AXkHexagonActor* HexagonActor : HexagonBlockers)
	{
		BlockArea.Add(HexagonActor->GetCoord());
	}

	TArray<class AXkHexagonActor*> FindingPathHexagonActors;
	HexagonAStarPathfinding.Init(&HexagonalWorldTable);
	HexagonAStarPathfinding.Blocking(BlockArea);
	if (HexagonAStarPathfinding.Pathfinding(HexagonStarter->GetCoord(), HexagonTargeter->GetCoord(), PathfindingMaxStep))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		FindingPathHexagonActors = FindHexagonActors(BacktrackingList);
	}
	else
	{
		TArray<FIntVector> SearchAreaList = HexagonAStarPathfinding.SearchArea();
		FindingPathHexagonActors = FindHexagonActors(SearchAreaList);
	}

	for (int32 i = 0; i < FindingPathHexagonActors.Num(); i++)
	{
		float Fade = (float)(i + 1) / (float)FindingPathHexagonActors.Num();
		AXkHexagonActor* XkHexagonActor = FindingPathHexagonActors[i];
		XkHexagonActor->OnBaseHighlight(FLinearColor(0.0, 0.0, Fade, 1.0));
	}
#endif
}


void AXkHexagonalWorldActor::BeginPlay()
{
	HexagonAStarPathfinding.Init(&HexagonalWorldTable);
	Super::BeginPlay();
}

void AXkHexagonalWorldActor::OnConstruction(const FTransform& Transform)
{
#if WITH_EDITOR
	float Distance = HEXAGON_RADIUS + HEXAGON_GAP_WIDTH;
	SceneRoot->ArrowHeight = HEXAGON_HEIGHT;
	SceneRoot->ArrowUnitStep = HEXAGON_RADIUS;
	SceneRoot->SetArrowLength(MaxManhattanDistance * Distance * 1.5);

	if (IsValid(HexagonStarter))
	{
		HexagonStarter->OnBaseHighlight(FLinearColor::Red);
		HexagonStarter->RerunConstructionScripts();
	}
	if (IsValid(HexagonTargeter))
	{
		HexagonTargeter->OnBaseHighlight(FLinearColor::Green);
		HexagonTargeter->RerunConstructionScripts();
	}
#endif
	Super::OnConstruction(Transform);
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNode(const FIntVector& InCoord) const
{
	return HexagonalWorldTable.Nodes.Find(InCoord);
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNode(const FVector& InPosition) const
{
	FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(
		InPosition.X, InPosition.Y, (HEXAGON_RADIUS + HEXAGON_GAP_WIDTH));
	FXkHexagonNode* HexagonNode = GetHexagonNode(InputCoord);
	if (HexagonNode)
	{
		return HexagonNode;
	}

	return HexagonNode;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodeNeighbors(const FIntVector& InCoord) const
{
	TArray<FXkHexagonNode*> HexagonNodeNeighbors;
	TArray<FIntVector> NeighborCoords = FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(InCoord);
	for (const FIntVector& NeighborCoord : NeighborCoords)
	{
		FXkHexagonNode* HexagonNode = GetHexagonNode(NeighborCoord);
		if (HexagonNode)
		{
			HexagonNodeNeighbors.AddUnique(HexagonNode);
		}
	}
	return HexagonNodeNeighbors;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodeSurrounders(const TArray<FIntVector>& InCoords) const
{
	TArray<FXkHexagonNode*> HexagonNodeSurrounders;
	TArray<FIntVector> SurroundersCoords = FXkHexagonAStarPathfinding::CalcHexagonSurroundingCoord(InCoords);
	for (const FIntVector& NeighborCoord : SurroundersCoords)
	{
		FXkHexagonNode* HexagonNode = GetHexagonNode(NeighborCoord);
		if (HexagonNode)
		{
			HexagonNodeSurrounders.AddUnique(HexagonNode);
		}
	}
	return HexagonNodeSurrounders;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodeCoverages(const FIntVector& InCoord, const int32 InRange) const
{
	TArray<FXkHexagonNode*> Results;
	for (TPair<FIntVector, FXkHexagonNode>& NodePair : HexagonalWorldTable.Nodes)
	{
		if (FXkHexagonAStarPathfinding::CalcManhattanDistance(InCoord, NodePair.Key) <= InRange)
		{
			FXkHexagonNode* HexagonNode = &NodePair.Value;
			if (HexagonNode)
			{
				Results.AddUnique(HexagonNode);
			}
		}
	}
	return Results;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodesPath(const FIntVector& StartCoord, const FIntVector& EndCoord)
{
	TArray<FXkHexagonNode*> FindingNodes;
	TArray<FIntVector> FindingPaths;
	HexagonAStarPathfinding.Reinit();
	if (HexagonAStarPathfinding.Pathfinding(StartCoord, EndCoord, PathfindingMaxStep))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		FindingPaths = BacktrackingList;
	}
	// It is Backtrack, reverse the order
	for (int32 Index = FindingPaths.Num() - 1; Index >= 0; --Index)
	{
		FIntVector FindingCoord = FindingPaths[Index];
		FXkHexagonNode* HexagonNode = GetHexagonNode(FindingCoord);
		if (HexagonNode)
		{
			FindingNodes.AddUnique(HexagonNode);
		}
	}
	return FindingNodes;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodesPathfinding(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList)
{
	TArray<FXkHexagonNode*> FindingNodes;
	TArray<FIntVector> FindingPaths;
	TArray<FIntVector> Blockers = BlockList;
	HexagonAStarPathfinding.Reinit();
	HexagonAStarPathfinding.Blocking(Blockers);
	if (HexagonAStarPathfinding.Pathfinding(StartCoord, EndCoord, PathfindingMaxStep))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		FindingPaths = BacktrackingList;
	}

	// It is Backtrack, reverse the order
	for (int32 Index = FindingPaths.Num() - 1; Index >= 0; --Index)
	{
		FIntVector FindingCoord = FindingPaths[Index];
		FXkHexagonNode* HexagonNode = GetHexagonNode(FindingCoord);
		if (HexagonNode && !BlockList.Contains(HexagonNode->Coord))
		{
			FindingNodes.AddUnique(HexagonNode);
		}
	}
	return FindingNodes;
}


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonalWorldNodes(const EXkHexagonType HexagonType) const
{
	TArray<FXkHexagonNode*> Results;
	for (TPair<FIntVector, FXkHexagonNode>& NodePair: HexagonalWorldTable.Nodes)
	{
		if (HexagonNodeIsValid(&NodePair.Value) && HexagonNodeHasAnyFlags(&NodePair.Value, HexagonType))
		{
			Results.AddUnique(&NodePair.Value);
		}
	}
	return Results;
}


int32 AXkHexagonalWorldActor::GetHexagonManhattanDistance(const FVector& A, const FVector& B) const
{
	FXkHexagonNode* HexagonA = GetHexagonNode(A);
	FXkHexagonNode* HexagonB = GetHexagonNode(B);
	if (HexagonA && HexagonB)
	{
		return FXkHexagonAStarPathfinding::CalcManhattanDistance(HexagonA->Coord, HexagonB->Coord);
	}
	return -1;
}


FVector2D AXkHexagonalWorldActor::GetHexagonalWorldExtent() const
{
	float Distance = HEXAGON_RADIUS + HEXAGON_GAP_WIDTH;
	float X = MaxManhattanDistance * Distance * 1.5 + Distance;
	float Y = MaxManhattanDistance * Distance * 2.0 * XkCos30 + Distance * XkCos30;
	return FVector2D(X, Y);
}


FVector2D AXkHexagonalWorldActor::GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const
{
	// UnscaledPatchCoverage is meant to represent the distance between the centers of the extremal pixels.
	// That distance in pixels is Resolution-1.
	FVector2D TargetPixelSize(UnscaledPatchCoverage / FVector2D::Max(Resolution - 1, FVector2D(1, 1)));
	return TargetPixelSize * Resolution;
}


void AXkHexagonalWorldActor::BuildHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	TArray<FVector4f> TmpEdgeVertices;
	TArray<uint32> TmpEdgeIndices;
	BuildHexagon(OutVertices, OutIndices, TmpEdgeVertices, TmpEdgeIndices,
		HEXAGON_RADIUS, HEXAGON_HEIGHT, HEXAGON_BASE_INNER_GAP, HEXAGON_BASE_OUTER_GAP, HEXAGON_EDGE_INNER_GAP, HEXAGON_EDGE_OUTER_GAP);
}