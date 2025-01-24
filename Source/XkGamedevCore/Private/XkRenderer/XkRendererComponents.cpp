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

	CanvasCenter = FVector4f::Zero();
	CanvasCenter.W = 200.0f; // CanvasCenter.W for land height range, to calculate ocean SDF
	CanvasExtent = FVector4f(204800.0/*WorldSize.X*/, 204800.0/*WorldSize.Y*/, -2048.0/*HeightRange MinZ*/, 2048.0/*HeightRange MaxZ*/);
	HorizonHeight = 100.0f;

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
	if (VertexBuffer.IsInitialized() && IndexBuffer.IsInitialized() && InstancePositionBuffer.IsInitialized() && InstanceWeightBuffer.IsInitialized())
	{
			return true;
	}
	return false;
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
			VertexBuffer.InitRHI();
			IndexBuffer.InitRHI();
			InstancePositionBuffer.InitRHI();
			InstanceWeightBuffer.InitRHI();
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
		});
}


void UXkCanvasRendererComponent::DrawCanvas()
{
	if (!CanvasRT0 || !CanvasRT1 || InstancePositionBuffer.GetInstanceNum() == 0 || InstanceWeightBuffer.GetInstanceNum() == 0)
	{
		return;
	}

	UTextureRenderTarget2D* Canvas0 = CanvasRT0;
	UTextureRenderTarget2D* Canvas1 = CanvasRT1;
	FIntVector4 TextureFilter = FIntVector4(ConvolutionRangeX, ConvolutionRangeY, ConvolutionRangeZ, 0);
	FIntVector4 SuperResMask = FIntVector4(SplatMaskRange.X, SplatMaskRange.Y, 2, 4);
	FVector4f Center = CanvasCenter;
	FVector4f Extent = CanvasExtent;
	FXkCanvasVertexBuffer* VertexBuf = &VertexBuffer;
	FXkCanvasIndexBuffer* IndexBuf = &IndexBuffer;
	FXkCanvasInstanceBuffer* InstancePositionBuf = &InstancePositionBuffer;
	FXkCanvasInstanceBuffer* InstanceWeightBuf = &InstanceWeightBuffer;
	FXkCanvasRenderVS::FParameters VertexShaderParamsToCopy;
	FXkCanvasRenderPS::FParameters PixelShaderParamsToCopy;
	FMatrix44f LocalToWorld = FMatrix44f(GetOwner()->GetTransform().ToMatrixWithScale());
	RenderCaptureInterface::FScopedCapture RenderCapture(CaptureDrawCanvas, TEXT("CaptureDrawCanvas"));
	ENQUEUE_RENDER_COMMAND(UXkRendererComponent_DrawCanvas)([Canvas0, Canvas1, TextureFilter, SuperResMask, LocalToWorld, Center, Extent,
		VertexShaderParamsToCopy, PixelShaderParamsToCopy, VertexBuf, IndexBuf, InstancePositionBuf, InstanceWeightBuf]
		(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_DrawCanvas);

			VertexBuf->InitRHI();
			IndexBuf->InitRHI();
			InstancePositionBuf->InitRHI();
			InstanceWeightBuf->InitRHI();

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
			FRHICopyTextureInfo CopyTextureInfo;
			CopyTextureInfo.NumMips = 1;
			CopyTextureInfo.Size = TextureSize;

			const ETextureCreateFlags TextureFlags = TexCreate_ShaderResource | TexCreate_UAV | TexCreate_GenerateMipCapable | TexCreate_RenderTargetable;
			const FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(FIntPoint(TextureSize.X, TextureSize.Y),
				Canvas0_RDG->Desc.Format, Canvas0_RDG->Desc.ClearValue, TextureFlags, 1 /*NumMips*/);
			FRDGTextureRef CanvasTemp_RDG = GraphBuilder.CreateTexture(Desc, TEXT("CanvasTemp"));

			FXkCanvasRenderVS::FParameters* VertexShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderVS::FParameters>();
			*VertexShaderParams = VertexShaderParamsToCopy;

			FXkCanvasRenderPS::FParameters* PixelShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderPS::FParameters>();
			*PixelShaderParams = PixelShaderParamsToCopy;

			TRDGUniformBufferRef<FXkCanvasRenderParameters> PassUniformBuffer = nullptr;
			auto* BuildParameters = GraphBuilder.AllocParameters<FXkCanvasRenderParameters>();
			{
				// Object data
				BuildParameters->LocalToWorld = FMatrix44f(LocalToWorld);
				BuildParameters->Center = Center;
				// Extent.X : world size x, Extent.Y : world size y, Extent.Z : unscaled patch coverage, Extent.W : unscaled world size
				BuildParameters->Extent = Extent;
				BuildParameters->Color = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
				PassUniformBuffer = GraphBuilder.CreateUniformBuffer(BuildParameters);
			}
			VertexShaderParams->Parameters = PassUniformBuffer;
			VertexShaderParams->InstancePositionBuffer = InstancePositionBuf->ShaderResourceViewRHI.GetReference();
			VertexShaderParams->InstanceWeightBuffer = InstanceWeightBuf->ShaderResourceViewRHI.GetReference();
			PixelShaderParams->Parameters = PassUniformBuffer;

			PixelShaderParams->RenderTargets[0] = FRenderTargetBinding(Canvas0_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			PixelShaderParams->RenderTargets[1] = FRenderTargetBinding(Canvas1_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			FIntRect Viewport = FIntRect(0, 0, TextureSize.X, TextureSize.Y);
			XkCanvasRendererDraw(GraphBuilder, NumInstances, Viewport, VertexShaderParams, PixelShaderParams,
				VertexBuf, IndexBuf);
			FXkCanvasRenderCS::FParameters* ComputerShaderParams =
				GraphBuilder.AllocParameters<FXkCanvasRenderCS::FParameters>();
			ComputerShaderParams->TextureFilter = FIntVector4(TextureFilter.X, TextureFilter.Y, TextureFilter.Z, TextureSize.X);
			ComputerShaderParams->SuperResMask = SuperResMask;
			ComputerShaderParams->Center = Center; // @TODO: just computer the pixel area which changed by game logic
			ComputerShaderParams->Extent = Extent;
			ComputerShaderParams->SourceTexture0 = Canvas0_RDG;
			ComputerShaderParams->SourceTexture1 = Canvas1_RDG;
			ComputerShaderParams->TargetTexture = GraphBuilder.CreateUAV(CanvasTemp_RDG);
			FIntVector GroupCount = FIntVector(
				FMath::CeilToInt((float)TextureSize.X / FXkCanvasRenderCS::ThreadGroupSizeX),
				FMath::CeilToInt((float)TextureSize.Y / FXkCanvasRenderCS::ThreadGroupSizeY),
				1);
			XkCanvasComputeDispatch<FXkCanvasRenderHeightCS>(GraphBuilder, ComputerShaderParams, GroupCount);
			AddCopyTexturePass(GraphBuilder, CanvasTemp_RDG, Canvas0_RDG, CopyTextureInfo);
			XkCanvasComputeDispatch<FXkCanvasRenderNormalCS>(GraphBuilder, ComputerShaderParams, GroupCount);
			AddCopyTexturePass(GraphBuilder, CanvasTemp_RDG, Canvas0_RDG, CopyTextureInfo);
			XkCanvasComputeDispatch<FXkCanvasRenderSdfCS>(GraphBuilder, ComputerShaderParams, GroupCount);
			AddCopyTexturePass(GraphBuilder, CanvasTemp_RDG, Canvas0_RDG, CopyTextureInfo);
			GraphBuilder.Execute();
		});
}