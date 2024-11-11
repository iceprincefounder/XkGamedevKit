// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkLandscapeRenderUtils.h"
#include "XkLandscapeComponents.h"
#include "ShaderParameters.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "VertexFactory.h"
#include "MeshMaterialShader.h"

class FXkQuadtreeVertexFactory;
class FXkQuadtreeSceneProxy;
class FXkQuadtreeVertexFactoryShaderParameters;

struct FPatchData
{
	TArray<FVector4f> Vertices;
	TArray<uint32> Indices;
};


class FXkQuadtreeVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FXkQuadtreeVertexFactory);
public:
	FXkQuadtreeVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	virtual ~FXkQuadtreeVertexFactory()
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

	void SetVertexStreams(FVertexBuffer* InStream0, FVertexBuffer* InStream1, FVertexBuffer* InStream2);

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	FVertexBuffer* VertexPositionBuffer;
	FVertexBuffer* InstancePositionBuffer;
	FVertexBuffer* InstanceMorphBuffer;
};


class FXkQuadtreeSceneProxy : public FPrimitiveSceneProxy
{
	friend UXkQuadtreeComponent;
	friend FXkQuadtreeVertexFactoryShaderParameters;
	friend FXkQuadtreeVertexFactory;

public:
	FXkQuadtreeSceneProxy(
		const UXkQuadtreeComponent* InComponent, const FName ResourceName = NAME_None,
		FMaterialRenderProxy* InMaterialRenderProxy = nullptr);

	virtual ~FXkQuadtreeSceneProxy();

	typedef FXkQuadtreeSceneProxy Super;

protected:
	mutable FQuadtree Quadtree;
	UXkQuadtreeComponent* OwnerComponent;
	FXkQuadtreeVertexFactory* VertexFactory;
};


class FXkLandscapeSceneProxy : public FXkQuadtreeSceneProxy
{
	friend class UXkLandscapeComponent;
public:
	FXkLandscapeSceneProxy(
		const UXkLandscapeComponent* InComponent, const FName ResourceName = NAME_None,
		FMaterialRenderProxy* InMaterialRenderProxy = nullptr);

	virtual ~FXkLandscapeSceneProxy();

	//~ Begin FPrimitiveSceneProxy Interface
	virtual SIZE_T GetTypeHash() const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	virtual uint32 GetAllocatedSize(void) const { return(FXkQuadtreeSceneProxy::GetAllocatedSize()); }
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		class FMeshElementCollector& Collector) const override;
	virtual void CreateRenderThreadResources() override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override { return false; };
	//~ End FPrimitiveSceneProxy Interface

	void GenerateBuffers();
	void GenerateBuffers_Renderthread(FRHICommandListImmediate& RHICmdList, FPatchData* InPatchData);
	virtual void UpdateInstanceBuffer(const int16 InFrameTag);

protected:
	FMaterialRenderProxy* MaterialRenderProxy;
	FMaterialRelevance MaterialRelevance;

	FVertexBuffer VertexPositionBuffer_GPU;
	FVertexBuffer InstancePositionBuffer_GPU;
	FVertexBuffer InstanceMorphBuffer_GPU;
	FIndexBuffer  IndexBuffer_GPU;

	uint8 PatchSize;
	FPatchData PatchData;
};


class FXkLandscapeWithWaterSceneProxy : public FXkLandscapeSceneProxy
{
	friend class UXkLandscapeWithWaterComponent;
public:
	FXkLandscapeWithWaterSceneProxy(
		const UXkLandscapeWithWaterComponent* InComponent, const FName ResourceName = NAME_None,
		FMaterialRenderProxy* InMaterialRenderProxy = nullptr, FMaterialRenderProxy* InWaterMaterialRenderProxy = nullptr);

	virtual ~FXkLandscapeWithWaterSceneProxy();

	//~ Begin FXkQuadtreeSceneProxy Interface
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		class FMeshElementCollector& Collector) const override;
	virtual void CreateRenderThreadResources() override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual void UpdateInstanceBuffer(const int16 InFrameTag) override;
	//~ End FXkQuadtreeSceneProxy Interface

private:
	void GenerateBuffers();
	void GenerateBuffers_Renderthread(FRHICommandListImmediate& RHICmdList, FPatchData* InPatchData);

protected:
	FXkQuadtreeVertexFactory* WaterVertexFactory;
	FMaterialRenderProxy* WaterMaterialRenderProxy;
	FMaterialRelevance WaterMaterialRelevance;

	FVertexBuffer WaterVertexPositionBuffer_GPU;
	FVertexBuffer WaterInstancePositionBuffer_GPU;
	FVertexBuffer WaterInstanceMorphBuffer_GPU;
	FIndexBuffer  WaterIndexBuffer_GPU;

	uint8 WaterPatchSize;
	FPatchData WaterPatchData;
};


class FXkSphericalLandscapeWithWaterSceneProxy final : public FXkLandscapeWithWaterSceneProxy
{
	friend class UXkSphericalLandscapeWithWaterComponent;
public:
	FXkSphericalLandscapeWithWaterSceneProxy(
		const UXkSphericalLandscapeWithWaterComponent* InComponent, const FName ResourceName = NAME_None,
		FMaterialRenderProxy* InMaterialRenderProxy = nullptr, FMaterialRenderProxy* InWaterMaterialRenderProxy = nullptr);

	virtual ~FXkSphericalLandscapeWithWaterSceneProxy();

	//~ Begin FXkQuadtreeSceneProxy Interface
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		class FMeshElementCollector& Collector) const override;

	void UpdateInstanceBuffer(const int16 InFrameTag) override;
	//~ End FXkQuadtreeSceneProxy Interface
};