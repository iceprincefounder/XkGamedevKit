// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

// 1024 x 1024 x 16
#define	MAX_INSTANCE_COUNT 512

/** The vertex data used to filter a texture. */
struct XKGAMEDEVCORE_API FXkVertex
{
public:
	FVector4f Position;
	FVector2f UV;
};


/** The proc mesh vertex declaration resource type. */
class XKGAMEDEVCORE_API FXkCanvasMapVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/** Destructor. */
	virtual ~FXkCanvasMapVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		uint16 Stride = sizeof(FXkVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FXkVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FXkVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};
extern XKGAMEDEVCORE_API TGlobalResource<FXkCanvasMapVertexDeclaration> GXkVertexDeclaration;


/**
 * CanvasInstanceBuffer for both instance position and instance weight
 */
class XKGAMEDEVCORE_API FXkCanvasInstanceBuffer : public FVertexBufferWithSRV
{
public:
	virtual void InitRHI() override;

	int32 GetInstanceNum() const { return Data.Num(); }

	TArray<FVector4f> Data;
};


/**
 * CanvasVertexBuffer
 */
class XKGAMEDEVCORE_API FXkCanvasVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override;

	int32 GetVertexNum() const { return Positions.Num(); }

	TArray<FVector4f> Positions;
	TArray<FVector2f> UVs;
};

extern XKGAMEDEVCORE_API TGlobalResource<FXkCanvasVertexBuffer> GXkCanvasVertexBuffer;


/**
 * ProcMesh index buffer.
 */
class XKGAMEDEVCORE_API FXkCanvasIndexBuffer : public FIndexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override;

	int32 GetTriangleNum() const { return Indices.Num() / 3; }
	int32 GetIndexCountNum() const { return Indices.Num(); }

	TArray<uint32> Indices;
};

extern XKGAMEDEVCORE_API TGlobalResource<FXkCanvasIndexBuffer> GXkCanvasIndexBuffer;


/////////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT #1 Parameters Name #2 API to export!!!
// @Note: I debugged this shit for hours finally know I should put XKGAMEDEVCORE_API into second args to fix compile issue.
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FXkCanvasRenderParameters, XKGAMEDEVCORE_API)
SHADER_PARAMETER(FMatrix44f, LocalToWorld)
SHADER_PARAMETER(FVector4f, Center)
SHADER_PARAMETER(FVector4f, Extent)
SHADER_PARAMETER(FVector4f, Color)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

/**
 * Vertex Shader that resterilizes a procedural mesh into texture.
 */
class XKGAMEDEVCORE_API FXkCanvasRenderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderVS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FXkCanvasRenderParameters, Parameters)
		SHADER_PARAMETER_SRV(Buffer<float4>, InstancePositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, InstanceWeightBuffer)
		RENDER_TARGET_BINDING_SLOTS() // Holds our output
		END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& InParameters, FShaderCompilerEnvironment& OutEnvironment);
};


/**
 * Pixel Shader that resterilizes a procedural mesh into texture.
 */
class XKGAMEDEVCORE_API FXkCanvasRenderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderPS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FXkCanvasRenderParameters, Parameters)
		RENDER_TARGET_BINDING_SLOTS() // Holds our output
		END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& InParameters, FShaderCompilerEnvironment& OutEnvironment);
};


extern void XKGAMEDEVCORE_API XkCanvasRendererDraw(FRDGBuilder& GraphBuilder,
	const uint32 NumInstances,
	const FIntRect& InDestinationBounds,
	FXkCanvasRenderVS::FParameters* InVSParameters,
	FXkCanvasRenderPS::FParameters* InPSParameters,
	FXkCanvasVertexBuffer* InVertexBuffer,
	FXkCanvasIndexBuffer* InIndexBuffer);

/**
 * Computer shader to filter the canvas render targets.
 */
class XKGAMEDEVCORE_API FXkCanvasRenderCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderCS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FIntVector4, TextureFilter) // #0 Height range #1 Normal range #3 SDF range #4 Texture size
		SHADER_PARAMETER(FIntVector4, SuperResMask)  // #0 Low splat id #1 High splat id #3 SuperRes minimal value #4 SuperRes scale factor
		SHADER_PARAMETER(FVector4f, Center) // @TODO: just computer the pixel area which changed by game logic
		SHADER_PARAMETER(FVector4f, Extent)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture0)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture1)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, TargetTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static const uint32 ThreadGroupSizeX = 16;
	static const uint32 ThreadGroupSizeY = 16;
};


class XKGAMEDEVCORE_API FXkCanvasRenderHeightCS : public FXkCanvasRenderCS
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderHeightCS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderHeightCS, FXkCanvasRenderCS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FXkCanvasRenderCS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HEIGHT_FILTER"), 1);
	}
};


class XKGAMEDEVCORE_API FXkCanvasRenderNormalCS : public FXkCanvasRenderCS
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderNormalCS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderNormalCS, FXkCanvasRenderCS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FXkCanvasRenderCS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NORMAL_FILTER"), 1);
	}
};


class XKGAMEDEVCORE_API FXkCanvasRenderSdfCS : public FXkCanvasRenderCS
{
	DECLARE_GLOBAL_SHADER(FXkCanvasRenderSdfCS);
	SHADER_USE_PARAMETER_STRUCT(FXkCanvasRenderSdfCS, FXkCanvasRenderCS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; };
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FXkCanvasRenderCS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SDF_FILTER"), 1);
	}
};


template<typename T, typename P>
extern void XKGAMEDEVCORE_API XkCanvasComputeDispatch(FRDGBuilder& GraphBuilder, P* InCSParameters, const FIntVector& DispatchCount);