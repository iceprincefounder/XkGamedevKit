// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkRenderer/XkRendererComponents.h"
#include "XkRenderer/XkRendererRenderUtils.h"

#include "Engine/TextureRenderTarget2D.h"
#include "MathUtil.h"
#include "RHIStaticStates.h"
#include "UObject/ObjectSaveContext.h"
#include "RenderGraphUtils.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RenderTargetPool.h"
#include "RenderCaptureInterface.h"
#include "RenderGraphDefinitions.h"
#include "ShaderParameterMetadata.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "HairStrandsInterface.h"


static bool CaptureDrawCanvas = 0;
static FAutoConsoleVariableRef CVarCaptureDrawCanvas(
	TEXT("r.xk.CaptureDrawCanvas"),
	CaptureDrawCanvas,
	TEXT("Render Capture Next Canvas Draw"));

UXkCanvasRendererComponent::UXkCanvasRendererComponent(const FObjectInitializer& ObjectInitializer) :
	UActorComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bTickInEditor = true;

	ConvolutionRangeX = 2;
	ConvolutionRangeY = 2;
	ConvolutionRangeZ = 4;
	ConvolutionRangeW = 4;

	CanvasCenter = FVector4f::Zero();
	CanvasCenter.W = 200.0f; // CanvasCenter.W for land height range, to calculate ocean SDF
	CanvasExtent = FVector4f(204800.0/*WorldSize.X*/, 204800.0/*WorldSize.Y*/, -2048.0/*HeightRange MinZ*/, 2048.0/*HeightRange MaxZ*/);
	HorizonHeight = 100.0;

	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> ObjectFinder(TEXT("/XkGamedevKit/RenderTargets/RT_SphericalLandscapeHeight"));
	CanvasRT0 = ObjectFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> ObjectFinder2(TEXT("/XkGamedevKit/RenderTargets/RT_SphericalLandscapeWeight"));
	CanvasRT1 = ObjectFinder2.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> ObjectFinder3(TEXT("/XkGamedevKit/Materials/MPC_CanvasRender"));
	CanvasMPC = ObjectFinder3.Object;
}


UXkCanvasRendererComponent::~UXkCanvasRendererComponent()
{
	VertexBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	InstancePositionBuffer.ReleaseResource();
	InstanceWeightBuffer.ReleaseResource();
}


void UXkCanvasRendererComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ProcessPendingDrawTasks();
}


void UXkCanvasRendererComponent::ProcessPendingDrawTasks()
{
	if (!IsRenderDataValid() || PendingDrawTasks.IsEmpty())
	{
		return;
	}

	// One task per frame: each step enqueues a render command that must complete
	// before the next step runs. Pacing by frame also gives the RHI time to flush.
	TFunction<void()> Task;
	if (PendingDrawTasks.Dequeue(Task))
	{
		Task();
	}
}


FVector2D UXkCanvasRendererComponent::GetCanvasSize() const
{
	if (CanvasRT0)
	{
		uint32 X = CanvasRT0->GetResource()->GetSizeX();
		uint32 Y = CanvasRT0->GetResource()->GetSizeY();
		return FVector2D(X, Y);
	}
	else if (CanvasRT1)
	{
		uint32 X = CanvasRT1->GetResource()->GetSizeX();
		uint32 Y = CanvasRT1->GetResource()->GetSizeY();
		return FVector2D(X, Y);
	}
	return FVector2D(1);
}


bool UXkCanvasRendererComponent::IsBuffersValid() const
{
	return (VertexBuffer.IsInitialized() && IndexBuffer.IsInitialized() && 
		InstancePositionBuffer.IsInitialized() && InstanceWeightBuffer.IsInitialized());
}


bool UXkCanvasRendererComponent::IsRenderDataValid() const
{
	return (CanvasRT0 && CanvasRT1
		&& InstancePositionBuffer.GetInstanceNum() > 0 
		&& InstanceWeightBuffer.GetInstanceNum() > 0);
}


void UXkCanvasRendererComponent::CreateBuffers(const TArray<FVector4f> Vertices, const TArray<uint32> Indices, const TArray<FVector4f> Positions, const TArray<FVector4f> Weights)
{
	VertexBuffer.Positions = Vertices;
	TArray<FVector2f> UVs; UVs.Init(FVector2f(), Vertices.Num());
	VertexBuffer.UVs = UVs;
	IndexBuffer.Indices = Indices;

	InstancePositionBuffer.Data = Positions;
	InstanceWeightBuffer.Data = Weights;

	ENQUEUE_RENDER_COMMAND(UXkCanvasRendererComponent_CreateBuffers)([&]
	(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_CreateBuffers);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
			VertexBuffer.InitRHI();
			IndexBuffer.InitRHI();
			InstancePositionBuffer.InitRHI();
			InstanceWeightBuffer.InitRHI();
#else
			VertexBuffer.InitRHI(RHICmdList);
			IndexBuffer.InitRHI(RHICmdList);
			InstancePositionBuffer.InitRHI(RHICmdList);
			InstanceWeightBuffer.InitRHI(RHICmdList);
#endif
		});
	FlushRenderingCommands();

	if (CanvasMPC && GetWorld())
	{
		UMaterialParameterCollectionInstance* MPC_Instance = GetWorld()->GetParameterCollectionInstance(CanvasMPC);
		MPC_Instance->SetScalarParameterValue(FName(TEXT("HorizonHeight")), HorizonHeight);
		MPC_Instance->SetVectorParameterValue(FName(TEXT("Center")), FLinearColor(CanvasCenter.X, CanvasCenter.Y, CanvasCenter.Z, CanvasCenter.W));
		MPC_Instance->SetVectorParameterValue(FName(TEXT("Extent")), FLinearColor(CanvasExtent.X, CanvasExtent.Y, CanvasExtent.Z, CanvasExtent.W));
	}
}


void UXkCanvasRendererComponent::UpdateBuffers(const TArray<FVector4f> Positions, const TArray<FVector4f> Weights)
{
	if (Positions.IsEmpty() || Weights.IsEmpty())
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(UXkCanvasRendererComponent_UpdateBuffers)([&]
	(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_UpdateBuffers);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
			/** instance position data */
			void* RawInstancePositionData = RHILockBuffer(
				InstancePositionBuffer.VertexBufferRHI, 0,
				InstancePositionBuffer.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstancePositionData, Positions.GetData(), Positions.Num() * sizeof(FVector4f));
			RHIUnlockBuffer(InstancePositionBuffer.VertexBufferRHI);

			/** instance weight data */
			void* RawInstanceWeightData = RHILockBuffer(
				InstanceWeightBuffer.VertexBufferRHI, 0,
				InstanceWeightBuffer.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstanceWeightData, Weights.GetData(), Weights.Num() * sizeof(FVector4f));
			RHIUnlockBuffer(InstanceWeightBuffer.VertexBufferRHI);
#else
			/** instance position data */
			void* RawInstancePositionData = RHICmdList.LockBuffer(
				InstancePositionBuffer.VertexBufferRHI, 0,
				InstancePositionBuffer.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstancePositionData, Positions.GetData(), Positions.Num() * sizeof(FVector4f));
			RHICmdList.UnlockBuffer(InstancePositionBuffer.VertexBufferRHI);

			/** instance weight data */
			void* RawInstanceWeightData = RHICmdList.LockBuffer(
				InstanceWeightBuffer.VertexBufferRHI, 0,
				InstanceWeightBuffer.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstanceWeightData, Weights.GetData(), Weights.Num() * sizeof(FVector4f));
			RHICmdList.UnlockBuffer(InstanceWeightBuffer.VertexBufferRHI);
#endif
		});
}

void UXkCanvasRendererComponent::RebuildCPUHeightField(
	const TArray<FVector4f>& Vertices,
	const TArray<uint32>& Indices,
	const TArray<FVector4f>& InstancePositions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_RebuildCPUHeightField);
	// Owner's actor transform (match GPU vertex shader: TranslatedWorldPosition = Position + InstancePos, then LocalToWorld).
	FMatrix44f LocalToWorld = FMatrix44f::Identity;
	if (AActor* Owner = GetOwner())
	{
		LocalToWorld = FMatrix44f(Owner->GetTransform().ToMatrixWithScale());
	}

	// World-space canvas footprint, same convention as CanvasExtent.xy (full world size centered at CanvasCenter.xy).
	const FVector2D WorldSize(CanvasExtent.X, CanvasExtent.Y);
	const FVector2D WorldMin(CanvasCenter.X - WorldSize.X * 0.5, CanvasCenter.Y - WorldSize.Y * 0.5);

	const FIntPoint Size = CPUHeightFieldResolution;
	if (Size.X <= 0 || Size.Y <= 0 || WorldSize.X <= 0.0 || WorldSize.Y <= 0.0)
	{
		CPUHeightFieldWorldZ.Reset();
		CPUHeightFieldSize = FIntPoint(0, 0);
		return;
	}

	const FVector2D PixelSize(WorldSize.X / (double)Size.X, WorldSize.Y / (double)Size.Y);
	CPUHeightFieldSize = Size;
	CPUHeightFieldWorldMin = WorldMin;
	CPUHeightFieldWorldSize = WorldSize;
	CPUHeightFieldPixelSize = PixelSize;

	const int32 NumPixels = Size.X * Size.Y;
	TArray<float> Raw;
	Raw.SetNumUninitialized(NumPixels);
	for (int32 i = 0; i < NumPixels; ++i)
	{
		Raw[i] = CPUHeightFieldDefaultWorldZ;
	}

	// 2D bounding box of hex local vertices in XY (BuildHexagon generates hexagon prism around origin).
	FVector2f HexLocalMin(FLT_MAX, FLT_MAX);
	FVector2f HexLocalMax(-FLT_MAX, -FLT_MAX);
	float HexLocalMaxZ = -FLT_MAX;
	for (const FVector4f& V : Vertices)
	{
		HexLocalMin.X = FMath::Min(HexLocalMin.X, V.X);
		HexLocalMin.Y = FMath::Min(HexLocalMin.Y, V.Y);
		HexLocalMax.X = FMath::Max(HexLocalMax.X, V.X);
		HexLocalMax.Y = FMath::Max(HexLocalMax.Y, V.Y);
		HexLocalMaxZ = FMath::Max(HexLocalMaxZ, V.Z); // top face Z
	}
	if (HexLocalMin.X >= HexLocalMax.X || HexLocalMin.Y >= HexLocalMax.Y)
	{
		// Fall back to a plain hex radius if BuildHexagonData returned empty.
		HexLocalMin = FVector2f(-100.f, -100.f);
		HexLocalMax = FVector2f(100.f, 100.f);
		HexLocalMaxZ = 0.f;
	}

	// Rasterize each instance: write max world Z into pixels whose XY lies inside the hex bbox.
	// (Hex interior is approximated by its bbox; the box blur step smooths the edge rings afterwards.)
	for (const FVector4f& Inst : InstancePositions)
	{
		// Convert local-space hex AABB top-face corners to world-space to account for LocalToWorld rotation/scale.
		const FVector3f LocalCorners[4] =
		{
			FVector3f(Inst.X + HexLocalMin.X, Inst.Y + HexLocalMin.Y, Inst.Z + HexLocalMaxZ),
			FVector3f(Inst.X + HexLocalMax.X, Inst.Y + HexLocalMin.Y, Inst.Z + HexLocalMaxZ),
			FVector3f(Inst.X + HexLocalMin.X, Inst.Y + HexLocalMax.Y, Inst.Z + HexLocalMaxZ),
			FVector3f(Inst.X + HexLocalMax.X, Inst.Y + HexLocalMax.Y, Inst.Z + HexLocalMaxZ)
		};
		FVector3f WorldMinC(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector3f WorldMaxC(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (int32 c = 0; c < 4; ++c)
		{
			const FVector4f W4 = LocalToWorld.TransformPosition(FVector3f(LocalCorners[c]));
			WorldMinC.X = FMath::Min(WorldMinC.X, W4.X);
			WorldMinC.Y = FMath::Min(WorldMinC.Y, W4.Y);
			WorldMinC.Z = FMath::Min(WorldMinC.Z, W4.Z);
			WorldMaxC.X = FMath::Max(WorldMaxC.X, W4.X);
			WorldMaxC.Y = FMath::Max(WorldMaxC.Y, W4.Y);
			WorldMaxC.Z = FMath::Max(WorldMaxC.Z, W4.Z);
		}
		const float HexTopZ = WorldMaxC.Z;

		// Pixel index range (inclusive-exclusive).
		const int32 PxMin = FMath::Clamp((int32)FMath::FloorToFloat((WorldMinC.X - WorldMin.X) / PixelSize.X), 0, Size.X - 1);
		const int32 PxMax = FMath::Clamp((int32)FMath::CeilToFloat((WorldMaxC.X - WorldMin.X) / PixelSize.X), 0, Size.X - 1);
		const int32 PyMin = FMath::Clamp((int32)FMath::FloorToFloat((WorldMinC.Y - WorldMin.Y) / PixelSize.Y), 0, Size.Y - 1);
		const int32 PyMax = FMath::Clamp((int32)FMath::CeilToFloat((WorldMaxC.Y - WorldMin.Y) / PixelSize.Y), 0, Size.Y - 1);

		for (int32 py = PyMin; py <= PyMax; ++py)
		{
			float* RowPtr = Raw.GetData() + py * Size.X;
			for (int32 px = PxMin; px <= PxMax; ++px)
			{
				if (HexTopZ > RowPtr[px])
				{
					RowPtr[px] = HexTopZ;
				}
			}
		}
	}

	// Box blur, same kernel as HEIGHT_FILTER branch in XkRendererCS.usf (square window of radius ConvolutionRangeX, mean filter).
	TArray<float> Blurred;
	Blurred.SetNumUninitialized(NumPixels);
	const int32 Range = FMath::Max((int32)ConvolutionRangeX, 0);
	if (Range == 0)
	{
		Blurred = Raw;
	}
	else
	{
		// Separable 2-pass box blur for performance (two 1D passes = one 2D box blur).
		TArray<float> Temp;
		Temp.SetNumUninitialized(NumPixels);
		// Horizontal pass: Raw -> Temp.
		for (int32 y = 0; y < Size.Y; ++y)
		{
			const float* Src = Raw.GetData() + y * Size.X;
			float* Dst = Temp.GetData() + y * Size.X;
			for (int32 x = 0; x < Size.X; ++x)
			{
				const int32 X0 = FMath::Max(x - Range, 0);
				const int32 X1 = FMath::Min(x + Range, Size.X - 1);
				double Sum = 0.0;
				for (int32 xi = X0; xi <= X1; ++xi)
				{
					Sum += Src[xi];
				}
				Dst[x] = (float)(Sum / (double)(X1 - X0 + 1));
			}
		}
		// Vertical pass: Temp -> Blurred.
		for (int32 x = 0; x < Size.X; ++x)
		{
			for (int32 y = 0; y < Size.Y; ++y)
			{
				const int32 Y0 = FMath::Max(y - Range, 0);
				const int32 Y1 = FMath::Min(y + Range, Size.Y - 1);
				double Sum = 0.0;
				for (int32 yi = Y0; yi <= Y1; ++yi)
				{
					Sum += Temp[yi * Size.X + x];
				}
				Blurred[y * Size.X + x] = (float)(Sum / (double)(Y1 - Y0 + 1));
			}
		}
	}

	CPUHeightFieldWorldZ = MoveTemp(Blurred);
}


float UXkCanvasRendererComponent::SampleWorldZCPU(const FVector2D& WorldXY) const
{
	if (CPUHeightFieldWorldZ.Num() == 0 || CPUHeightFieldSize.X <= 0 || CPUHeightFieldSize.Y <= 0)
	{
		return CPUHeightFieldDefaultWorldZ;
	}

	// Pixel-space coord, pixel center at (i+0.5, j+0.5).
	const double Fx = (WorldXY.X - CPUHeightFieldWorldMin.X) / CPUHeightFieldPixelSize.X - 0.5;
	const double Fy = (WorldXY.Y - CPUHeightFieldWorldMin.Y) / CPUHeightFieldPixelSize.Y - 0.5;

	const int32 X0 = FMath::Clamp((int32)FMath::FloorToDouble(Fx), 0, CPUHeightFieldSize.X - 1);
	const int32 X1 = FMath::Clamp(X0 + 1, 0, CPUHeightFieldSize.X - 1);
	const int32 Y0 = FMath::Clamp((int32)FMath::FloorToDouble(Fy), 0, CPUHeightFieldSize.Y - 1);
	const int32 Y1 = FMath::Clamp(Y0 + 1, 0, CPUHeightFieldSize.Y - 1);

	const float Tx = (float)FMath::Clamp(Fx - (double)X0, 0.0, 1.0);
	const float Ty = (float)FMath::Clamp(Fy - (double)Y0, 0.0, 1.0);

	const float V00 = CPUHeightFieldWorldZ[Y0 * CPUHeightFieldSize.X + X0];
	const float V10 = CPUHeightFieldWorldZ[Y0 * CPUHeightFieldSize.X + X1];
	const float V01 = CPUHeightFieldWorldZ[Y1 * CPUHeightFieldSize.X + X0];
	const float V11 = CPUHeightFieldWorldZ[Y1 * CPUHeightFieldSize.X + X1];

	const float V0 = FMath::Lerp(V00, V10, Tx);
	const float V1 = FMath::Lerp(V01, V11, Tx);
	return FMath::Lerp(V0, V1, Ty);
}


void UXkCanvasRendererComponent::DrawHeightWeightCanvas_MultiFrame(
	const TArray<FVector4f>& Vertices,
	const TArray<uint32>& Indices,
	const TArray<FVector4f>& InstancePositions)
{
	if (!IsRenderDataValid())
	{
		return;
	}

	// Reset the queue (drop any stale pipeline from a previous call).
	PendingDrawTasks.Empty();
	PendingDrawTasks.Enqueue([this]()
		{
			InitCanvasRTCached_Internal();
		});
	PendingDrawTasks.Enqueue([this]()
		{
			DrawPrimaryCanvas_Internal(CanvasRT0_Cached, CanvasRT1_Cached);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			DrawFilteringCanvas_Internal<FXkCanvasRenderHeightCS>(CanvasRT0_Cached, CanvasRT1_Cached, CanvasRT0_Cached);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			DrawFilteringCanvas_Internal<FXkCanvasRenderNormalCS>(CanvasRT0_Cached, CanvasRT1_Cached, CanvasRT0_Cached);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			DrawFilteringCanvas_Internal<FXkCanvasRenderSdfCS>(CanvasRT0_Cached, CanvasRT1_Cached, CanvasRT0_Cached);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			DrawFilteringCanvas_Internal<FXkCanvasRenderWeightCS>(CanvasRT0_Cached, CanvasRT1_Cached, CanvasRT1_Cached);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			CopyCanvasRT_Internal(CanvasRT0_Cached, CanvasRT0);
			CopyCanvasRT_Internal(CanvasRT1_Cached, CanvasRT1);
		});
	PendingDrawTasks.Enqueue([this]()
		{
			// Do nothing, wait for a frame to ensure RT copy finished
		});
	PendingDrawTasks.Enqueue([this]()
		{
			ReleaseCanvasRTCached_Internal();
		});

	// Final step: build the CPU heightfield from the SAME source data used for the GPU draw.
	// Capture arrays by value so the lambda owns its data regardless of caller lifetime.
	PendingDrawTasks.Enqueue([this, Vertices, Indices, InstancePositions]()
		{
			RebuildCPUHeightField(Vertices, Indices, InstancePositions);
			// Pure data-ready notification. Subscribers (e.g. collision component) own their
			// own schedulers and run in parallel with this renderer's GPU workload.
			OnCPUHeightFieldRebuilt.Broadcast();
		});
}


void UXkCanvasRendererComponent::InitCanvasRTCached_Internal()
{
	if (CanvasRT0 && CanvasRT0->GetResource()
		&& CanvasRT1 && CanvasRT1->GetResource())
	{
		if (CanvasRT0_Cached && IsValid(CanvasRT0_Cached) && CanvasRT0_Cached->GetResource()
			&& CanvasRT1_Cached && IsValid(CanvasRT1_Cached) && CanvasRT1_Cached->GetResource())
		{
			// Already initialized
			return;
		}

		// Clone CanvasRT0 to CanvasRT_Cached
		CanvasRT0_Cached = NewObject<UTextureRenderTarget2D>(this);
		CanvasRT0_Cached->RenderTargetFormat = CanvasRT0->RenderTargetFormat;
		CanvasRT0_Cached->ClearColor = CanvasRT0->ClearColor;
		CanvasRT0_Cached->InitAutoFormat(CanvasRT0->SizeX, CanvasRT0->SizeY);
		CanvasRT0_Cached->UpdateResourceImmediate(false);
		CanvasRT0_Cached->SetFlags(RF_Transient);

		CanvasRT1_Cached = NewObject<UTextureRenderTarget2D>(this);
		CanvasRT1_Cached->RenderTargetFormat = CanvasRT1->RenderTargetFormat;
		CanvasRT1_Cached->ClearColor = CanvasRT1->ClearColor;
		CanvasRT1_Cached->InitAutoFormat(CanvasRT1->SizeX, CanvasRT1->SizeY);
		CanvasRT1_Cached->UpdateResourceImmediate(false);
		CanvasRT1_Cached->SetFlags(RF_Transient);
	}
}


void UXkCanvasRendererComponent::ReleaseCanvasRTCached_Internal()
{
	if (CanvasRT0_Cached)
	{
		CanvasRT0_Cached->ReleaseResource();
		CanvasRT0_Cached->MarkAsGarbage();
		CanvasRT0_Cached = nullptr;
	}
	if (CanvasRT1_Cached)
	{
		CanvasRT1_Cached->ReleaseResource();
		CanvasRT1_Cached->MarkAsGarbage();
		CanvasRT1_Cached = nullptr;
	}
}


void UXkCanvasRendererComponent::CopyCanvasRT_Internal(UTextureRenderTarget2D* Input, UTextureRenderTarget2D* Output)
{
	if (Input && Input->GetResource()
		&& Output && Output->GetResource())
	{
		ENQUEUE_RENDER_COMMAND(UXkCanvasRendererComponent_CopyCanvasRTCached)([Input, Output]
		(FRHICommandListImmediate& RHICmdList)
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_CopyCanvasRTCached);

				FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("CaptureDrawCanvas"));

				TRefCountPtr<IPooledRenderTarget> Input_RT = CreateRenderTarget(
					Input->GetResource()->GetTexture2DRHI(), TEXT("Input"));
				FRDGTextureRef Input_RDG = GraphBuilder.RegisterExternalTexture(Input_RT);

				TRefCountPtr<IPooledRenderTarget> Output_RT = CreateRenderTarget(
					Output->GetResource()->GetTexture2DRHI(), TEXT("Output"));
				FRDGTextureRef Output_RDG = GraphBuilder.RegisterExternalTexture(Output_RT);

				FIntVector TextureSize = Input_RDG->Desc.GetSize();
				FRHICopyTextureInfo CopyTextureInfo;
				CopyTextureInfo.NumMips = 1;
				CopyTextureInfo.Size = TextureSize;

				AddCopyTexturePass(GraphBuilder, Input_RDG, Output_RDG, CopyTextureInfo);
				GraphBuilder.Execute();
			});
	}
}


void UXkCanvasRendererComponent::DrawPrimaryCanvas_Internal(UTextureRenderTarget2D* Output0, UTextureRenderTarget2D* Output1)
{
	if (!IsRenderDataValid())
	{
		return;
	}

	UTextureRenderTarget2D* Canvas0 = Output0;
	UTextureRenderTarget2D* Canvas1 = Output1;
	FVector4f Center = CanvasCenter;
	FVector4f Extent = CanvasExtent;
	FXkCanvasVertexBuffer* VertexBuf = &VertexBuffer;
	FXkCanvasIndexBuffer* IndexBuf = &IndexBuffer;
	FXkCanvasInstanceBuffer* InstancePositionBuf = &InstancePositionBuffer;
	FXkCanvasInstanceBuffer* InstanceWeightBuf = &InstanceWeightBuffer;
	FXkCanvasRenderVS::FParameters VertexShaderParamsToCopy;
	FXkCanvasRenderPS::FParameters PixelShaderParamsToCopy;
	FMatrix44f LocalToWorld = FMatrix44f(GetOwner()->GetTransform().ToMatrixWithScale());
	RenderCaptureInterface::FScopedCapture RenderCapture(CaptureDrawCanvas, TEXT("DrawPrimaryCanvas"));
	ENQUEUE_RENDER_COMMAND(UXkRendererComponent_DrawPrimaryCanvas)([Canvas0, Canvas1, LocalToWorld, Center, Extent,
		VertexShaderParamsToCopy, PixelShaderParamsToCopy, VertexBuf, IndexBuf, InstancePositionBuf, InstanceWeightBuf]
		(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_DrawPrimaryCanvas);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
			VertexBuf->InitRHI();
			IndexBuf->InitRHI();
			InstancePositionBuf->InitRHI();
			InstanceWeightBuf->InitRHI();
#else
			VertexBuf->InitRHI(RHICmdList);
			IndexBuf->InitRHI(RHICmdList);
			InstancePositionBuf->InitRHI(RHICmdList);
			InstanceWeightBuf->InitRHI(RHICmdList);
#endif
			FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("CaptureDrawCanvas"));

			uint32 NumInstances = InstancePositionBuf->Data.Num();
			NumInstances = FMath::Max(NumInstances, (uint32)1);

			// This great func CreateRenderTarget() would create RT and manage it, it's awesome~
			TRefCountPtr<IPooledRenderTarget> Canvas0_RT = CreateRenderTarget(
				Canvas0->GetResource()->GetTexture2DRHI(), TEXT("Canvas0"));
			FRDGTextureRef Canvas0_RDG = GraphBuilder.RegisterExternalTexture(Canvas0_RT);

			TRefCountPtr<IPooledRenderTarget> Canvas1_RT = CreateRenderTarget(
				Canvas1->GetResource()->GetTexture2DRHI(), TEXT("Canvas1"));
			FRDGTextureRef Canvas1_RDG = GraphBuilder.RegisterExternalTexture(Canvas1_RT);

			FIntVector TextureSize = Canvas0_RDG->Desc.GetSize();

			FXkCanvasRenderVS::FParameters* VertexShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderVS::FParameters>();
			*VertexShaderParams = VertexShaderParamsToCopy;

			FXkCanvasRenderPS::FParameters* PixelShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderPS::FParameters>();
			*PixelShaderParams = PixelShaderParamsToCopy;

			VertexShaderParams->LocalToWorld = FMatrix44f(LocalToWorld);
			VertexShaderParams->Center = Center;
			// Extent.X : world size x, Extent.Y : world size y, Extent.Z : unscaled patch coverage, Extent.W : unscaled world size
			VertexShaderParams->Extent = Extent;
			VertexShaderParams->InstancePositionBuffer = InstancePositionBuf->ShaderResourceViewRHI.GetReference();
			VertexShaderParams->InstanceWeightBuffer = InstanceWeightBuf->ShaderResourceViewRHI.GetReference();
			PixelShaderParams->RenderTargets[0] = FRenderTargetBinding(Canvas0_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			PixelShaderParams->RenderTargets[1] = FRenderTargetBinding(Canvas1_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			FIntRect Viewport = FIntRect(0, 0, TextureSize.X, TextureSize.Y);
			XkCanvasRendererDraw(GraphBuilder, NumInstances, Viewport, VertexShaderParams, PixelShaderParams,
				VertexBuf, IndexBuf);
			GraphBuilder.Execute();
		});
}


template<typename T>
inline void UXkCanvasRendererComponent::DrawFilteringCanvas_Internal(UTextureRenderTarget2D* Input0, UTextureRenderTarget2D* Input1, UTextureRenderTarget2D* Output)
{
	if (!IsRenderDataValid())
	{
		return;
	}

	UTextureRenderTarget2D* Canvas0 = Input0;
	UTextureRenderTarget2D* Canvas1 = Input1;
	FIntVector4 TextureFilter = FIntVector4(ConvolutionRangeX, ConvolutionRangeY, ConvolutionRangeZ, ConvolutionRangeW);
	FVector4f Center = CanvasCenter;
	FVector4f Extent = CanvasExtent;
	FMatrix44f LocalToWorld = FMatrix44f(GetOwner()->GetTransform().ToMatrixWithScale());
	RenderCaptureInterface::FScopedCapture RenderCapture(CaptureDrawCanvas, TEXT("CaptureDrawFilteringCanvas"));
	ENQUEUE_RENDER_COMMAND(UXkRendererComponent_DrawFilteringCanvas)([Output, Canvas0, Canvas1, LocalToWorld, Center, Extent, TextureFilter]
	(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_DrawCanvas);

			FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("CaptureDrawCanvas"));

			// This great func CreateRenderTarget() would create RT and manage it, it's awesome~
			TRefCountPtr<IPooledRenderTarget> Output_RT = CreateRenderTarget(
				Output->GetResource()->GetTexture2DRHI(), TEXT("Output"));
			FRDGTextureRef Output_RDG = GraphBuilder.RegisterExternalTexture(Output_RT);

			TRefCountPtr<IPooledRenderTarget> Canvas0_RT = CreateRenderTarget(
				Canvas0->GetResource()->GetTexture2DRHI(), TEXT("Canvas0"));
			FRDGTextureRef Canvas0_RDG = GraphBuilder.RegisterExternalTexture(Canvas0_RT);

			TRefCountPtr<IPooledRenderTarget> Canvas1_RT = CreateRenderTarget(
				Canvas1->GetResource()->GetTexture2DRHI(), TEXT("Canvas1"));
			FRDGTextureRef Canvas1_RDG = GraphBuilder.RegisterExternalTexture(Canvas1_RT);

			FIntVector TextureSize = Canvas0_RDG->Desc.GetSize();
			FRHICopyTextureInfo CopyTextureInfo;
			CopyTextureInfo.NumMips = 1;
			CopyTextureInfo.Size = TextureSize;

			const ETextureCreateFlags TextureFlags = TexCreate_ShaderResource | TexCreate_UAV | TexCreate_GenerateMipCapable | TexCreate_RenderTargetable;
			const FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(FIntPoint(TextureSize.X, TextureSize.Y),
				Output_RDG->Desc.Format, Output_RDG->Desc.ClearValue, TextureFlags, 1 /*NumMips*/);
			FRDGTextureRef CanvasTemp_RDG = GraphBuilder.CreateTexture(Desc, TEXT("CanvasTemp"));

			FXkCanvasRenderCS::FParameters* ComputerShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderCS::FParameters>();
			ComputerShaderParams->TextureSize = FIntVector4(TextureSize.X, TextureSize.Y, TextureSize.Z, 1);
			ComputerShaderParams->TextureFilter = TextureFilter;
			ComputerShaderParams->Center = Center; // @TODO: just computer the pixel area which changed by game logic
			ComputerShaderParams->Extent = Extent;
			ComputerShaderParams->SourceTexture0 = Canvas0_RDG;
			ComputerShaderParams->SourceTexture1 = Canvas1_RDG;
			ComputerShaderParams->TargetTexture = GraphBuilder.CreateUAV(CanvasTemp_RDG);
			FIntVector GroupCount = FIntVector(
				FMath::CeilToInt((float)TextureSize.X / FXkCanvasRenderCS::ThreadGroupSizeX),
				FMath::CeilToInt((float)TextureSize.Y / FXkCanvasRenderCS::ThreadGroupSizeY),
				1);
			XkCanvasComputeDispatch<T>(GraphBuilder, ComputerShaderParams, GroupCount);
			AddCopyTexturePass(GraphBuilder, CanvasTemp_RDG, Output_RDG, CopyTextureInfo);
			GraphBuilder.Execute();
		});
}
