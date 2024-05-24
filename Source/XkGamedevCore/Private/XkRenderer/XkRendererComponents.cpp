// Copyright ©XUKAI. All Rights Reserved.


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

void UXkCanvasRendererComponent::CreateBuffers(
	FXkCanvasVertexBuffer* VertexBuffer, 
	FXkCanvasIndexBuffer* IndexBuffer, 
	FXkCanvasInstanceBuffer* InstancePositionBuffer, 
	FXkCanvasInstanceBuffer* InstanceWeightBuffer)
{
	ENQUEUE_RENDER_COMMAND(UXkCanvasRendererComponent_CreateBuffers)([VertexBuffer, IndexBuffer, InstancePositionBuffer, InstanceWeightBuffer]
		(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_CreateBuffers);
			VertexBuffer->InitRHI();
			IndexBuffer->InitRHI();
			InstancePositionBuffer->InitRHI();
			InstanceWeightBuffer->InitRHI();
		});
}


void UXkCanvasRendererComponent::DrawCanvas(
	FXkCanvasVertexBuffer* VertexBuffer, 
	FXkCanvasIndexBuffer* IndexBuffer, 
	FXkCanvasInstanceBuffer* InstancePositionBuffer, 
	FXkCanvasInstanceBuffer* InstanceWeightBuffer,
	const FVector4f& Center, const FVector4f& Extent)
{
	if (!VertexBuffer || !IndexBuffer)
	{
		return;
	}

	if (!CanvasRT0 || !CanvasRT1)
	{
		return;
	}

	UTextureRenderTarget2D* Canvas0 = CanvasRT0;
	UTextureRenderTarget2D* Canvas1 = CanvasRT1;
	FXkCanvasRenderVS::FParameters VertexShaderParamsToCopy;
	FXkCanvasRenderPS::FParameters PixelShaderParamsToCopy;
	FMatrix44f LocalToWorld = FMatrix44f(GetOwner()->GetTransform().ToMatrixWithScale());
	RenderCaptureInterface::FScopedCapture RenderCapture(CaptureDrawCanvas, TEXT("CaptureDrawCanvas"));
	ENQUEUE_RENDER_COMMAND(UXkRendererComponent_DrawCanvas)([Canvas0, Canvas1, LocalToWorld, Center, Extent,
		VertexShaderParamsToCopy, PixelShaderParamsToCopy, VertexBuffer, IndexBuffer, InstancePositionBuffer, InstanceWeightBuffer]
		(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UXkCanvasRendererComponent_DrawCanvas);

			VertexBuffer->InitRHI();
			IndexBuffer->InitRHI();
			InstancePositionBuffer->InitRHI();
			InstanceWeightBuffer->InitRHI();

			FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("DrawCanvas"));

			uint32 NumInstances = InstancePositionBuffer->Data.Num();
			NumInstances = FMath::Max(NumInstances, (uint32)1);

			// This great func CreateRenderTarget() would create RT and manage it, it's awesome~
			TRefCountPtr<IPooledRenderTarget> Canvas0_RT = CreateRenderTarget(
				Canvas0->GetResource()->GetTexture2DRHI(), TEXT("Canvas0"));
			FRDGTextureRef Canvas0_RDG = GraphBuilder.RegisterExternalTexture(Canvas0_RT);

			TRefCountPtr<IPooledRenderTarget> Canvas1_RT = CreateRenderTarget(
				Canvas1->GetResource()->GetTexture2DRHI(), TEXT("Canvas1"));
			FRDGTextureRef Canvas1_RDG = GraphBuilder.RegisterExternalTexture(Canvas1_RT);

			FIntVector TextureSize = Canvas0_RDG->Desc.GetSize();
			EPixelFormat TextureFormat = Canvas0_RDG->Desc.Format;
			FClearValueBinding ClearValue = Canvas0_RDG->Desc.ClearValue;
			FVector4f DefaultValue = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
			FRHICopyTextureInfo CopyTextureInfo;
			CopyTextureInfo.NumMips = 1;
			CopyTextureInfo.Size = TextureSize;

			const ETextureCreateFlags TextureFlags = TexCreate_ShaderResource | TexCreate_UAV | TexCreate_GenerateMipCapable | TexCreate_RenderTargetable;
			const FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(FIntPoint(TextureSize.X, TextureSize.Y),
				TextureFormat, ClearValue, TextureFlags, 1 /*NumMips*/);
			FRDGTextureRef CanvasTemp0_RDG = GraphBuilder.CreateTexture(Desc, TEXT("CanvasTemp0"));
			FRDGTextureRef CanvasTemp1_RDG = GraphBuilder.CreateTexture(Desc, TEXT("CanvasTemp1"));

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
				BuildParameters->Center = FVector4f(Center.X, Center.Y, Center.Z, Center.W);
				BuildParameters->Extent = FVector4f(Extent.X, Extent.Y, Extent.Z, Extent.W);
				BuildParameters->Color = DefaultValue;
				PassUniformBuffer = GraphBuilder.CreateUniformBuffer(BuildParameters);
			}
			VertexShaderParams->Parameters = PassUniformBuffer;
			VertexShaderParams->InstancePositionBuffer = InstancePositionBuffer->ShaderResourceViewRHI.GetReference();
			VertexShaderParams->InstanceWeightBuffer = InstanceWeightBuffer->ShaderResourceViewRHI.GetReference();
			PixelShaderParams->Parameters = PassUniformBuffer;

			PixelShaderParams->RenderTargets[0] = FRenderTargetBinding(Canvas0_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			PixelShaderParams->RenderTargets[1] = FRenderTargetBinding(Canvas1_RDG, ERenderTargetLoadAction::EClear, /*InMipIndex = */0);
			FIntRect Viewport = FIntRect(0, 0, TextureSize.X, TextureSize.Y);
			XkCanvasRendererDraw(GraphBuilder, NumInstances, Viewport, VertexShaderParams, PixelShaderParams,
				VertexBuffer, IndexBuffer);

			//FXkApplyProcMeshPatchCS::FParameters* ComputerShaderParams =
			//	GraphBuilder.AllocParameters<FXkApplyProcMeshPatchCS::FParameters>();
			//ComputerShaderParams->TextureSize = FIntVector4(0, 0, TextureSize.X, TextureSize.Y);
			//ComputerShaderParams->FilterParms = Params;
			//ComputerShaderParams->DefaultValue = DefaultValue;
			//ComputerShaderParams->SourceTexture = TextureAssetCanvas0_RDG;
			//ComputerShaderParams->TargetTexture = GraphBuilder.CreateUAV(TextureAssetCanvas2_RDG);
			//FIntVector GroupCount = FIntVector(
			//	FMath::CeilToInt((float)TextureSize.X / FXkApplyProcMeshPatchCS::ThreadGroupSizeX),
			//	FMath::CeilToInt((float)TextureSize.Y / FXkApplyProcMeshPatchCS::ThreadGroupSizeY),
			//	1);
			//FXkApplyProcMeshPatchCS::AddToRenderGraph(GraphBuilder, ComputerShaderParams, GroupCount);
			//AddCopyTexturePass(GraphBuilder, TextureAssetCanvas2_RDG, TextureAssetTarget_RDG, CopyTextureInfo);
			//AddCopyTexturePass(GraphBuilder, TextureAssetCanvas2_RDG, TextureAssetCanvas1_RDG, CopyTextureInfo);

			GraphBuilder.Execute();
		});
}
