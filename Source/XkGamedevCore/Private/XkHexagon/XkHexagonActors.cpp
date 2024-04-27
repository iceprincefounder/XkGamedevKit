// Copyright ©xukai. All Rights Reserved.

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
	ProcMesh->SetCollisionProfileName(FName(TEXT("IgnoreOnlyPawn")));
	ProcMesh->SetCastShadow(false);
	ProcMesh->SetupAttachment(RootComponent);

	Radius = 100.0;
	BaseInnerGap = 5.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;
	Height = 10.0;
	bShowBaseMesh = true;
	bShowEdgeMesh = true;
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

bool AXkHexagonActor::IsAccessible() const
{
	return ((int32)HexagonType <= AVAILABLEMARK);
}


int32 AXkHexagonActor::CalcCostOffset() const
{
	return (HexagonType == EXkHexagonType::Road) ? -1 : 0;
}

void AXkHexagonActor::SetBaseColor(const FLinearColor& Input)
{ 
	BaseColor = Input; 
	UpdateMaterial();
}

void AXkHexagonActor::SetEdgeColor(const FLinearColor& Input)
{
	EdgeColor = Input;
	UpdateMaterial();
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
	TArray<FVector> VerticesTopArray;
	TArray<FVector> VerticesBtmArray;
	{
		TArray<FVector> VerticesArray;
		TArray<int32> TrianglesArray;
		TArray<FVector> NormalsArray;
		TArray<FProcMeshTangent> TangentsArray;
		TArray<FColor> VertexColorsArray;

		//	x
		//	|   1
		//	| 2/ \6
		//	| | 0 |
		//	| 3\ /5
		//	|   4
		//	---------y

		VerticesArray.Add(FVector(0.0, 0.0, Height));
		VerticesTopArray.Empty();
		VerticesTopArray.Add(FVector((Radius - BaseInnerGap), 0.0, Height));
		VerticesTopArray.Add(FVector((Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		VerticesTopArray.Add(FVector(-(Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		VerticesTopArray.Add(FVector(-(Radius - BaseInnerGap), 0.0, Height));
		VerticesTopArray.Add(FVector(-(Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		VerticesTopArray.Add(FVector((Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		for (const FVector& Vert : VerticesTopArray)
		{
			VerticesArray.Add(Vert);
		}
		TrianglesArray = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0 , 5, 6, 0, 6, 1 };
		VerticesBtmArray.Empty();
		VerticesBtmArray.Add(FVector(Radius, 0.0, 0.0));
		VerticesBtmArray.Add(FVector(Radius * XkSin30, -XkCos30 * Radius, 0.0));
		VerticesBtmArray.Add(FVector(-Radius * XkSin30, -XkCos30 * Radius, 0.0));
		VerticesBtmArray.Add(FVector(-Radius, 0.0, 0.0));
		VerticesBtmArray.Add(FVector(-Radius * XkSin30, XkCos30 * Radius, 0.0));
		VerticesBtmArray.Add(FVector(Radius * XkSin30, XkCos30 * Radius, 0.0));
		for (int32 i = 0; i < VerticesTopArray.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % VerticesTopArray.Num();
			int32 IndexTopB = (i + 1 + 1) % VerticesTopArray.Num();
			int32 IndexBtmA = (i + 1) % VerticesTopArray.Num();
			int32 IndexBtmB = (i + 1 + 1) % VerticesTopArray.Num();

			FVector VertTopA = VerticesTopArray[IndexTopA];
			FVector VertTopB = VerticesTopArray[IndexTopB];
			FVector VertBtmA = VerticesBtmArray[IndexBtmA];
			FVector VertBtmB = VerticesBtmArray[IndexBtmB];

			int32 CurrIndex = VerticesArray.Num();
			VerticesArray.Add(VertTopA);
			VerticesArray.Add(VertTopB);
			VerticesArray.Add(VertBtmA);
			VerticesArray.Add(VertBtmB);

			TrianglesArray.Add(CurrIndex);
			TrianglesArray.Add(CurrIndex + 2);
			TrianglesArray.Add(CurrIndex + 1);
			TrianglesArray.Add(CurrIndex + 1);
			TrianglesArray.Add(CurrIndex + 2);
			TrianglesArray.Add(CurrIndex + 3);
		}
		//TrianglesArray = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0 , 5, 6, 0, 6, 1,
		//1, 7, 2, 2, 7, 8, 2, 8, 3, 3, 8, 9, 3, 9, 4, 4, 9, 10, 4, 10, 5, 5, 10, 11, 5, 11, 6, 6, 11, 12};

		NormalsArray.Init(FVector::ZeroVector, VerticesArray.Num());
		TangentsArray.Init(FProcMeshTangent(0.0f, 0.0f, 0.0f), VerticesArray.Num());
		for (int32 i = 0; i < TrianglesArray.Num() / 3; i++)
		{
			int32 I0 = TrianglesArray[i * 3];
			FVector P0 = VerticesArray[I0];
			int32 I1 = TrianglesArray[i * 3 + 1];
			FVector P1 = VerticesArray[I1];
			int32 I2 = TrianglesArray[i * 3 + 2];
			FVector P2 = VerticesArray[I2];
			FVector N = -FVector::CrossProduct(P1 - P0, P2 - P1).GetSafeNormal();
			FVector T = (P1 - P0).GetUnsafeNormal();
			NormalsArray[I0] += N;
			NormalsArray[I1] += N;
			NormalsArray[I2] += N;
			TangentsArray[I0].TangentX += T;
			TangentsArray[I1].TangentX += T;
			TangentsArray[I2].TangentX += T;
		}
		ProcMesh->CreateMeshSection(BASE_SECTION_INDEX, VerticesArray, TrianglesArray, NormalsArray, TArray<FVector2D>(), TArray<FColor>(), TangentsArray, true);
		ProcMesh->SetMeshSectionVisible(BASE_SECTION_INDEX, bShowBaseMesh);
		ProcMesh->SetMaterial(BASE_SECTION_INDEX, BaseMID);
		ProcMesh->Bounds = FBoxSphereBounds(VerticesArray, VerticesArray.Num());
	}

	{
		// XkHexagon Edge ->|-----------|------------|-------------|----------> XkHexagon Center
		//		    BaseOuterGap EdgeOuterGap EdgeInnerGap BaseInnerGap
		float EdgeHeight = Height + 1; // Fix Z-fighting
		VerticesTopArray.Empty();
		VerticesTopArray.Add(FVector((Radius - EdgeInnerGap), 0.0, EdgeHeight));
		VerticesTopArray.Add(FVector((Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		VerticesTopArray.Add(FVector(-(Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		VerticesTopArray.Add(FVector(-(Radius - EdgeInnerGap), 0.0, EdgeHeight));
		VerticesTopArray.Add(FVector(-(Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		VerticesTopArray.Add(FVector((Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		VerticesBtmArray.Empty();
		VerticesBtmArray.Add(FVector(Radius - EdgeOuterGap, 0.0, EdgeHeight));
		VerticesBtmArray.Add(FVector((Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		VerticesBtmArray.Add(FVector(-(Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		VerticesBtmArray.Add(FVector(-(Radius - EdgeOuterGap), 0.0, EdgeHeight));
		VerticesBtmArray.Add(FVector(-(Radius - EdgeOuterGap) * XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		VerticesBtmArray.Add(FVector((Radius - EdgeOuterGap)* XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));

		TArray<FVector> VerticesArray;
		TArray<int32> TrianglesArray;
		TArray<FVector> NormalsArray;
		TArray<FProcMeshTangent> TangentsArray;
		TArray<FColor> VertexColorsArray;

		for (int32 i = 0; i < VerticesTopArray.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % VerticesTopArray.Num();
			int32 IndexTopB = (i + 1 + 1) % VerticesTopArray.Num();
			int32 IndexBtmA = (i + 1) % VerticesTopArray.Num();
			int32 IndexBtmB = (i + 1 + 1) % VerticesTopArray.Num();

			FVector VertTopA = VerticesTopArray[IndexTopA];
			FVector VertTopB = VerticesTopArray[IndexTopB];
			FVector VertBtmA = VerticesBtmArray[IndexBtmA];
			VertBtmA.Z = VertTopA.Z;
			FVector VertBtmB = VerticesBtmArray[IndexBtmB];
			VertBtmB.Z = VertTopB.Z;

			int32 CurrIndex = VerticesArray.Num();
			VerticesArray.Add(VertTopA);
			VerticesArray.Add(VertTopB);
			VerticesArray.Add(VertBtmA);
			VerticesArray.Add(VertBtmB);

			TrianglesArray.Add(CurrIndex);
			TrianglesArray.Add(CurrIndex + 2);
			TrianglesArray.Add(CurrIndex + 1);
			TrianglesArray.Add(CurrIndex + 1);
			TrianglesArray.Add(CurrIndex + 2);
			TrianglesArray.Add(CurrIndex + 3);
		}

		NormalsArray.Init(FVector::ZeroVector, VerticesArray.Num());
		TangentsArray.Init(FProcMeshTangent(0.0f, 0.0f, 0.0f), VerticesArray.Num());
		for (int32 i = 0; i < TrianglesArray.Num() / 3; i++)
		{
			int32 I0 = TrianglesArray[i * 3];
			FVector P0 = VerticesArray[I0];
			int32 I1 = TrianglesArray[i * 3 + 1];
			FVector P1 = VerticesArray[I1];
			int32 I2 = TrianglesArray[i * 3 + 2];
			FVector P2 = VerticesArray[I2];
			FVector N = -FVector::CrossProduct(P1 - P0, P2 - P1).GetSafeNormal();
			FVector T = (P1 - P0).GetUnsafeNormal();
			NormalsArray[I0] += N;
			NormalsArray[I1] += N;
			NormalsArray[I2] += N;
			TangentsArray[I0].TangentX += T;
			TangentsArray[I1].TangentX += T;
			TangentsArray[I2].TangentX += T;
		}
		ProcMesh->CreateMeshSection(EDGE_SECTION_INDEX, VerticesArray, TrianglesArray, NormalsArray, TArray<FVector2D>(), TArray<FColor>(), TangentsArray, true);
		ProcMesh->SetMeshSectionVisible(EDGE_SECTION_INDEX, bShowEdgeMesh);
		ProcMesh->SetMaterial(EDGE_SECTION_INDEX, EdgeMID);
	}
}


AXkHexagonalWorldActor::AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneRoot = CreateDefaultSubobject<UXkHexagonArrowComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	UObject* BaseMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonBase"));
	BaseMaterial = CastChecked<UMaterialInterface>(BaseMaterialObject);
	UObject* EdgeMaterialObject = StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/XkGamedevKit/Materials/M_HexagonEdge"));
	EdgeMaterial = CastChecked<UMaterialInterface>(EdgeMaterialObject);

	Radius = 100.0;
	BaseInnerGap = 5.0;
	//////////////////////////////////////////
	// EdgeWidth = EdgeInnerGap - EdgeOuterGap
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;
	Height = 10.0;
	GapWidth = 0.0;
	XAxisCount = 10;
	YAxisCount = 10;
	bShowBaseMesh = false;
	bShowEdgeMesh = true;
	BaseColor = FLinearColor::White;
	EdgeColor = FLinearColor::White;
	EdgeColor.A = 0.25f;
	PathfindingMaxStep = 9999;
	BacktrackingMaxStep = 9999;
}


void AXkHexagonalWorldActor::Generate()
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
	for (int32 X = -XAxisCount; X < (XAxisCount + 1); X++)
	{
		for (int32 Y = -YAxisCount; Y < (YAxisCount + 1); Y++)
		{
			float Dist = Radius + GapWidth;
			FVector2D Pos = FXkHexagonAStarPathfinding::CalcHexagonPosition(X, Y, Dist);
			//////////////////////////////////////////////////////////////
			// calculate XkHexagon coordinate
			 FIntVector HexagonCoord = FXkHexagonAStarPathfinding::CalcHexagonCoord(Pos.X, Pos.Y, Dist);
			//////////////////////////////////////////////////////////////
			FVector Location = FVector(Pos.X, Pos.Y, 0.0);
			FActorSpawnParameters ActorSpawnParameters;
			AXkHexagonActor* HexagonActor = GetWorld()->SpawnActor<AXkHexagonActor>(AXkHexagonActor::StaticClass(), Location, FRotator(0.0), ActorSpawnParameters);
			HexagonActor->SetRadius(Radius);
			HexagonActor->SetBaseInnerGap(BaseInnerGap);
			HexagonActor->SetEdgeInnerGap(EdgeInnerGap);
			HexagonActor->SetEdgeOuterGap(EdgeOuterGap);
			HexagonActor->SetHeight(Height);
			HexagonActor->SetCoord(HexagonCoord);
			HexagonActor->SetShowBaseMesh(bShowBaseMesh);
			HexagonActor->SetShowEdgeMesh(bShowEdgeMesh);
			HexagonActor->SetBaseColor(BaseColor);
			HexagonActor->SetEdgeColor(EdgeColor);
			HexagonActor->SetHexagonWorld(this);
			FString CoordString = FString::Printf(
				TEXT("HexagonActor(%i, %i, %i)"), HexagonCoord.X, HexagonCoord.Y, HexagonCoord.Z);
#if WITH_EDITOR
			HexagonActor->SetActorLabel(CoordString);
#endif
			HexagonActor->ConstructionScripts();
			HexagonActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	HexagonAStarPathfinding.Init(GetWorld());
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
	SceneRoot->ArrowZOffset = Height;
	SceneRoot->ArrowMarkStep = Radius;
	SceneRoot->SetArrowLength(XAxisCount * Radius * 1.5 * 2.0);

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
			HexagonActor->SetHeight(Height);
			HexagonActor->SetShowBaseMesh(bShowBaseMesh);
			HexagonActor->SetShowEdgeMesh(bShowEdgeMesh);
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
	int32 MaxManhattanDistance = 9999;
	for (AXkHexagonActor* Target : PendingMovementTargets)
	{
		FIntVector EndPoint = Target->GetCoord();
		int32 ManhattanDistance = FXkHexagonAStarPathfinding::CalcManhattanDistance(StartPoint, EndPoint);
		if (ManhattanDistance < MaxManhattanDistance)
		{
			Result = Target;
			MaxManhattanDistance = ManhattanDistance;
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
