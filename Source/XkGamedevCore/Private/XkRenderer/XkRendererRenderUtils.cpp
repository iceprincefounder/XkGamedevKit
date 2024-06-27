// Copyright ©XUKAI. All Rights Reserved.

#include "XkRenderer/XkRendererRenderUtils.h"

#include "PixelShaderUtils.h"
#include "CommonRenderResources.h"

TGlobalResource<FXkCanvasMapVertexDeclaration> GXkVertexDeclaration;
TGlobalResource<FXkCanvasVertexBuffer> GXkCanvasVertexBuffer;
TGlobalResource<FXkCanvasIndexBuffer> GXkCanvasIndexBuffer;

void FXkCanvasInstanceBuffer::InitRHI()
{
	if (Data.Num() == 0)
	{
		// Create the texture RHI.  		
		FRHIResourceCreateInfo CreateInfo(TEXT("WhiteVertexBuffer"));

		VertexBufferRHI = RHICreateVertexBuffer(sizeof(FVector4f), BUF_Static | BUF_ShaderResource, CreateInfo);

		FVector4f* BufferData = (FVector4f*)RHILockBuffer(VertexBufferRHI, 0, sizeof(FVector4f), RLM_WriteOnly);
		*BufferData = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
		RHIUnlockBuffer(VertexBufferRHI);

		// Create a view of the buffer
		ShaderResourceViewRHI = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FVector4f), PF_A32B32G32R32F);
	}
	else
	{
		TResourceArray<FVector4f, VERTEXBUFFER_ALIGNMENT> RawData;
		RawData.SetNumUninitialized(Data.Num());
		for (int32 i = 0; i < Data.Num(); i++)
		{
			RawData[i] = Data[i];
		}

		// Create the texture RHI.  		
		FRHIResourceCreateInfo CreateInfo(TEXT("FXkCanvasInstanceBuffer"), &RawData);
		VertexBufferRHI = RHICreateVertexBuffer(Data.Num() * sizeof(FVector4f), BUF_ShaderResource | BUF_Dynamic, CreateInfo);

		void* RawBufferData = RHILockBuffer(VertexBufferRHI, 0, RawData.GetResourceDataSize(), RLM_WriteOnly);
		FMemory::Memcpy((char*)RawBufferData, &Data[0], Data.Num() * sizeof(FVector4f));
		RHIUnlockBuffer(VertexBufferRHI);

		// Create a view of the buffer
		ShaderResourceViewRHI = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FVector4f), PF_A32B32G32R32F);
	}
}


void FXkCanvasVertexBuffer::InitRHI()
{
	check(Positions.Num() == UVs.Num());
	TResourceArray<FXkVertex, VERTEXBUFFER_ALIGNMENT> RawData;
	if (Positions.Num() == 0)
	{
		RawData.SetNumUninitialized(6);

		RawData[0].Position = FVector4f(1, 1, 0, 1);
		RawData[0].UV = FVector2f(1, 1);

		RawData[1].Position = FVector4f(0, 1, 0, 1);
		RawData[1].UV = FVector2f(0, 1);

		RawData[2].Position = FVector4f(1, 0, 0, 1);
		RawData[2].UV = FVector2f(1, 0);

		RawData[3].Position = FVector4f(0, 0, 0, 1);
		RawData[3].UV = FVector2f(0, 0);

		//The final two vertices are used for the triangle optimization (a single triangle spans the entire viewport )
		RawData[4].Position = FVector4f(-1, 1, 0, 1);
		RawData[4].UV = FVector2f(-1, 1);

		RawData[5].Position = FVector4f(1, -1, 0, 1);
		RawData[5].UV = FVector2f(1, -1);
	}
	else
	{
		RawData.SetNumUninitialized(Positions.Num());
		for (int32 i = 0; i < Positions.Num(); i++)
		{
			RawData[i].Position = Positions[i];
			RawData[i].UV = UVs[i];
		}
	}

	// Create vertex buffer. Fill buffer with initial data upon creation
	FRHIResourceCreateInfo CreateInfo(TEXT("FXkCanvasVertexBuffer"), &RawData);
	VertexBufferRHI = RHICreateVertexBuffer(RawData.GetResourceDataSize(), BUF_Static, CreateInfo);
}


void FXkCanvasIndexBuffer::InitRHI()
{
	TResourceArray<uint32, INDEXBUFFER_ALIGNMENT> RawData;
	if (Indices.Num() == 0)
	{
		Indices =
		{
			0, 1, 2, 2, 1, 3,	// [0 .. 5]  Full screen quad with 2 triangles
			0, 4, 5,			// [6 .. 8]  Full screen triangle
			3, 2, 1				// [9 .. 11] Full screen rect defined with TL, TR, BL corners
		};
	}
	uint32 NumIndices = Indices.Num();
	RawData.AddUninitialized(NumIndices);
	FMemory::Memcpy(RawData.GetData(), Indices.GetData(), NumIndices * sizeof(uint32));

	// Create index buffer. Fill buffer with initial data upon creation
	FRHIResourceCreateInfo CreateInfo(TEXT("FXkCanvasIndexBuffer"), &RawData);
	IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), RawData.GetResourceDataSize(), BUF_Static, CreateInfo);
}


void FXkCanvasRenderVS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("MAX_INSTANCE_COUNT"), MAX_INSTANCE_COUNT);
}


void FXkCanvasRenderPS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("APPLY_PROCMESH_PATCH"), 1);
}


void XkCanvasRendererDraw(FRDGBuilder& GraphBuilder,
	const uint32 InNumInstances,
	const FIntRect& InDestinationBounds,
	FXkCanvasRenderVS::FParameters* InVSParameters,
	FXkCanvasRenderPS::FParameters* InPSParameters,
	FXkCanvasVertexBuffer* InVertexBuffer,
	FXkCanvasIndexBuffer* InIndexBuffer)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FXkCanvasRenderVS> VertexShader(ShaderMap);
	TShaderMapRef<FXkCanvasRenderPS> PixelShader(ShaderMap);

	FRDGEventName&& PassName = RDG_EVENT_NAME("XkCanvasRendererDraw");
	const FIntRect& Viewport = InDestinationBounds;
	FRHIBlendState* BlendState = nullptr;
	FRHIRasterizerState* RasterizerState = nullptr;
	FRHIDepthStencilState* DepthStencilState = nullptr;
	uint32 StencilRef = 0;

	ClearUnusedGraphResources(VertexShader, InVSParameters);
	ClearUnusedGraphResources(PixelShader, InPSParameters);

	GraphBuilder.AddPass(
		Forward<FRDGEventName>(PassName),
		InPSParameters,
		ERDGPassFlags::Raster,
		[InNumInstances, InVSParameters, InPSParameters, InVertexBuffer, InIndexBuffer, VertexShader, PixelShader, Viewport, BlendState, RasterizerState, DepthStencilState, StencilRef]
	(FRHICommandList& RHICmdList)
		{
			check(VertexShader.IsValid() && PixelShader.IsValid());
			RHICmdList.SetViewport((float)Viewport.Min.X, (float)Viewport.Min.Y, 0.0f, (float)Viewport.Max.X, (float)Viewport.Max.Y, 1.0f);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			// @see: 
			// FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, ShaderMap, PixelShader, /* out */ GraphicsPSOInit);
			// InitFullscreenPipelineState
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GXkVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			// GraphicsPSOInit override
			GraphicsPSOInit.BlendState = BlendState ? BlendState : GraphicsPSOInit.BlendState;
			GraphicsPSOInit.RasterizerState = RasterizerState ? RasterizerState : GraphicsPSOInit.RasterizerState;
			GraphicsPSOInit.DepthStencilState = DepthStencilState ? DepthStencilState : GraphicsPSOInit.DepthStencilState;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, StencilRef);

			SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *InVSParameters);
			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *InPSParameters);

			// @see: 
			// FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
			// DrawFullscreenTriangle
			uint32 InstanceCount = InNumInstances;
			RHICmdList.SetStreamSource(0, InVertexBuffer->VertexBufferRHI, 0);

			uint32 StartIndex = 0;
			uint32 IndexCount = InIndexBuffer->GetIndexCountNum();
			// check validation of IndexBuffer
			if ((StartIndex + IndexCount) * InIndexBuffer->IndexBufferRHI->GetStride() > InIndexBuffer->IndexBufferRHI->GetSize())
			{
				return;
			}
			RHICmdList.DrawIndexedPrimitive(
				InIndexBuffer->IndexBufferRHI,
				/*BaseVertexIndex=*/ 0,
				/*MinIndex=*/ 0,
				/*NumVertices=*/ InVertexBuffer->GetVertexNum(),
				/*StartIndex=*/ StartIndex,
				/*NumPrimitives=*/ InIndexBuffer->GetTriangleNum(),
				/*NumInstances=*/ InstanceCount);
		});
}


void FXkCanvasRenderCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadGroupSizeX);
	OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), ThreadGroupSizeY);
}


template<typename T, typename P>
void XkCanvasComputeDispatch(FRDGBuilder& GraphBuilder, P* InCSParameters, const FIntVector& DispatchCount)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<T> ComputeShader(ShaderMap);
	FRDGEventName&& PassName = RDG_EVENT_NAME("XkCanvasComputeDispatch");
	GraphBuilder.AddPass(
		Forward<FRDGEventName>(PassName),
		InCSParameters,
		ERDGPassFlags::Compute,
		[InCSParameters, ComputeShader, DispatchCount]
	(FRHICommandList& RHICmdList)
		{
			ensure(DispatchCount.X <= GRHIMaxDispatchThreadGroupsPerDimension.X);
			ensure(DispatchCount.Y <= GRHIMaxDispatchThreadGroupsPerDimension.Y);
			ensure(DispatchCount.Z <= GRHIMaxDispatchThreadGroupsPerDimension.Z);

			FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
			SetComputePipelineState(RHICmdList, ShaderRHI);
			SetShaderParameters(RHICmdList, ComputeShader, ShaderRHI, *InCSParameters);
			RHICmdList.DispatchComputeShader(DispatchCount.X, DispatchCount.Y, DispatchCount.Z);
			UnsetShaderUAVs(RHICmdList, ComputeShader, ShaderRHI);
		});

	// @see:
	// FComputeShaderUtils::AddPass( GraphBuilder, RDG_EVENT_NAME("ApplyPatchFilter"), ComputeShader, InCSParameters, DispatchCount);
}


IMPLEMENT_UNIFORM_BUFFER_STRUCT(FXkCanvasRenderParameters, "Parameters");
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderVS, "/Plugin/XkGamedevKit/Private/XkRendererVS.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderPS, "/Plugin/XkGamedevKit/Private/XkRendererPS.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderCS, "/Plugin/XkGamedevKit/Private/XkRendererCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderHeightCS, "/Plugin/XkGamedevKit/Private/XkRendererCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderNormalCS, "/Plugin/XkGamedevKit/Private/XkRendererCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FXkCanvasRenderSdfCS, "/Plugin/XkGamedevKit/Private/XkRendererCS.usf", "MainCS", SF_Compute);