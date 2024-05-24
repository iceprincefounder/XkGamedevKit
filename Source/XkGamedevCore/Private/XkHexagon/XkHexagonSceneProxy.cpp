// Copyright ©XUKAI. All Rights Reserved.


#include "XkHexagon/XkHexagonSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/CollisionProfile.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "UObject/UObjectIterator.h"
#include "StaticMeshResources.h"
#include "MeshMaterialShader.h"


class FXkHexagonalWorldVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FXkHexagonalWorldVertexFactoryShaderParameters, NonVirtual);
public:
	void Bind(const FShaderParameterMap& ParameterMap) {};

	void GetElementShaderBindings(
		const class FSceneInterface* Scene,
		const class FSceneView* View,
		const class FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const class FVertexFactory* VertexFactory,
		const struct FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
	}
};


FXkHexagonalWorldVertexFactory::FXkHexagonalWorldVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	:FVertexFactory(InFeatureLevel)
{
	SceneProxy = NULL;
}


void FXkHexagonalWorldVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	if (SceneProxy)
	{
		FVertexStreamComponent VertexPosStream(&SceneProxy->VertexPositionBuffer_GPU, 0, sizeof(FVector4f), VET_Float4);

		Elements.Add(AccessStreamComponent(VertexPosStream, 0));

		FVertexStreamComponent PositionInstStream(&SceneProxy->InstancePositionBuffer_GPU, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(PositionInstStream, 1));

		FVertexStreamComponent WeightInstStream(&SceneProxy->InstanceWeightBuffer_GPU, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(WeightInstStream, 2));

		InitDeclaration(Elements);
	}
}


void FXkHexagonalWorldVertexFactory::SetSceneProxy(FXkHexagonalWorldSceneProxy* pProxy)
{
	SceneProxy = pProxy;
	UpdateRHI();
}


bool FXkHexagonalWorldVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	const bool bIsCompatible = Parameters.MaterialParameters.MaterialDomain == MD_Surface;
	if (bIsCompatible)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	return false;
}


void FXkHexagonalWorldVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("HEXAGON_VERTEX"), 1);
	OutEnvironment.SetDefine(TEXT("QUADTREE_VERTEX"), 0);
	OutEnvironment.SetDefine(TEXT("FARMESH_VERTEX"), 0);
}


IMPLEMENT_TYPE_LAYOUT(FXkHexagonalWorldVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkHexagonalWorldVertexFactory, SF_Vertex, FXkHexagonalWorldVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkHexagonalWorldVertexFactory, SF_Compute, FXkHexagonalWorldVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkHexagonalWorldVertexFactory, SF_RayHitGroup, FXkHexagonalWorldVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_TYPE(FXkHexagonalWorldVertexFactory, "/Plugin/XkGamedevKit/Private/XkVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPrecisePrevWorldPos
	| EVertexFactoryFlags::SupportsPrimitiveIdStream
	| EVertexFactoryFlags::SupportsRayTracing
	| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
	| EVertexFactoryFlags::SupportsPSOPrecaching
);


FXkHexagonalWorldSceneProxy::FXkHexagonalWorldSceneProxy(const UXkHexagonalWorldComponent* InComponent, const FName ResourceName, 
	FMaterialRenderProxy* InBaseMaterialRenderProxy, FMaterialRenderProxy* InEdgeMaterialRenderProxy)
	:FPrimitiveSceneProxy(InComponent, ResourceName)
	, MaterialRelevance(InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
	OwnerComponent = const_cast<UXkHexagonalWorldComponent*>(InComponent);
	BaseMaterialRenderProxy = InBaseMaterialRenderProxy;
	EdgeMaterialRenderProxy = InEdgeMaterialRenderProxy;
	VertexFactory = new FXkHexagonalWorldVertexFactory(GetScene().GetFeatureLevel());

	BuildHexagon(HexagonData.BaseVertices, HexagonData.BaseIndices, HexagonData.EdgeVertices, HexagonData.EdgeIndices,
		OwnerComponent->Radius, OwnerComponent->Height, OwnerComponent->BaseInnerGap, OwnerComponent->BaseOuterGap, OwnerComponent->EdgeInnerGap, OwnerComponent->EdgeOuterGap);

	// Enqueue initialization of render resource
	BeginInitResource(&VertexPositionBuffer_GPU);
	BeginInitResource(&InstancePositionBuffer_GPU);
	BeginInitResource(&InstanceWeightBuffer_GPU);
	BeginInitResource(&BaseIndexBuffer_GPU);
	BeginInitResource(&EdgeIndexBuffer_GPU);

	GenerateBuffers();
}


FXkHexagonalWorldSceneProxy::~FXkHexagonalWorldSceneProxy()
{
	check(IsInRenderingThread());

	VertexFactory->ReleaseResource();

	VertexPositionBuffer_GPU.ReleaseResource();
	InstancePositionBuffer_GPU.ReleaseResource();
	InstanceWeightBuffer_GPU.ReleaseResource();
	BaseIndexBuffer_GPU.ReleaseResource();
	EdgeIndexBuffer_GPU.ReleaseResource();

	OwnerComponent = nullptr;
	VertexFactory = nullptr;
}


SIZE_T FXkHexagonalWorldSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}


uint32 FXkHexagonalWorldSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}


void FXkHexagonalWorldSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkHexagonalWorldSceneProxy::GetDynamicMeshElements);

	check(IsInRenderingThread());

	// Set up wire frame material (if needed)
	const bool bWireframe = AllowDebugViewmodes() && (ViewFamily.EngineShowFlags.Wireframe);
	FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
	if (bWireframe)
	{
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL, FColor::Cyan);
		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}

	const bool bShowBaseMesh = OwnerComponent->bShowBaseMesh;
	const bool bShowEdgeMesh = OwnerComponent->bShowEdgeMesh;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView& View = *Views[ViewIndex];
		int16 FrameTag = View.GetOcclusionFrameCounter() % 65535;
		FVector Position = GetActorPosition();

		int32 NunInst = OwnerComponent->HexagonalWorldNodes.Num();
		if (NunInst > 0)
		{
			const_cast<FXkHexagonalWorldSceneProxy*>(this)->UpdateInstanceBuffer(FrameTag);

			// Hexagon Base Mesh
			if (bShowBaseMesh)
			{
				FMeshBatch& Mesh = Collector.AllocateMesh();
				Mesh.bWireframe = bWireframe;
				Mesh.bUseForMaterial = true;
				Mesh.bUseWireframeSelectionColoring = IsSelected();
				Mesh.VertexFactory = VertexFactory;
				Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : BaseMaterialRenderProxy;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.NumInstances = NunInst;
				BatchElement.IndexBuffer = &BaseIndexBuffer_GPU;

				// We need the uniform buffer of this primitive because it stores the proper value for the bOutputVelocity flag.
				// The identity primitive uniform buffer simply stores false for this flag which leads to missing motion vectors.
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = BaseIndexBuffer_GPU.IndexBufferRHI->GetSize() / (3 * sizeof(uint32));
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = HexagonData.BaseVertices.Num() - 1;

				TRACE_CPUPROFILER_EVENT_SCOPE(Collector.AddMesh);
				Collector.AddMesh(ViewIndex, Mesh);
			}

			// Hexagon Edge Mesh
			if (bShowEdgeMesh)
			{
				FMeshBatch& Mesh = Collector.AllocateMesh();
				Mesh.bWireframe = bWireframe;
				Mesh.bUseForMaterial = true;
				Mesh.bUseWireframeSelectionColoring = IsSelected();
				Mesh.VertexFactory = VertexFactory;
				Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : EdgeMaterialRenderProxy;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.NumInstances = NunInst;
				BatchElement.IndexBuffer = &EdgeIndexBuffer_GPU;

				// We need the uniform buffer of this primitive because it stores the proper value for the bOutputVelocity flag.
				// The identity primitive uniform buffer simply stores false for this flag which leads to missing motion vectors.
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = EdgeIndexBuffer_GPU.IndexBufferRHI->GetSize() / (3 * sizeof(uint32));
				BatchElement.BaseVertexIndex = HexagonData.BaseVertices.Num();
				BatchElement.MinVertexIndex = HexagonData.BaseVertices.Num();
				BatchElement.MaxVertexIndex = HexagonData.BaseVertices.Num() + HexagonData.EdgeVertices.Num() - 1;

				TRACE_CPUPROFILER_EVENT_SCOPE(Collector.AddMesh);
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}
}


void FXkHexagonalWorldSceneProxy::CreateRenderThreadResources()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::CreateRenderThreadResources);
	check(IsInRenderingThread());

	VertexFactory->SetSceneProxy(this);
	VertexFactory->InitResource();
}


FPrimitiveViewRelevance FXkHexagonalWorldSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bStaticRelevance = false;
	Result.bRenderInDepthPass = false; // don't draw hexagon into depth
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}


void FXkHexagonalWorldSceneProxy::GenerateBuffers()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkHexagonalWorldSceneProxy::GenerateBuffers);

	if (HexagonData.BaseVertices.Num() && HexagonData.EdgeVertices.Num())
	{
		check(HexagonData.BaseIndices.Num());
		check(HexagonData.EdgeIndices.Num());

		FHexagonData* HexagonDataRef = new FHexagonData();
		HexagonDataRef->BaseVertices = HexagonData.BaseVertices;
		HexagonDataRef->BaseIndices = HexagonData.BaseIndices;
		HexagonDataRef->EdgeVertices = HexagonData.EdgeVertices;
		HexagonDataRef->EdgeIndices = HexagonData.EdgeIndices;

		FXkHexagonalWorldSceneProxy* SceneProxy = this;

		ENQUEUE_RENDER_COMMAND(GenerateBuffers)(
			[HexagonDataRef, SceneProxy](FRHICommandListImmediate& RHICmdList)
			{
				SceneProxy->GenerateBuffers_Renderthread(RHICmdList, HexagonDataRef);
				delete HexagonDataRef;
			});
	}
}


void FXkHexagonalWorldSceneProxy::GenerateBuffers_Renderthread(FRHICommandListImmediate& RHICmdList, FHexagonData* InHexagonData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::GenerateBuffers_Renderthread);

	check(IsInRenderingThread());

	check(IsInRenderingThread());

	if (!InHexagonData->BaseVertices.Num() || !InHexagonData->EdgeVertices.Num())
		return;

	FRHIResourceCreateInfo CreateInfo(TEXT("QuadtreeSceneProxy"));

	/** vertex buffer */
	TArray<FVector4f> BaseAndEdgeVertices;
	BaseAndEdgeVertices.Append(InHexagonData->BaseVertices);
	BaseAndEdgeVertices.Append(InHexagonData->EdgeVertices);
	int32 NumSourceVerts = BaseAndEdgeVertices.Num();
	VertexPositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		NumSourceVerts * sizeof(FVector4f),
		BUF_Static | BUF_ShaderResource, CreateInfo);
	void* RawVertexBuffer = RHILockBuffer(
		VertexPositionBuffer_GPU.VertexBufferRHI, 0,
		VertexPositionBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawVertexBuffer, &BaseAndEdgeVertices[0], NumSourceVerts * sizeof(FVector4f));
	RHIUnlockBuffer(VertexPositionBuffer_GPU.VertexBufferRHI);

	/** index buffer */
	int32 NumSourceIndices = InHexagonData->BaseIndices.Num();
	BaseIndexBuffer_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);
	/** index buffer */
	void* RawBaseIndexBuffer = RHILockBuffer(
		BaseIndexBuffer_GPU.IndexBufferRHI,
		0, NumSourceIndices * sizeof(uint32),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawBaseIndexBuffer, &InHexagonData->BaseIndices[0], NumSourceIndices * sizeof(uint32));
	RHIUnlockBuffer(BaseIndexBuffer_GPU.IndexBufferRHI);

	NumSourceIndices = InHexagonData->EdgeIndices.Num();
	EdgeIndexBuffer_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);
	/** index buffer */
	void* RawEdgeIndexBuffer = RHILockBuffer(
		EdgeIndexBuffer_GPU.IndexBufferRHI,
		0, NumSourceIndices * sizeof(uint32),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawEdgeIndexBuffer, &InHexagonData->EdgeIndices[0], NumSourceIndices * sizeof(uint32));
	RHIUnlockBuffer(EdgeIndexBuffer_GPU.IndexBufferRHI);

	/** instance position */
	InstancePositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f),
		BUF_Dynamic | BUF_ShaderResource, CreateInfo);
	void* RawInstancePositionBuffer = RHILockBuffer(
		InstancePositionBuffer_GPU.VertexBufferRHI, 0,
		InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memset((char*)RawInstancePositionBuffer, 0, MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f));
	RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

	/** instance weight, RGB for vertex color, A for splat map */
	InstanceWeightBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f),
		BUF_Dynamic | BUF_ShaderResource, CreateInfo);
	void* RawInstanceWeightBuffer = RHILockBuffer(
		InstanceWeightBuffer_GPU.VertexBufferRHI, 0,
		InstanceWeightBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memset((char*)RawInstanceWeightBuffer, 0, MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceWeightBuffer_GPU.VertexBufferRHI);
}


void FXkHexagonalWorldSceneProxy::UpdateInstanceBuffer(const int16 InFrameTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::UpdateInstanceBuffer);

	check(IsInRenderingThread());

	/** instance pos buffer */
	FRHIResourceCreateInfo CreateInfo(TEXT("UpdateInstanceBuffer"));
	// @TODO Cull
	TArray<FXkHexagonNode> HexagonalWorldNodes;
	OwnerComponent->HexagonalWorldNodes.GenerateValueArray(HexagonalWorldNodes);
	int iNunInst = HexagonalWorldNodes.Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceWeightData;

	for (int i = 0; i < iNunInst; i++)
	{
		const FXkHexagonNode& Node = HexagonalWorldNodes[i];
		FVector4f InstancePositionValue = Node.Position;
		FVector4f InstanceWeightValue = FVector4f(FVector3f(Node.Color.R, Node.Color.G, Node.Color.B), Node.Splatmap);

		InstancePositionData.Add(InstancePositionValue);
		InstanceWeightData.Add(InstanceWeightValue);
	}

	/** instance position data */
	void* RawInstancePositionData = RHILockBuffer(
		InstancePositionBuffer_GPU.VertexBufferRHI, 0,
		InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawInstancePositionData, InstancePositionData.GetData(), iNunInst * sizeof(FVector4f));
	RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

	/** instance weight data */
	void* RawInstanceWeightData = RHILockBuffer(
		InstanceWeightBuffer_GPU.VertexBufferRHI, 0,
		InstanceWeightBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawInstanceWeightData, InstanceWeightData.GetData(), iNunInst * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceWeightBuffer_GPU.VertexBufferRHI);
}