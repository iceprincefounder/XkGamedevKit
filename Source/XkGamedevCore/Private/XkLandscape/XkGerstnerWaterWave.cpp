// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkLandscape/XkGerstnerWaterWave.h"
#include "XkLandscape/XkLandscapeComponents.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "WaterBodyComponent.h"
#include "WaterBodyManager.h"
#include "XkGameWorld.h"


FXkGerstnerWaterWaveViewExtension::FXkGerstnerWaterWaveViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld) : FWorldSceneViewExtension(AutoReg, InWorld), WaveGPUData(MakeShared<FXkWaveGPUResources, ESPMode::ThreadSafe>())
{
}


FXkGerstnerWaterWaveViewExtension::~FXkGerstnerWaterWaveViewExtension()
{
}


void FXkGerstnerWaterWaveViewExtension::Initialize()
{
	for (TObjectIterator<UXkLandscapeWithWaterComponent> It; It; ++It)
	{
		UXkLandscapeWithWaterComponent* Component = *It;
		UWorld* TargetWorld = Component->GetWorld();
		if (TargetWorld != nullptr && IsValid(TargetWorld))
		{
			Components.Add(Component);
		}
	}
}


void FXkGerstnerWaterWaveViewExtension::Deinitialize()
{
	ENQUEUE_RENDER_COMMAND(DeallocateWaterInstanceDataBuffer)
		(
			// Copy the shared ptr into a local copy for this lambda, this will increase the ref count and keep it alive on the renderthread until this lambda is executed
			[WaveGPUData = WaveGPUData](FRHICommandListImmediate& RHICmdList) {
			} 
			);
	Components.Empty();
}


void FXkGerstnerWaterWaveViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	if (bRebuildGPUData)
	{
		TResourceArray<FVector4f> WaterDataBuffer;
		TResourceArray<FVector4f> WaterIndirectionBuffer;

		const TWeakObjectPtr<UWorld> WorldPtr = GetWorld();
		check(WorldPtr.IsValid())

		for (TWeakObjectPtr<UXkLandscapeWithWaterComponent> WaterComponent : Components)
		{
			// Some max value
			constexpr int32 MaxWavesPerWaterBody = 4096;
			constexpr int32 NumFloat4PerWave = 2;

			WaterIndirectionBuffer.AddZeroed();

			if (WaterComponent.IsValid() && WaterComponent->WaterWavesAsset)
			{
				const UWaterWavesBase* WaterWavesBase = WaterComponent->WaterWavesAsset->GetWaterWaves();
				check(WaterWavesBase != nullptr);
				if (const UGerstnerWaterWaves* GerstnerWaves = Cast<const UGerstnerWaterWaves>(WaterWavesBase->GetWaterWaves()))
				{
					const TArray<FGerstnerWave>& Waves = GerstnerWaves->GetGerstnerWaves();

					// Where the data for this water body starts (including header)
					const int32 DataBaseIndex = WaterDataBuffer.Num();
					// Allocate for the waves in this water body
					const int32 NumWaves = FMath::Min(Waves.Num(), MaxWavesPerWaterBody);
					WaterDataBuffer.AddZeroed(NumWaves * NumFloat4PerWave);

					// The header is a vector4 and contains generic per-water body information
					// X: Index to the wave data
					// Y: Num waves
					// Z: TargetWaveMaskDepth
					// W: Unused
					FVector4f& Header = WaterIndirectionBuffer.Last();
					Header.X = DataBaseIndex;
					Header.Y = NumWaves;
					Header.Z = WaterComponent->TargetWaveMaskDepth;
					Header.W = 0.0f;

					for (int32 i = 0; i < NumWaves; i++)
					{
						const FGerstnerWave& Wave = Waves[i];

						const int32 WaveIndex = DataBaseIndex + (i * NumFloat4PerWave);

						WaterDataBuffer[WaveIndex] = FVector4f(Wave.Direction.X, Wave.Direction.Y, Wave.WaveLength, Wave.Amplitude);
						WaterDataBuffer[WaveIndex + 1] = FVector4f(Wave.Steepness, 0.0f, 0.0f, 0.0f);
					}
				}
			}
		}

		if (WaterIndirectionBuffer.Num() == 0)
		{
			WaterIndirectionBuffer.AddZeroed();
		}

		if (WaterDataBuffer.Num() == 0)
		{
			WaterDataBuffer.AddZeroed();
		}

		ENQUEUE_RENDER_COMMAND(AllocateXkWaterInstanceDataBuffer)
			(
				[WaveGPUData = WaveGPUData, WaterIndirectionBuffer, WaterDataBuffer](FRHICommandListImmediate& RHICmdList) mutable
				{
					FRHIResourceCreateInfo CreateInfoData(TEXT("WaterDataBuffer"), &WaterDataBuffer);
					WaveGPUData->DataBuffer = RHICreateBuffer(WaterDataBuffer.GetResourceDataSize(), BUF_VertexBuffer | BUF_ShaderResource | BUF_Static, sizeof(FVector4f), ERHIAccess::SRVMask, CreateInfoData);
					WaveGPUData->DataSRV = RHICreateShaderResourceView(WaveGPUData->DataBuffer, sizeof(FVector4f), PF_A32B32G32R32F);

					FRHIResourceCreateInfo CreateInfoIndirection(TEXT("WaterIndirectionBuffer"), &WaterIndirectionBuffer);
					WaveGPUData->IndirectionBuffer = RHICreateBuffer(WaterIndirectionBuffer.GetResourceDataSize(), BUF_VertexBuffer | BUF_ShaderResource | BUF_Static, sizeof(FVector4f), ERHIAccess::SRVMask, CreateInfoIndirection);
					WaveGPUData->IndirectionSRV = RHICreateShaderResourceView(WaveGPUData->IndirectionBuffer, sizeof(FVector4f), PF_A32B32G32R32F);
				}
				);

		bRebuildGPUData = false;
	}
}


void FXkGerstnerWaterWaveViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	SceneViews.Add(&InView);
	// FGerstnerWaterWaveViewExtension::PreRenderView_RenderThread(...)
	// This would be override by Water plugin, so I record it and submit it at PreInitViews_RenderThread
	//if (WaveGPUData->DataSRV && WaveGPUData->IndirectionSRV)
	//{
	//	InView.WaterDataBuffer = WaveGPUData->DataSRV;
	//	InView.WaterIndirectionBuffer = WaveGPUData->IndirectionSRV;
	//}
}


void FXkGerstnerWaterWaveViewExtension::PreInitViews_RenderThread(FRDGBuilder& GraphBuilder)
{
	for (FSceneView* SceneView : SceneViews)
	{
		SceneView->WaterDataBuffer = WaveGPUData->DataSRV;
		SceneView->WaterIndirectionBuffer = WaveGPUData->IndirectionSRV;
	}
	SceneViews.Reset();
}


UXkGerstnerWaterWaveSubsystem::UXkGerstnerWaterWaveSubsystem()
{
}


void UXkGerstnerWaterWaveSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


TStatId UXkGerstnerWaterWaveSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UXkGerstnerWaterWaveSubsystem, STATGROUP_Tickables);
}



bool UXkGerstnerWaterWaveSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
#if WITH_EDITOR
	// In editor, don't let preview worlds instantiate a water subsystem (except if explicitly allowed by a tool that requested it by setting bAllowWaterSubsystemOnPreviewWorld)
	if (WorldType == EWorldType::EditorPreview)
	{
		return false;
	}
#endif // WITH_EDITOR

	return WorldType == EWorldType::Game || WorldType == EWorldType::Editor || WorldType == EWorldType::PIE;
}


void UXkGerstnerWaterWaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GetWorld() != nullptr)
	{
		GerstnerWaterWaveViewExtension = FSceneViewExtensions::NewExtension<FXkGerstnerWaterWaveViewExtension>(GetWorld());
		GerstnerWaterWaveViewExtension->Initialize();
	}

	FCoreDelegates::OnBeginFrame.AddUObject(this, &UXkGerstnerWaterWaveSubsystem::BeginFrameCallback);
}


void UXkGerstnerWaterWaveSubsystem::Deinitialize()
{
	GerstnerWaterWaveViewExtension->Deinitialize();
	GerstnerWaterWaveViewExtension.Reset();

	FCoreDelegates::OnBeginFrame.RemoveAll(this);

	Super::Deinitialize();
}


void UXkGerstnerWaterWaveSubsystem::BeginFrameCallback()
{
	// In case there was a change, all registered view extensions need to update their GPU data : 
	if (bRebuildGPUData && GerstnerWaterWaveViewExtension)
	{
		GerstnerWaterWaveViewExtension->bRebuildGPUData = true;
	}
	bRebuildGPUData = false;
}