// Copyright ©XUKAI. All Rights Reserved.

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


AXkHexagonActor::AXkHexagonActor()
{
	StaticProcMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticProcMesh"));
	StaticProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticProcMesh->SetCollisionProfileName(FName(TEXT("NoCollision")));
	StaticProcMesh->SetCastShadow(false);
	UObject* Object = StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/XkGamedevKit/Meshes/SM_StandardHexagonWithUV.SM_StandardHexagonWithUV"));
	UStaticMesh* StaticMeshObject = CastChecked<UStaticMesh>(Object);
	StaticProcMesh->SetStaticMesh(StaticMeshObject);
	SetRootComponent(StaticProcMesh);

#if WITH_EDITORONLY_DATA
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProcMesh->SetCollisionProfileName(FName(TEXT("NoCollision")));
	ProcMesh->SetVisibility(false);
	ProcMesh->SetCastShadow(false);
	ProcMesh->SetupAttachment(RootComponent);
#endif
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
			ParentHexagonalWorld->RegenerateHexagonalWorldContext();
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
	if (IsValid(BaseMID))
	{
		BaseMID->SetVectorParameterValue(FName("Color"), InColor);
	}
}


void AXkHexagonActor::OnEdgeHighlight(const FLinearColor& InColor)
{
	if (IsValid(EdgeMID))
	{
		EdgeMID->SetVectorParameterValue(FName("Color"), InColor);
	}
}


void AXkHexagonActor::UpdateMaterial()
{
	if (!BaseMID)
	{
		UObject* BaseMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonBase"));
		BaseMID = UMaterialInstanceDynamic::Create(CastChecked<UMaterialInterface>(BaseMaterialObject), this);
	}
	if (BaseMID && IsValid(BaseMID) && ParentHexagonalWorld.IsValid())
	{
		BaseMID->SetVectorParameterValue(FName("Color"), ParentHexagonalWorld->BaseColor);
	}
	if (!EdgeMID)
	{
		UObject* EdgeMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonEdge"));
		EdgeMID = UMaterialInstanceDynamic::Create(CastChecked<UMaterialInterface>(EdgeMaterialObject), this);
	}
	if (EdgeMID && IsValid(EdgeMID) && ParentHexagonalWorld.IsValid())
	{
		EdgeMID->SetVectorParameterValue(FName("Color"), ParentHexagonalWorld->EdgeColor);
	}
	StaticProcMesh->SetMaterial(0, BaseMID);
	StaticProcMesh->SetMaterial(1, EdgeMID);
}

#if WITH_EDITOR
void AXkHexagonActor::UpdateProcMesh()
{
	if (ParentHexagonalWorld.IsValid())
	{
		TArray<FVector> BaseVertices;
		TArray<int32> BaseIndices;

		TArray<FVector> EdgeVertices;
		TArray<int32> EdgeIndices;

		BuildHexagon(BaseVertices, BaseIndices, EdgeVertices, EdgeIndices, 
			ParentHexagonalWorld->Radius, 
			ParentHexagonalWorld->Height, 
			ParentHexagonalWorld->BaseInnerGap, 
			ParentHexagonalWorld->BaseOuterGap, 
			ParentHexagonalWorld->EdgeInnerGap, 
			ParentHexagonalWorld->EdgeOuterGap);

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
		TArray<FVector2D> BaseUV0s = GenerateUV(BaseVertices, ParentHexagonalWorld->Radius);
		ProcMesh->CreateMeshSection(BASE_SECTION_INDEX, BaseVertices, BaseIndices, TArray<FVector>(), BaseUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		ProcMesh->SetMaterial(BASE_SECTION_INDEX, BaseMID);
		ProcMesh->Bounds = FBoxSphereBounds(BaseVertices, BaseVertices.Num());

		TArray<FVector2D> EdgeUV0s = GenerateUV(EdgeVertices, ParentHexagonalWorld->Radius);
		ProcMesh->CreateMeshSection(EDGE_SECTION_INDEX, EdgeVertices, EdgeIndices, TArray<FVector>(), EdgeUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		ProcMesh->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
	}
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
			FVector NewLocation = FVector(Position.X, Position.Y, Position.Z + 1.0 /* Fix Z-Fighting*/);
			SetActorLocation(NewLocation, true);
		}
	}
	StaticProcMesh->SetVisibility(true);
	StaticProcMesh->MarkRenderStateDirty();
}


void AXkHexagonActor::FreeHexagon()
{
	StaticProcMesh->SetVisibility(false);
	SetActorLocation(FVector(0.0, 0.0, -HALF_WORLD_MAX));
	// Clear hight light colors
	if (BaseMID && IsValid(BaseMID) && EdgeMID && IsValid(EdgeMID))
	{
		BaseMID->SetVectorParameterValue(FName("Color"), FLinearColor(1.0, 1.0, 1.0, 0.0));
		EdgeMID->SetVectorParameterValue(FName("Color"), FLinearColor(1.0, 1.0, 1.0, 0.0));
	}
}


AXkHexagonalWorldActor::AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<UXkHexagonArrowComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InstancedHexagonComponent = CreateDefaultSubobject<UXkInstancedHexagonComponent>(TEXT("InstancedHexagons"));
	InstancedHexagonComponent->SetupAttachment(RootComponent);
	InstancedHexagonComponent->bRenderInMainPass = false;
	InstancedHexagonComponent->bRenderInDepthPass = false;
	UObject* Object = StaticLoadObject(URuntimeVirtualTexture::StaticClass(), NULL, TEXT("/XkGamedevKit/Textures/RVT_InstancedHexagons.RVT_InstancedHexagons"));
	URuntimeVirtualTexture* RVT = CastChecked<URuntimeVirtualTexture>(Object);
	InstancedHexagonComponent->RuntimeVirtualTextures.Empty();
	InstancedHexagonComponent->RuntimeVirtualTextures.Add(RVT);

	Radius = 100.0;
	Height = 10.0;
	GapWidth = 0.0;
	BaseInnerGap = 0.0;
	BaseOuterGap = 0.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;
	BaseColor = FLinearColor(1.0, 1.0, 1.0, 0.0);
	EdgeColor = FLinearColor(1.0, 1.0, 1.0, 0.0);
	MaxManhattanDistance = 64;

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


void AXkHexagonalWorldActor::DebugUpdateWorld()
{
	UpdateHexagonalWorldCustomData();
	RegenerateHexagonalWorldContext();
}


void AXkHexagonalWorldActor::BeginPlay()
{
	HexagonAStarPathfinding.Init(&HexagonalWorldTable);
	Super::BeginPlay();
}

void AXkHexagonalWorldActor::OnConstruction(const FTransform& Transform)
{
#if WITH_EDITOR
	float Distance = Radius + GapWidth;
	SceneRoot->ArrowZOffset = Height;
	SceneRoot->ArrowMarkStep = Radius;
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
	return HexagonalWorldTable.Nodes.Find(InCoord);;
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNode(const FVector& InPosition) const
{
	FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(
		InPosition.X, InPosition.Y, (Radius + GapWidth));
	return GetHexagonNode(InputCoord);
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


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonNodesPathfinding(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList)
{
	TArray<FXkHexagonNode*> FindingNodes;
	TArray<FIntVector> FindingPaths;
	HexagonAStarPathfinding.Reinit();
	HexagonAStarPathfinding.Blocking(BlockList);
	if (HexagonAStarPathfinding.Pathfinding(StartCoord, EndCoord))
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


TArray<FXkHexagonNode*> AXkHexagonalWorldActor::GetHexagonalWorldNodes(const EXkHexagonType HexagonType) const
{
	TArray<FXkHexagonNode*> Results;
	for (TPair<FIntVector, FXkHexagonNode>& NodePair: HexagonalWorldTable.Nodes)
	{
		if (NodePair.Value.Type == HexagonType)
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
	float Distance = Radius + GapWidth;
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
	TArray<FVector4f> EdgeVertices;
	TArray<uint32> EdgeIndices;
	BuildHexagon(OutVertices, OutIndices, EdgeVertices, EdgeIndices,
		Radius, Height, BaseInnerGap, BaseOuterGap, EdgeInnerGap, EdgeOuterGap);
}