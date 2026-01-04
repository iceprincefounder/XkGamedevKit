// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "VertexFactory.h"
#include "XkHexagonComponents.h"

class FXkHexagonalWorldVertexFactoryShaderParameters;
class FXkHexagonalWorldVertexFactory;
class FXkHexagonalWorldSceneProxy;

class FXkHexagonalWorldVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FXkHexagonalWorldVertexFactory);
public:
	FXkHexagonalWorldVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	virtual ~FXkHexagonalWorldVertexFactory()
	{
		// can only be destroyed from the render thread
		ReleaseResource();
	}

	/**
	* Constructs render resources for this vertex factory.
	*/
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
#else
	virtual void InitRHI() override;
#endif
	/**
	* Release render resources for this vertex factory.
	*/
	virtual void ReleaseRHI() override {};

	static bool ShouldCache(const FVertexFactoryShaderPermutationParameters& Parameters) { return true; }

	void SetVertexBuffer(FVertexBuffer* InData0, FVertexBuffer* InData1, FVertexBuffer* InData2);

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	FVertexBuffer* VertexPositionVertexBuffer;
	FVertexBuffer* InstancePositionVertexBuffer;
	FVertexBuffer* InstanceColorVertexBuffer;
};