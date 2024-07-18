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
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetCollisionProfileName(FName(TEXT("BlockAll")));
	StaticMesh->SetCastShadow(false);
	UObject* Object = StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/XkGamedevKit/Meshes/SM_StandardHexagonWithUV.SM_StandardHexagonWithUV"));
	UStaticMesh* StaticMeshObject = CastChecked<UStaticMesh>(Object);
	StaticMesh->SetStaticMesh(StaticMeshObject);
	SetRootComponent(StaticMesh);

#if WITH_EDITORONLY_DATA
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
	ProcMesh->SetCollisionProfileName(FName(TEXT("NoCollision")));
	ProcMesh->SetVisibility(false);
	ProcMesh->SetCastShadow(false);
	ProcMesh->SetupAttachment(RootComponent);
#endif
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


void AXkHexagonActor::SetHexagonWorld(class AXkHexagonalWorldActor* Input)
{
	CachedHexagonalWorld = MakeWeakObjectPtr<AXkHexagonalWorldActor>(Input);
}


void AXkHexagonActor::OnBaseSelecting(bool bSelecting, const FLinearColor& SelectingColor)
{
	if (IsValid(BaseMID) && CachedHexagonalWorld.IsValid())
	{
		if (bSelecting)
		{
			BaseMID->SetVectorParameterValue(FName("Color"), SelectingColor);
		}
		else
		{
			BaseMID->SetVectorParameterValue(FName("Color"), bCachedBaseHighlight ? CachedBaseHighlightColor : 
				CachedHexagonalWorld->BaseColor);
		}
	}
}


void AXkHexagonActor::OnBaseHighlight(bool bHighlight, const FLinearColor& HighlightColor)
{
	if (IsValid(BaseMID) && CachedHexagonalWorld.IsValid())
	{
		bCachedBaseHighlight = bHighlight;
		CachedBaseHighlightColor = HighlightColor;
		BaseMID->SetVectorParameterValue(FName("Color"), bHighlight ? HighlightColor : 
			CachedHexagonalWorld->BaseColor);
	}
}


void AXkHexagonActor::OnEdgeSelecting(bool bSelecting, const FLinearColor& SelectingColor)
{
	if (IsValid(EdgeMID) && CachedHexagonalWorld.IsValid())
	{
		if (bSelecting)
		{
			EdgeMID->SetVectorParameterValue(FName("Color"), SelectingColor);
		}
		else
		{
			EdgeMID->SetVectorParameterValue(FName("Color"), bCachedEdgeHighlight ? CachedEdgeHighlightColor : 
				CachedHexagonalWorld->EdgeColor);
		}
	}
}


void AXkHexagonActor::OnEdgeHighlight(bool bHighlight, const FLinearColor& HighlightColor)
{
	if (IsValid(EdgeMID) && CachedHexagonalWorld.IsValid())
	{
		bCachedEdgeHighlight = bHighlight;
		CachedEdgeHighlightColor = HighlightColor;
		EdgeMID->SetVectorParameterValue(FName("Color"), bHighlight ? HighlightColor : 
			CachedHexagonalWorld->EdgeColor);
	}
}


void AXkHexagonActor::UpdateMaterial()
{
	if (!BaseMID && CachedHexagonalWorld.IsValid() && CachedHexagonalWorld.IsValid())
	{
		BaseMID = UMaterialInstanceDynamic::Create(CachedHexagonalWorld->HexagonBaseMaterial, this);
	}
	if (BaseMID && IsValid(BaseMID) && CachedHexagonalWorld.IsValid())
	{
		if (CachedHexagonalWorld->BaseColor != FLinearColor::White)
		{
			BaseMID->SetVectorParameterValue(FName("Color"), CachedHexagonalWorld->BaseColor);
		}
	}
	if (!EdgeMID && CachedHexagonalWorld.IsValid() && CachedHexagonalWorld.IsValid())
	{
		EdgeMID = UMaterialInstanceDynamic::Create(CachedHexagonalWorld->HexagonEdgeMaterial, this);
	}
	if (EdgeMID && IsValid(EdgeMID) && CachedHexagonalWorld.IsValid())
	{
		EdgeMID->SetVectorParameterValue(FName("Color"), CachedHexagonalWorld->EdgeColor);
	}
	StaticMesh->SetMaterial(0, BaseMID);
	StaticMesh->SetMaterial(1, EdgeMID);
}

#if WITH_EDITOR
void AXkHexagonActor::UpdateProcMesh()
{
	if (CachedHexagonalWorld.IsValid())
	{
		TArray<FVector> BaseVertices;
		TArray<int32> BaseIndices;

		TArray<FVector> EdgeVertices;
		TArray<int32> EdgeIndices;

		BuildHexagon(BaseVertices, BaseIndices, EdgeVertices, EdgeIndices, 
			CachedHexagonalWorld->Radius, 
			CachedHexagonalWorld->Height, 
			CachedHexagonalWorld->BaseInnerGap, 
			CachedHexagonalWorld->BaseOuterGap, 
			CachedHexagonalWorld->EdgeInnerGap, 
			CachedHexagonalWorld->EdgeOuterGap);

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
		TArray<FVector2D> BaseUV0s = GenerateUV(BaseVertices, CachedHexagonalWorld->Radius);
		ProcMesh->CreateMeshSection(BASE_SECTION_INDEX, BaseVertices, BaseIndices, TArray<FVector>(), BaseUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		ProcMesh->SetMaterial(BASE_SECTION_INDEX, BaseMID);
		ProcMesh->Bounds = FBoxSphereBounds(BaseVertices, BaseVertices.Num());

		TArray<FVector2D> EdgeUV0s = GenerateUV(EdgeVertices, CachedHexagonalWorld->Radius);
		ProcMesh->CreateMeshSection(EDGE_SECTION_INDEX, EdgeVertices, EdgeIndices, TArray<FVector>(), EdgeUV0s, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		ProcMesh->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
	}
}
#endif

AXkHexagonalWorldActor::AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<UXkHexagonArrowComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InstancedHexagonComponent = CreateDefaultSubobject<UXkInstancedHexagonComponent>(TEXT("InstancedHexagons"));
	InstancedHexagonComponent->SetupAttachment(RootComponent);

	UObject* BaseMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonBase"));
	HexagonBaseMaterial = CastChecked<UMaterialInterface>(BaseMaterialObject);
	UObject* EdgeMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonEdge"));
	HexagonEdgeMaterial = CastChecked<UMaterialInterface>(EdgeMaterialObject);
	Radius = 100.0;
	Height = 10.0;
	GapWidth = 0.0;
	BaseInnerGap = 0.0;
	BaseOuterGap = 0.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;
	MaxManhattanDistance = 64;

	PathfindingMaxStep = 9999;
	BacktrackingMaxStep = 9999;
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
		XkHexagonActor->OnBaseHighlight(true, FLinearColor(0.0, 0.0, Fade, 1.0));
	}
}


void AXkHexagonalWorldActor::BeginPlay()
{
	if (!ensure(GetWorld()))
	{
		return;
	}

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
		HexagonStarter->OnBaseHighlight(true, FLinearColor::Red);
		HexagonStarter->UpdateMaterial();
	}
	if (IsValid(HexagonTargeter))
	{
		HexagonTargeter->OnBaseHighlight(true, FLinearColor::Green);
		HexagonTargeter->UpdateMaterial();
	}
#endif
	Super::OnConstruction(Transform);
}


void AXkHexagonalWorldActor::UpdateHexagonalWorld()
{
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


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNodeByCoord(const FIntVector& InCoord)
{
	return HexagonalWorldTable.Nodes.Find(InCoord);;
}


FXkHexagonNode* AXkHexagonalWorldActor::GetHexagonNodeByLocation(const FVector& InPosition)
{
	FIntVector InputCoord = HexagonAStarPathfinding.CalcHexagonCoord(
		InPosition.X, InPosition.Y, (Radius + GapWidth));
	return GetHexagonNodeByCoord(InputCoord);
}


AXkHexagonActor* AXkHexagonalWorldActor::GetHexagonActorByCoord(const FIntVector& InCoord) const
{
	return FindHexagonActor(InCoord);
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
		AXkHexagonActor* HexagonActor = FindHexagonActor(NeighboringCoord);
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


TArray<AXkHexagonActor*> AXkHexagonalWorldActor::PathfindingHexagonActors(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList)
{
	HexagonAStarPathfinding.Reinit();
	HexagonAStarPathfinding.Blocking(BlockList);
	if (HexagonAStarPathfinding.Pathfinding(StartCoord, EndCoord))
	{
		TArray<FIntVector> BacktrackingList = HexagonAStarPathfinding.Backtracking(BacktrackingMaxStep);
		return FindHexagonActors(BacktrackingList);
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


void AXkHexagonalWorldActor::FetchHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	TArray<FVector4f> EdgeVertices;
	TArray<uint32> EdgeIndices;
	BuildHexagon(OutVertices, OutIndices, EdgeVertices, EdgeIndices,
		Radius, Height, BaseInnerGap, BaseOuterGap, EdgeInnerGap, EdgeOuterGap);
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


TArray<class AXkHexagonActor*> AXkHexagonalWorldActor::FindHexagonActors(const TArray<FIntVector>& Inputs) const
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
}


AXkHexagonActor* AXkHexagonalWorldActor::FindHexagonActor(const FIntVector& Input) const
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
}