// Copyright ©XUKAI. All Rights Reserved.

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
	virtual void InitRHI() override;

	/**
	* Release render resources for this vertex factory.
	*/
	virtual void ReleaseRHI() override {};

	static bool ShouldCache(const FVertexFactoryShaderPermutationParameters& Parameters) { return true; }

	void SetSceneProxy(FXkHexagonalWorldSceneProxy* pProxy);

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	FXkHexagonalWorldSceneProxy* SceneProxy;
};


class FXkHexagonalWorldSceneProxy : public FPrimitiveSceneProxy
{
	friend UXkHexagonalWorldComponent;
	friend FXkHexagonalWorldVertexFactoryShaderParameters;
	friend FXkHexagonalWorldVertexFactory;

	struct FHexagonData
	{
		TArray<FVector4f> BaseVertices;
		TArray<uint32> BaseIndices;

		TArray<FVector4f> EdgeVertices;
		TArray<uint32> EdgeIndices;
	} HexagonData;
public:
	FXkHexagonalWorldSceneProxy(
		const UXkHexagonalWorldComponent* InComponent, const FName ResourceName = NAME_None,
		FMaterialRenderProxy* InBaseMaterialRenderProxy = nullptr, FMaterialRenderProxy* InEdgeMaterialRenderProxy = nullptr);

	virtual ~FXkHexagonalWorldSceneProxy();

	typedef FXkHexagonalWorldSceneProxy Super;

	//~ Begin FPrimitiveSceneProxy Interface
	virtual SIZE_T GetTypeHash() const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		class FMeshElementCollector& Collector) const override;
	virtual void CreateRenderThreadResources() override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override { return false; };
	//~ End FPrimitiveSceneProxy Interface

	virtual void GenerateBuffers();
	virtual void GenerateBuffers_Renderthread(FRHICommandListImmediate& RHICmdList, FHexagonData* InHexagonData);
	virtual void UpdateInstanceBuffer(const int16 InFrameTag);

protected:
	UXkHexagonalWorldComponent* OwnerComponent;
	FXkHexagonalWorldVertexFactory* VertexFactory;
	FMaterialRenderProxy* BaseMaterialRenderProxy;
	FMaterialRenderProxy* EdgeMaterialRenderProxy;
	FMaterialRelevance MaterialRelevance;

	FVertexBuffer VertexPositionBuffer_GPU;
	FVertexBuffer InstancePositionBuffer_GPU;
	FVertexBuffer InstanceWeightBuffer_GPU;
	FIndexBuffer  BaseIndexBuffer_GPU;
	FIndexBuffer  EdgeIndexBuffer_GPU;
};