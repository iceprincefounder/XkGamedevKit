// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkLandscape/XkLandscapeComponents.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
#include "XkLandscape/XkLandscapeSceneProxy.h"
#include "XkRenderer/XkRendererComponents.h"
#include "Materials/MaterialParameterCollectionInstance.h"

#include "Components/DynamicMeshComponent.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"


static bool VisQuadtreeBounds = 0;
static FAutoConsoleVariableRef CVarVisQuadtreeBounds(
	TEXT("r.xk.VisQuadtreeBounds"),
	VisQuadtreeBounds,
	TEXT("Vis Quadtree Bounds"));


UXkLandscapeComponent::UXkLandscapeComponent(const FObjectInitializer& ObjectInitializer) 
	: UXkQuadtreeComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bTickInEditor = true;
	
	// Enable shadow casting.
	// NOTE: This component uses a world-sized bounds (WORLD_MAX) in CalcBounds,
	// so per-object projected shadow paths must be disabled. Otherwise
	// FProjectedShadowInfo::SetupPerObjectProjection produces a degenerate
	// matrix and triggers an ensure in TMatrix::InverseFast().
	CastShadow = true;
	bCastDynamicShadow = true;
	bCastStaticShadow = true;
	bCastVolumetricTranslucentShadow = false;
	bCastContactShadow = false;
	bCastHiddenShadow = false;
	bCastFarShadow = false;
	bCastShadowAsTwoSided = false;
	bCastInsetShadow = false; // Must be false: avoids per-object shadow path.

	Material = CastChecked<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/Engine/EngineMaterials/WorldGridMaterial")));
}


void UXkLandscapeComponent::PostLoad()
{
	Super::PostLoad();
}


FPrimitiveSceneProxy* UXkLandscapeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* LocalSceneProxy = NULL;

	if (Material)
	{
		FPrimitiveSceneProxy* Proxy = new FXkLandscapeSceneProxy(
			this, NAME_None, Material->GetRenderProxy());
		LocalSceneProxy = Proxy;
	}
	return LocalSceneProxy;
}


FBoxSphereBounds UXkLandscapeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds BoxSphereBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(WORLD_MAX, WORLD_MAX, 6400), WORLD_MAX);
	return FBoxSphereBounds(BoxSphereBounds).TransformBy(LocalToWorld);
}


void UXkLandscapeComponent::OnRegister()
{
	Super::OnRegister();
	EnsureTileComponents();

	// PostEditChangeProperty on any UPROPERTY triggers component reregister, which runs
	// OnUnregister -> ReleaseTileComponents (destroys all DMC tiles) then OnRegister
	// -> EnsureTileComponents (creates fresh empty tiles). The CanvasRenderer's heightfield
	// is unchanged, so OnCPUHeightFieldRebuilt never fires and the new tiles stay empty.
	// Re-seed synchronously here so editor edits never leave tiles blank.
	if (CanvasRenderer.IsValid() && CanvasRenderer->IsCPUHeightFieldValid())
	{
		MarkAllTilesDirty();
		FlushDirtyTiles();
	}
}


void UXkLandscapeComponent::OnUnregister()
{
	if (CanvasRenderer.IsValid() && HeightFieldRebuiltHandle.IsValid())
	{
		CanvasRenderer->OnCPUHeightFieldRebuilt.Remove(HeightFieldRebuiltHandle);
	}
	HeightFieldRebuiltHandle.Reset();
	PendingTileCookTasks.Empty();
	ReleaseTileComponents();
	Super::OnUnregister();
}

void UXkLandscapeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (VisQuadtreeBounds)
	{
		if (FXkLandscapeSceneProxy* LocalSceneProxy = static_cast<FXkLandscapeSceneProxy*>(SceneProxy))
		{
			const TArray<int32> VisibleNodes = LocalSceneProxy->Quadtree.GetVisibleNodes();
			for (int32 i = 0; i < VisibleNodes.Num(); i++)
			{
				float Thickness = 100.0;
				int32 NodeID = VisibleNodes[i];
				FVector RootOffset = LocalSceneProxy->Quadtree.GetRootOffset();
				int32 MaxLod = LocalSceneProxy->Quadtree.GetCullLodDistance().Num();
				FVector Origin = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetCenter();
				FVector Extent = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetExtent();
				int32 Depth = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeDepth();
				Origin += RootOffset;
				Thickness *= (float(MaxLod - Depth) / (float)MaxLod);
				::DrawDebugBox(GetWorld(), Origin, Extent, Hue2RGB(Depth).ToFColorSRGB(), false, -1.0f, SDPG_Foreground, Thickness);
			}
		}
	}

	// Drain up to MaxTileCooksPerFrame tile-cook tasks this frame.
	ProcessPendingTileCooks();
}


FMaterialRelevance UXkLandscapeComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= Material->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkLandscapeComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (Material)
	{
		OutMaterials.Add(Material);
	}
}


void UXkLandscapeComponent::FetchPatchData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	if (FXkLandscapeSceneProxy* LocalSceneProxy = static_cast<FXkLandscapeSceneProxy*>(SceneProxy))
	{
		OutVertices = LocalSceneProxy->PatchData.Vertices;
		OutIndices = LocalSceneProxy->PatchData.Indices;
	}
}


void UXkLandscapeComponent::EnsureTileComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const int32 TotalTiles = FMath::Max(TileCount.X, 1) * FMath::Max(TileCount.Y, 1);
	if (TileComponents.Num() == TotalTiles)
	{
		return;
	}

	ReleaseTileComponents();

	TileComponents.Reserve(TotalTiles);
	for (int32 TileY = 0; TileY < TileCount.Y; ++TileY)
	{
		for (int32 TileX = 0; TileX < TileCount.X; ++TileX)
		{
			const FName CompName = *FString::Printf(TEXT("LandscapeCollisionTile_%d_%d"), TileX, TileY);
			UDynamicMeshComponent* Tile = NewObject<UDynamicMeshComponent>(Owner, CompName, RF_Transient | RF_DuplicateTransient);
			Tile->CreationMethod = EComponentCreationMethod::Instance;
			Tile->SetFlags(RF_Transient | RF_DuplicateTransient);
			Tile->SetMobility(EComponentMobility::Movable);
			Tile->bEnableComplexCollision = true;
			Tile->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
			// Async cook + deferred collision updates: the physics trimesh BVH build is the
			// single most expensive step of RebuildTileMesh. Running it on a worker thread
			// keeps the game thread responsive; the few-frame latency before the new trimesh
			// becomes queryable is acceptable for a landscape proxy.
			Tile->bUseAsyncCooking = true;
			Tile->bDeferCollisionUpdates = true;
			Tile->SetCollisionProfileName(CollisionProfileName);
			Tile->SetCollisionObjectType(ECC_WorldStatic);
			Tile->SetCollisionResponseToAllChannels(ECR_Block);
			Tile->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// Visibility is driven by bDebugDrawCollision (off by default = pure physics proxy).
			ApplyDebugDrawSettings(Tile);

			// Attach each tile DMC directly to this landscape component instead of the owner's root,
			// so the collision proxy follows the landscape in the scene hierarchy.
			Tile->SetupAttachment(this);
			Tile->RegisterComponent();
			TileComponents.Add(Tile);
		}
	}

	DirtyTiles.Init(false, TotalTiles);
}


void UXkLandscapeComponent::ReleaseTileComponents()
{
	for (TObjectPtr<UDynamicMeshComponent>& Tile : TileComponents)
	{
		if (Tile)
		{
			Tile->DestroyComponent();
		}
	}
	TileComponents.Reset();
	DirtyTiles.Reset();
}


void UXkLandscapeComponent::SetCanvasRenderer(UXkCanvasRendererComponent* InRenderer)
{
	// Unsubscribe from the previous renderer, if any.
	if (CanvasRenderer.IsValid() && HeightFieldRebuiltHandle.IsValid())
	{
		CanvasRenderer->OnCPUHeightFieldRebuilt.Remove(HeightFieldRebuiltHandle);
	}
	HeightFieldRebuiltHandle.Reset();

	CanvasRenderer = InRenderer;
	if (!InRenderer)
	{
		return;
	}

	// Subscribe to the renderer's data-ready notification. When the CPU heightfield is
	// (re)built, we mark everything dirty and seed our OWN multi-frame queue. The renderer
	// keeps running its GPU pipeline in parallel; the two schedulers are fully decoupled.
	TWeakObjectPtr<UXkLandscapeComponent> WeakSelf(this);
	HeightFieldRebuiltHandle = InRenderer->OnCPUHeightFieldRebuilt.AddLambda([WeakSelf]()
	{
		if (UXkLandscapeComponent* Self = WeakSelf.Get())
		{
			Self->MarkAllTilesDirty();
			Self->EnqueueDirtyTileTasks();
		}
	});

	// If the heightfield is already valid when we bind (late subscriber), seed immediately.
	if (InRenderer->IsCPUHeightFieldValid())
	{
		MarkAllTilesDirty();
		EnqueueDirtyTileTasks();
	}
}


void UXkLandscapeComponent::ApplyDebugDrawSettings(UDynamicMeshComponent* Tile) const
{
	if (!Tile)
	{
		return;
	}
	// Visibility and wireframe overlay are gated by bDebugDrawCollision; by default
	// the collision tiles act as pure physics proxies and render nothing.
	const bool bShow = bDebugDrawCollision;
	Tile->SetVisibility(bShow);
	Tile->SetHiddenInGame(!bShow);
	Tile->bRenderInMainPass = bShow;
	Tile->bRenderInDepthPass = bShow;
	Tile->CastShadow = false;
	Tile->bExplicitShowWireframe = bShow;
	if (bShow && DebugDrawMaterial)
	{
		// Apply override material to every mesh section (DMC supports material index 0).
		Tile->SetMaterial(0, DebugDrawMaterial);
	}
}


void UXkLandscapeComponent::ApplyDebugCollisionBodyHiding(bool bCollisionOn)
{
	// Collision ON -> hide the landscape body so only the collision wireframe is visible.
	// Collision OFF -> show the landscape body again (full reset to "everything visible").
	bDisableLandscapeBody = bCollisionOn;
}

#if WITH_EDITOR
void UXkLandscapeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropName == GET_MEMBER_NAME_CHECKED(UXkLandscapeComponent, bDisableLandscapeBody))
	{
		// Scene proxy copies bDisableLandscapeBody at construction; force a rebuild.
		MarkRenderStateDirty();
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(UXkLandscapeComponent, bDebugDrawCollision))
	{
		// Auto-hide landscape (and water, via virtual hook) so the collision wireframe
		// is the only visible layer. Resetting to OFF unhides them. This drives the
		// scene-proxy bDisable* flags directly rather than SetVisibility so attached
		// DMC tile children are not caught by visibility propagation.
		ApplyDebugCollisionBodyHiding(bDebugDrawCollision);

		// Scene proxy copies bDisable* at construction; rebuild it.
		MarkRenderStateDirty();

		// Refresh every tile's visibility/material.
		for (TObjectPtr<UDynamicMeshComponent>& Tile : TileComponents)
		{
			if (Tile)
			{
				ApplyDebugDrawSettings(Tile);
			}
		}
		// OFF->ON: flush a one-shot sync rebuild so the user sees current heightfield immediately.
		if (bDebugDrawCollision && CanvasRenderer.IsValid() && CanvasRenderer->IsCPUHeightFieldValid())
		{
			MarkAllTilesDirty();
			FlushDirtyTiles();
		}
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(UXkLandscapeComponent, DebugDrawMaterial))
	{
		for (TObjectPtr<UDynamicMeshComponent>& Tile : TileComponents)
		{
			if (Tile)
			{
				ApplyDebugDrawSettings(Tile);
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


void UXkLandscapeComponent::MarkAllTilesDirty()
{
	EnsureTileComponents();
	if (DirtyTiles.Num() > 0)
	{
		DirtyTiles.Init(true, DirtyTiles.Num());
	}
}


void UXkLandscapeComponent::FlushDirtyTiles()
{
	if (!CanvasRenderer.IsValid() || !CanvasRenderer->IsCPUHeightFieldValid())
	{
		return;
	}
	for (int32 Idx = 0; Idx < DirtyTiles.Num(); ++Idx)
	{
		if (DirtyTiles[Idx])
		{
			const int32 TileX = Idx % TileCount.X;
			const int32 TileY = Idx / TileCount.X;
			RebuildTileMesh(TileX, TileY);
			DirtyTiles[Idx] = false;
		}
	}
	// Drop any queued per-tile tasks; they would be no-ops anyway (DirtyTiles[*] cleared above),
	// but clearing keeps Tick latency flat right after a sync flush.
	PendingTileCookTasks.Empty();
}


void UXkLandscapeComponent::EnqueueDirtyTileTasks()
{
	if (!CanvasRenderer.IsValid())
	{
		return;
	}

	// Discard any stale pending work; we will re-seed from the current dirty set.
	PendingTileCookTasks.Empty();

	// Resolve the view anchor (= quadtree root in world space). The quadtree's RootOffset is
	// the point the LOD system considers the camera, so sorting dirty tiles by 2D distance to
	// this anchor gives us a near-to-far cook order. If the scene proxy isn't ready yet
	// (first register, before any render tick), fall back to row-major order.
	FVector2D ViewAnchorXY = FVector2D(GetComponentLocation());
	if (const FXkLandscapeSceneProxy* LocalSceneProxy = static_cast<const FXkLandscapeSceneProxy*>(SceneProxy))
	{
		const FVector QuadtreeWS = GetComponentLocation() + LocalSceneProxy->Quadtree.GetRootOffset();
		ViewAnchorXY = FVector2D(QuadtreeWS.X, QuadtreeWS.Y);
	}

	// Collect dirty tile indices with their squared distance to the view anchor.
	// Squared distance is monotonic with distance and avoids a sqrt per tile.
	const FIntPoint HFSize = CanvasRenderer->GetCPUHeightFieldSize();
	const FVector2D WorldMin = CanvasRenderer->GetCPUHeightFieldWorldMin();
	const FVector2D WorldSize = CanvasRenderer->GetCPUHeightFieldWorldSize();
	const FVector2D PxWorld(WorldSize.X / FMath::Max(HFSize.X, 1), WorldSize.Y / FMath::Max(HFSize.Y, 1));
	const int32 TilePxW = FMath::Max(HFSize.X / FMath::Max(TileCount.X, 1), 1);
	const int32 TilePxH = FMath::Max(HFSize.Y / FMath::Max(TileCount.Y, 1), 1);

	struct FTileOrder
	{
		int32 TileIdx;
		double DistSq;
	};
	TArray<FTileOrder, TInlineAllocator<64>> Ordered;
	Ordered.Reserve(DirtyTiles.Num());

	for (int32 Idx = 0; Idx < DirtyTiles.Num(); ++Idx)
	{
		if (!DirtyTiles[Idx])
		{
			continue;
		}
		const int32 TileX = Idx % TileCount.X;
		const int32 TileY = Idx / TileCount.X;
		// Tile center in world XY (matches the Anchor math used by RebuildTileMesh).
		const int32 Px0 = TileX * TilePxW;
		const int32 Py0 = TileY * TilePxH;
		const int32 Px1 = (TileX == TileCount.X - 1) ? HFSize.X - 1 : FMath::Min(Px0 + TilePxW, HFSize.X - 1);
		const int32 Py1 = (TileY == TileCount.Y - 1) ? HFSize.Y - 1 : FMath::Min(Py0 + TilePxH, HFSize.Y - 1);
		const double CenterX = WorldMin.X + (((double)Px0 + (double)Px1) * 0.5 + 0.5) * PxWorld.X;
		const double CenterY = WorldMin.Y + (((double)Py0 + (double)Py1) * 0.5 + 0.5) * PxWorld.Y;
		const double DX = CenterX - ViewAnchorXY.X;
		const double DY = CenterY - ViewAnchorXY.Y;
		Ordered.Add({ Idx, DX * DX + DY * DY });
	}

	// Ascending: nearest tile to the view anchor cooks first.
	Ordered.Sort([](const FTileOrder& A, const FTileOrder& B) { return A.DistSq < B.DistSq; });

	TWeakObjectPtr<UXkLandscapeComponent> WeakSelf(this);
	for (const FTileOrder& Entry : Ordered)
	{
		const int32 TileIdx = Entry.TileIdx;
		PendingTileCookTasks.Enqueue([WeakSelf, TileIdx]()
		{
			UXkLandscapeComponent* Self = WeakSelf.Get();
			if (!Self || !Self->CanvasRenderer.IsValid() || !Self->CanvasRenderer->IsCPUHeightFieldValid())
			{
				return;
			}
			if (!Self->DirtyTiles.IsValidIndex(TileIdx) || !Self->DirtyTiles[TileIdx])
			{
				// Already handled by a sync flush (e.g. debug-draw toggle).
				return;
			}
			const int32 TileX = TileIdx % Self->TileCount.X;
			const int32 TileY = TileIdx / Self->TileCount.X;
			Self->RebuildTileMesh(TileX, TileY);
			Self->DirtyTiles[TileIdx] = false;
		});
	}
}


void UXkLandscapeComponent::ProcessPendingTileCooks()
{
	if (PendingTileCookTasks.IsEmpty())
	{
		return;
	}

	// MaxTileCooksPerFrame is clamped >= 1 via UPROPERTY meta; no runtime guard needed.
	for (int32 i = 0; i < MaxTileCooksPerFrame; ++i)
	{
		TFunction<void()> Task;
		if (!PendingTileCookTasks.Dequeue(Task))
		{
			break;
		}
		Task();
	}
}


void UXkLandscapeComponent::RebuildTileMesh(int32 TileX, int32 TileY)
{
	using namespace UE::Geometry;

	const int32 TileIdx = TileY * TileCount.X + TileX;
	if (!TileComponents.IsValidIndex(TileIdx))
	{
		return;
	}
	UDynamicMeshComponent* Tile = TileComponents[TileIdx];	if (!Tile || !CanvasRenderer.IsValid() || !CanvasRenderer->IsCPUHeightFieldValid())
	{
		return;
	}

	// Each tile covers a slice of the heightfield grid; we build a per-pixel vertex quad grid.
	const FIntPoint HFSize = CanvasRenderer->GetCPUHeightFieldSize();
	const FVector2D WorldMin = CanvasRenderer->GetCPUHeightFieldWorldMin();
	const FVector2D WorldSize = CanvasRenderer->GetCPUHeightFieldWorldSize();
	const FVector2D PxWorld(WorldSize.X / HFSize.X, WorldSize.Y / HFSize.Y);

	// Tile pixel span. Overlap by 1 pixel on the +X/+Y side so neighboring tiles share edges.
	const int32 TilePxW = FMath::Max(HFSize.X / TileCount.X, 1);
	const int32 TilePxH = FMath::Max(HFSize.Y / TileCount.Y, 1);
	const int32 Px0 = TileX * TilePxW;
	const int32 Py0 = TileY * TilePxH;
	const int32 Px1 = (TileX == TileCount.X - 1) ? HFSize.X - 1 : FMath::Min(Px0 + TilePxW, HFSize.X - 1);
	const int32 Py1 = (TileY == TileCount.Y - 1) ? HFSize.Y - 1 : FMath::Min(Py0 + TilePxH, HFSize.Y - 1);

	// Downsample: skip every N heightfield pixels along X/Y when building the physics trimesh.
	// Always include the last column/row (Px1/Py1) so neighboring tiles share edges exactly.
	const int32 Step = FMath::Max(CollisionDownsample, 1);
	TArray<int32, TInlineAllocator<256>> ColPx; // sampled pixel X coords
	TArray<int32, TInlineAllocator<256>> RowPy; // sampled pixel Y coords
	for (int32 Px = Px0; Px < Px1; Px += Step) { ColPx.Add(Px); }
	ColPx.Add(Px1);
	for (int32 Py = Py0; Py < Py1; Py += Step) { RowPy.Add(Py); }
	RowPy.Add(Py1);

	const int32 Vw = ColPx.Num();
	const int32 Vh = RowPy.Num();
	if (Vw < 2 || Vh < 2)
	{
		return;
	}

	// Tile anchor in world: lower-left of the tile's first pixel center.
	const FVector2D AnchorWorld(
		WorldMin.X + ((double)Px0 + 0.5) * PxWorld.X,
		WorldMin.Y + ((double)Py0 + 0.5) * PxWorld.Y);

	// Place the DMC at AnchorWorld so vertices stay around origin (reduces float precision loss at large coords).
	Tile->SetWorldLocation(FVector(AnchorWorld.X, AnchorWorld.Y, 0.0));
	Tile->SetWorldRotation(FRotator::ZeroRotator);
	Tile->SetWorldScale3D(FVector::OneVector);

	const TArray<float>& HeightZ = CanvasRenderer->GetCPUHeightFieldWorldZ();
	const int32 HFCount = HeightZ.Num();

	// Build the dynamic mesh locally with batch inserts, then hand it off via SetMesh(MoveTemp).
	// This avoids:
	//  - the per-vertex / per-triangle Append* overhead (free-list updates, broadcast churn),
	//  - keeping unused normal/UV overlays alive (we only need triangle group + material id),
	//  - the EditMesh lambda bouncing through the existing UDynamicMesh.
	FDynamicMesh3 LocalMesh;
	LocalMesh.EnableTriangleGroups(0);
	LocalMesh.EnableAttributes();
	// Collision trimesh never samples normals/UVs; drop those overlays to cut allocations.
	LocalMesh.Attributes()->SetNumNormalLayers(0);
	LocalMesh.Attributes()->SetNumUVLayers(0);
	LocalMesh.Attributes()->EnableMaterialID();
	FDynamicMeshMaterialAttribute* MaterialIDs = LocalMesh.Attributes()->GetMaterialID();

	const int32 NumTris = (Vw - 1) * (Vh - 1) * 2;

	// Batch-insert vertices with contiguous IDs [0..Vw*Vh).
	LocalMesh.BeginUnsafeVerticesInsert();
	for (int32 j = 0; j < Vh; ++j)
	{
		const int32 Py = RowPy[j];
		const double RowOffsetY = ((double)Py + 0.5) * PxWorld.Y + WorldMin.Y - AnchorWorld.Y;
		const int32 RowBase = Py * HFSize.X;
		for (int32 i = 0; i < Vw; ++i)
		{
			const int32 Px = ColPx[i];
			const double Lx = ((double)Px + 0.5) * PxWorld.X + WorldMin.X - AnchorWorld.X;
			const int32 HFIdx = RowBase + Px;
			// HFIdx is guaranteed in-range: Px/Py come from clamped tile spans and HeightZ
			// is sized HFSize.X * HFSize.Y by the renderer.
			const double Lz = (HFIdx >= 0 && HFIdx < HFCount) ? (double)HeightZ[HFIdx] : 0.0;
			FVertexInfo VInfo;
			VInfo.Position = FVector3d(Lx, RowOffsetY, Lz);
			LocalMesh.InsertVertex(j * Vw + i, VInfo, /*bUnsafe=*/true);
		}
	}
	LocalMesh.EndUnsafeVerticesInsert();

	// Batch-insert triangles with contiguous IDs [0..NumTris).
	LocalMesh.BeginUnsafeTrianglesInsert();
	int32 TID = 0;
	for (int32 j = 0; j < Vh - 1; ++j)
	{
		for (int32 i = 0; i < Vw - 1; ++i)
		{
			const int32 A = (j + 0) * Vw + (i + 0);
			const int32 B = (j + 0) * Vw + (i + 1);
			const int32 C = (j + 1) * Vw + (i + 1);
			const int32 D = (j + 1) * Vw + (i + 0);
			// Flipped winding so normals point +Z (up).
			LocalMesh.InsertTriangle(TID, FIndex3i(A, C, B), /*GroupID=*/0, /*bUnsafe=*/true);
			MaterialIDs->SetValue(TID, 0);
			++TID;
			LocalMesh.InsertTriangle(TID, FIndex3i(A, D, C), /*GroupID=*/0, /*bUnsafe=*/true);
			MaterialIDs->SetValue(TID, 0);
			++TID;
		}
	}
	LocalMesh.EndUnsafeTrianglesInsert();
	ensureMsgf(TID == NumTris, TEXT("Triangle count mismatch: %d vs %d"), TID, NumTris);

	// Hand the built mesh to the DMC. SetMesh(FDynamicMesh3&&) moves storage in-place,
	// rewires delegates, and broadcasts the change so render/collision pick up new geometry.
	UDynamicMesh* DynamicMesh = Tile->GetDynamicMesh();
	if (!DynamicMesh || !IsValid(DynamicMesh))
	{
		DynamicMesh = NewObject<UDynamicMesh>(Tile);
	}
	DynamicMesh->SetMesh(MoveTemp(LocalMesh));
	Tile->SetDynamicMesh(DynamicMesh);

	// Configure BodySetup the same way the Fortress component does, then update collision.
	if (UBodySetup* BodySetup = Tile->GetBodySetup())
	{
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		BodySetup->bMeshCollideAll = true;
	}
	// bOnlyIfPending=true pairs with bDeferCollisionUpdates=true: the actual trimesh cook
	// runs on a worker thread (bUseAsyncCooking=true) and reports back when ready.
	Tile->UpdateCollision(/*bOnlyIfPending=*/true);
}


UXkLandscapeWithWaterComponent::UXkLandscapeWithWaterComponent(const FObjectInitializer& ObjectInitializer)
	: UXkLandscapeComponent(ObjectInitializer)
	, WaterWavesAsset(nullptr)
	, TargetWaveMaskDepth(2048.0)
{
}

#if WITH_EDITOR
void UXkLandscapeWithWaterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(UXkLandscapeWithWaterComponent, bDisableWaterBody))
	{
		// Scene proxy copies bDisableWaterBody at construction; force a rebuild.
		MarkRenderStateDirty();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UXkLandscapeWithWaterComponent::ApplyDebugCollisionBodyHiding(bool bCollisionOn)
{
	Super::ApplyDebugCollisionBodyHiding(bCollisionOn);
	// Also drive the water body visibility in lockstep with landscape.
	bDisableWaterBody = bCollisionOn;
}


FMaterialRelevance UXkLandscapeWithWaterComponent::GetWaterMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= WaterMaterial->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkLandscapeWithWaterComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (Material)
	{
		OutMaterials.Add(Material);
	}
	if (WaterMaterial)
	{
		OutMaterials.Add(WaterMaterial);
	}
}


FPrimitiveSceneProxy* UXkSphericalLandscapeWithWaterComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* LocalSceneProxy = NULL;
	if (Material && WaterMaterial)
	{
		FPrimitiveSceneProxy* Proxy = new FXkSphericalLandscapeWithWaterSceneProxy(
			this, NAME_None, Material->GetRenderProxy(), WaterMaterial->GetRenderProxy());
		LocalSceneProxy = Proxy;
	}
	return LocalSceneProxy;
}