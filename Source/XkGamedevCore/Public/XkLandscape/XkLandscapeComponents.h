// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "XkLandscapeComponents.generated.h"


class UDynamicMeshComponent;
class UXkCanvasRendererComponent;


UCLASS(BlueprintType, Blueprintable, ShowCategories = (VirtualTexture), ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkQuadtreeComponent"))
class XKGAMEDEVCORE_API UXkQuadtreeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkLandscapeComponent"))
class XKGAMEDEVCORE_API UXkLandscapeComponent : public UXkQuadtreeComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface *Material;

	/* Tile grid size for the CPU-heightfield physics proxy. 5x5 = 25 tiles. */
	UPROPERTY(EditAnywhere, Category = "LandscapeCollision [KEVINTSUIXUGAMEDEV]")
	FIntPoint TileCount = FIntPoint(5, 5);

	/* Max tiles cooked per tick. Keep small to avoid frame spikes. */
	UPROPERTY(EditAnywhere, Category = "LandscapeCollision [KEVINTSUIXUGAMEDEV]", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxTileCooksPerFrame = 1;

	/* Downsample factor for the physics trimesh: 1 = sample every heightfield pixel (highest fidelity, slowest cook),
	 * 2/4/8 = skip pixels (collision vertex count drops quadratically, cook time drops roughly quadratically).
	 * Players usually can't feel the difference at 4; defaults to 4 for a ~16x cook-cost reduction vs raw. */
	UPROPERTY(EditAnywhere, Category = "LandscapeCollision [KEVINTSUIXUGAMEDEV]", meta = (ClampMin = "1", UIMin = "1", ClampMax = "16", UIMax = "16"))
	int32 CollisionDownsample = 8;

	/* Collision profile applied to every tile component. */
	UPROPERTY(EditAnywhere, Category = "LandscapeCollision [KEVINTSUIXUGAMEDEV]")
	FName CollisionProfileName = FName(TEXT("BlockAll"));

	/* Optional override material used when bDebugDrawCollision is enabled. If null, DMC default material is used. */
	UPROPERTY(EditAnywhere, Category = "LandscapeCollision [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UMaterialInterface> DebugDrawMaterial = nullptr;

	UXkLandscapeComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual void PostLoad() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UPrimitiveComponent interface

	virtual void FetchPatchData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);

	/* Hide the landscape body (scene-proxy skips landscape mesh draw).
	 * Scene proxy copies this at construction time, so PostEditChangeProperty
	 * triggers MarkRenderStateDirty to rebuild the proxy on toggle. */
	UPROPERTY(EditAnywhere, Category = "LandscapeDebugger [KEVINTSUIXUGAMEDEV]")
	bool bDisableLandscapeBody = false;

	/* Show the collision tiles as wireframe overlays (uses DebugDrawMaterial if set).
	 * Read on the game thread by ApplyDebugDrawSettings; PostEditChangeProperty
	 * refreshes every tile and, on OFF->ON, flushes a one-shot sync rebuild so the
	 * current heightfield is visible immediately. */
	UPROPERTY(EditAnywhere, Category = "LandscapeDebugger [KEVINTSUIXUGAMEDEV]")
	bool bDebugDrawCollision = false;

	/* Set (or clear with nullptr) the canvas renderer this landscape sources its CPU
	 * heightfield from, and auto-subscribe to its OnCPUHeightFieldRebuilt notification.
	 * Public because the owning actor (not a subclass) wires this up at construction time. */
	void SetCanvasRenderer(UXkCanvasRendererComponent* InRenderer);

protected:
	//////////////////////////////////////////////////////////////////////////
	// Collision (CPU-heightfield tile grid)

	/* Mark every tile for a full rebuild (next ticks will drain the queue). */
	void MarkAllTilesDirty();

	/* Immediately (synchronously) rebuild every tile currently flagged dirty.
	 * Useful for editor-time preview buttons where you don't want to wait for Tick to drain the queue. */
	void FlushDirtyTiles();

	/* Ensure tile components exist and are configured (call on register / after TileCount change). */
	void EnsureTileComponents();

	/* Release all tile components. */
	void ReleaseTileComponents();

	/* Fully rebuild a single tile's mesh from the renderer's CPU heightfield. */
	void RebuildTileMesh(int32 TileX, int32 TileY);

	/* Apply current debug-draw visibility/material to a single tile. */
	void ApplyDebugDrawSettings(class UDynamicMeshComponent* Tile) const;

	/* Hook called whenever bDebugDrawCollision toggles, to hide/unhide the landscape (and in
	 * the water subclass, the water body) so the collision wireframe is the only visible layer.
	 * Base implementation drives bDisableLandscapeBody; subclasses extend for extra bodies.
	 * Note: we drive the scene-proxy bDisable* flags directly (not SetVisibility) so the
	 * DMC child components attached to this landscape component are never hidden by propagation. */
	virtual void ApplyDebugCollisionBodyHiding(bool bCollisionOn);

	/* Fill PendingTileCookTasks with one cook-task per currently-dirty tile.
	 * Safe to call repeatedly; existing tasks are cleared first. */
	void EnqueueDirtyTileTasks();

	/* Drain up to MaxTileCooksPerFrame tasks from PendingTileCookTasks. Decoupled so Tick stays slim. */
	void ProcessPendingTileCooks();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UXkCanvasRendererComponent> CanvasRenderer;

	/* Row-major, size TileCount.X * TileCount.Y. Transient, never serialized. */
	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<UDynamicMeshComponent>> TileComponents;

	/* Row-major dirty flags. */
	TBitArray<> DirtyTiles;

	/* Self-owned frame-paced task queue. One task = one tile cook; TickComponent drains
	 * up to MaxTileCooksPerFrame tasks per frame. Decoupled from the canvas renderer's
	 * GPU-side scheduler so the two pipelines advance in parallel.
	 * SPSC: enqueued on game thread (OnCPUHeightFieldRebuilt), dequeued on game thread (Tick). */
	TQueue<TFunction<void()>, EQueueMode::Spsc> PendingTileCookTasks;

	/* Handle for our subscription to CanvasRenderer->OnCPUHeightFieldRebuilt. */
	FDelegateHandle HeightFieldRebuiltHandle;
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkLandscapeWithWaterComponent"))
class XKGAMEDEVCORE_API UXkLandscapeWithWaterComponent : public UXkLandscapeComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]")
	class UWaterWavesAsset* WaterWavesAsset;

	/** Water depth at which waves start being attenuated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]", meta = (UIMin = 0, ClampMin = 0, UIMax = 10000.0))
	float TargetWaveMaskDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* WaterMaterial;

	/* Hide the water body (scene-proxy skips water mesh draw).
	 * Scene proxy copies this at construction time; PostEditChangeProperty
	 * triggers MarkRenderStateDirty to rebuild the proxy on toggle. */
	UPROPERTY(EditAnywhere, Category = "LandscapeDebugger [KEVINTSUIXUGAMEDEV]")
	bool bDisableWaterBody = false;

	UXkLandscapeWithWaterComponent(const FObjectInitializer& ObjectInitializer);

	FMaterialRelevance GetWaterMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;

	//~ Begin UPrimitiveComponent interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UPrimitiveComponent interface

protected:
	//~ Begin UXkLandscapeComponent interface
	virtual void ApplyDebugCollisionBodyHiding(bool bCollisionOn) override;
	//~ End UXkLandscapeComponent interface
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkSphericalLandscapeComponent"))
class XKGAMEDEVCORE_API UXkSphericalLandscapeWithWaterComponent : public UXkLandscapeWithWaterComponent
{
	GENERATED_BODY()
public:
	//~ Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent interface
};