// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneView.h"
#include "GerstnerWaterWaveViewExtension.h"
#include "GerstnerWaterWaveSubsystem.h"
#include "Subsystems/EngineSubsystem.h"
#include "XkGerstnerWaterWave.generated.h"

// FXkWaveGPUResources
struct FXkWaveGPUResources
{
	FBufferRHIRef DataBuffer;
	FShaderResourceViewRHIRef DataSRV;

	FBufferRHIRef IndirectionBuffer;
	FShaderResourceViewRHIRef IndirectionSRV;
};

// FXkGerstnerWaterWaveViewExtension
class FXkGerstnerWaterWaveViewExtension : public FWorldSceneViewExtension
{
public:
	FXkGerstnerWaterWaveViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld);
	~FXkGerstnerWaterWaveViewExtension();

	void Initialize();
	void Deinitialize();

	// FSceneViewExtensionBase implementation : 
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {};
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {};
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
	virtual void PreInitViews_RenderThread(FRDGBuilder& GraphBuilder) override;

	bool bRebuildGPUData = false;

	TSharedRef<FXkWaveGPUResources, ESPMode::ThreadSafe> WaveGPUData;
private:
	TArray<TWeakObjectPtr<class UXkLandscapeWithWaterComponent>> Components;
	TArray<FSceneView*> SceneViews;
};


// UGerstnerWaterWaveSubsystem
/**
 * This is the API used to get information about water at runtime
 */
UCLASS(BlueprintType, Transient)
class XKGAMEDEVCORE_API UXkGerstnerWaterWaveSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UXkGerstnerWaterWaveSubsystem();

	// FTickableGameObject implementation Begin
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject implementation End

	// UWorldSubsystem implementation Begin
	/** Override to support water subsystems in editor preview worlds */
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	// UWorldSubsystem implementation End

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	void RebuildGPUData() { bRebuildGPUData = true; }

private:
	void BeginFrameCallback();

private:
	TSharedPtr<FXkGerstnerWaterWaveViewExtension, ESPMode::ThreadSafe> GerstnerWaterWaveViewExtension;
	bool bRebuildGPUData = true;
};