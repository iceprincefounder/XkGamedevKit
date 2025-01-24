// Copyright ©ICEPRINCE. All Rights Reserved.


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
	VertexPositionVertexBuffer = NULL;
	InstancePositionVertexBuffer = NULL;
	VertexPositionVertexBuffer = NULL;
}


void FXkHexagonalWorldVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	if (VertexPositionVertexBuffer && InstancePositionVertexBuffer && InstanceColorVertexBuffer)
	{
		FVertexStreamComponent VertexPosStream(VertexPositionVertexBuffer, 0, sizeof(FVector4f), VET_Float4);

		Elements.Add(AccessStreamComponent(VertexPosStream, 0));

		FVertexStreamComponent PositionInstStream(InstancePositionVertexBuffer, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(PositionInstStream, 1));

		FVertexStreamComponent WeightInstStream(InstanceColorVertexBuffer, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(WeightInstStream, 2));

		InitDeclaration(Elements);
	}
}


void FXkHexagonalWorldVertexFactory::SetVertexBuffer(FVertexBuffer* InData0, FVertexBuffer* InData1, FVertexBuffer* InData2)
{
	VertexPositionVertexBuffer = InData0;
	InstancePositionVertexBuffer = InData1;
	InstanceColorVertexBuffer = InData2;
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
	BaseVertexFactory = new FXkHexagonalWorldVertexFactory(GetScene().GetFeatureLevel());
	EdgeVertexFactory = new FXkHexagonalWorldVertexFactory(GetScene().GetFeatureLevel());

	BuildHexagon(HexagonData.BaseVertices, HexagonData.BaseIndices, HexagonData.EdgeVertices, HexagonData.EdgeIndices,
		OwnerComponent->Radius, OwnerComponent->Height, OwnerComponent->BaseInnerGap, OwnerComponent->BaseOuterGap, OwnerComponent->EdgeInnerGap, OwnerComponent->EdgeOuterGap);

	// Enqueue initialization of render resource
	BeginInitResource(&VertexPositionBuffer_GPU);
	BeginInitResource(&InstancePositionBuffer_GPU);
	BeginInitResource(&InstanceBaseColorBuffer_GPU);
	BeginInitResource(&InstanceEdgeColorBuffer_GPU);
	BeginInitResource(&BaseIndexBuffer_GPU);
	BeginInitResource(&EdgeIndexBuffer_GPU);

	GenerateBuffers();
}


FXkHexagonalWorldSceneProxy::~FXkHexagonalWorldSceneProxy()
{
	check(IsInRenderingThread());

	BaseVertexFactory->ReleaseResource();
	EdgeVertexFactory->ReleaseResource();

	VertexPositionBuffer_GPU.ReleaseResource();
	InstancePositionBuffer_GPU.ReleaseResource();
	InstanceBaseColorBuffer_GPU.ReleaseResource();
	InstanceEdgeColorBuffer_GPU.ReleaseResource();
	BaseIndexBuffer_GPU.ReleaseResource();
	EdgeIndexBuffer_GPU.ReleaseResource();

	OwnerComponent = nullptr;
	BaseVertexFactory = nullptr;
	EdgeVertexFactory = nullptr;
}


//void FXkHexagonalWorldSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
//{
//	FMeshBatch MeshBatch;
//	MeshBatch.LODIndex = 0;
//	MeshBatch.SegmentIndex = 0;
//
//	MeshBatch.VertexFactory = BaseVertexFactory;
//	MeshBatch.Type = PT_TriangleList;
//
//	MeshBatch.LODIndex = 0;
//	MeshBatch.SegmentIndex = 0;
//
//	MeshBatch.bDitheredLODTransition = false;
//	MeshBatch.bWireframe = false;
//	MeshBatch.CastShadow = false;
//	MeshBatch.bUseForDepthPass = false;
//	MeshBatch.bUseAsOccluder = false;
//	MeshBatch.bUseForMaterial = false;
//	MeshBatch.bRenderToVirtualTexture = true;
//	MeshBatch.RuntimeVirtualTextureMaterialType = (int32)ERuntimeVirtualTextureMaterialType::BaseColor;
//	MeshBatch.MaterialRenderProxy = BaseMaterialRenderProxy;
//	MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
//
//	MeshBatch.DepthPriorityGroup = SDPG_World;
//	MeshBatch.bCanApplyViewModeOverrides = false;
//
//	MeshBatch.Elements.Empty(1);
//	FMeshBatchElement BatchElement;
//
//	int32 NunInst = VisibleNodes.Num();
//
//	BatchElement.NumInstances = NunInst;
//	BatchElement.IndexBuffer = &BaseIndexBuffer_GPU;
//
//	// We need the uniform buffer of this primitive because it stores the proper value for the bOutputVelocity flag.
//	// The identity primitive uniform buffer simply stores false for this flag which leads to missing motion vectors.
//	BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
//
//	BatchElement.FirstIndex = 0;
//	BatchElement.NumPrimitives = BaseIndexBuffer_GPU.IndexBufferRHI->GetSize() / (3 * sizeof(uint32));
//	BatchElement.MinVertexIndex = 0;
//	BatchElement.MaxVertexIndex = HexagonData.BaseVertices.Num() - 1;
//
//	PDI->DrawMesh(MeshBatch, FLT_MAX);
//}


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

		const_cast<FXkHexagonalWorldSceneProxy*>(this)->UpdateInstanceBuffer(FrameTag);
		int32 NunInst = VisibleNodes.Num();
		if (NunInst > 0)
		{
			// Hexagon Base Mesh
			if (bShowBaseMesh)
			{
				FMeshBatch& Mesh = Collector.AllocateMesh();
				Mesh.bWireframe = bWireframe;
				Mesh.bUseForMaterial = true;
				Mesh.bUseWireframeSelectionColoring = IsSelected();
				Mesh.VertexFactory = BaseVertexFactory;
				Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : BaseMaterialRenderProxy;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Mesh.bRenderToVirtualTexture = true;
				Mesh.RuntimeVirtualTextureMaterialType = (int32)ERuntimeVirtualTextureMaterialType::BaseColor;

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
				Mesh.VertexFactory = EdgeVertexFactory;
				Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : EdgeMaterialRenderProxy;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Mesh.bRenderToVirtualTexture = true;
				Mesh.RuntimeVirtualTextureMaterialType = (int32)ERuntimeVirtualTextureMaterialType::BaseColor;

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

	BaseVertexFactory->SetVertexBuffer(&VertexPositionBuffer_GPU, &InstancePositionBuffer_GPU, &InstanceBaseColorBuffer_GPU);
	BaseVertexFactory->InitResource();
	EdgeVertexFactory->SetVertexBuffer(&VertexPositionBuffer_GPU, &InstancePositionBuffer_GPU, &InstanceEdgeColorBuffer_GPU);
	EdgeVertexFactory->InitResource();
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

	/** instance base color, RGBA for vertex color */
	InstanceBaseColorBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f),
		BUF_Dynamic | BUF_ShaderResource, CreateInfo);
	void* RawInstanceBaseColorBuffer = RHILockBuffer(
		InstanceBaseColorBuffer_GPU.VertexBufferRHI, 0,
		InstanceBaseColorBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memset((char*)RawInstanceBaseColorBuffer, 0, MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceBaseColorBuffer_GPU.VertexBufferRHI);

	/** instance edge color, RGBA for vertex color */
	InstanceEdgeColorBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f),
		BUF_Dynamic | BUF_ShaderResource, CreateInfo);
	void* RawInstanceEdgeColorBuffer = RHILockBuffer(
		InstanceEdgeColorBuffer_GPU.VertexBufferRHI, 0,
		InstanceEdgeColorBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memset((char*)RawInstanceEdgeColorBuffer, 0, MAX_HEXAGON_NODE_COUNT * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceEdgeColorBuffer_GPU.VertexBufferRHI);
}


void FXkHexagonalWorldSceneProxy::UpdateInstanceBuffer(const int16 InFrameTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::UpdateInstanceBuffer);

	check(IsInRenderingThread());

	/** instance pos buffer */
	FRHIResourceCreateInfo CreateInfo(TEXT("UpdateInstanceBuffer"));
	// @TODO Cull
	TArray<FXkHexagonNode> AllHexagonalWorldNodes;
	OwnerComponent->ModifyHexagonalWorldNodes().GenerateValueArray(AllHexagonalWorldNodes);
	VisibleNodes.Empty();
	for (int i = 0; i < AllHexagonalWorldNodes.Num(); i++)
	{
		const FXkHexagonNode& Node = AllHexagonalWorldNodes[i];
		if (Node.Type == EXkHexagonType::Unavailable)
		{
			continue;
		}
		VisibleNodes.Add(Node.Coord);
	}

	if (VisibleNodes.Num() == 0)
	{
		return;
	}

	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceBaseColorData;
	TArray<FVector4f> InstanceEdgeColorData;

	for (int i = 0; i < VisibleNodes.Num(); i++)
	{
		FIntVector Coord = VisibleNodes[i];
		const FXkHexagonNode& Node = OwnerComponent->ModifyHexagonalWorldNodes()[Coord];
		FVector4f InstancePositionValue = Node.Position;
		FVector4f InstanceBaseColorValue = FVector4f(1.0);
		FVector4f InstanceEdgeColorValue = FVector4f(1.0);

		InstancePositionData.Add(InstancePositionValue);
		InstanceBaseColorData.Add(InstanceBaseColorValue);
		InstanceEdgeColorData.Add(InstanceEdgeColorValue);
	}

	/** instance position data */
	void* RawInstancePositionData = RHILockBuffer(
		InstancePositionBuffer_GPU.VertexBufferRHI, 0,
		InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawInstancePositionData, InstancePositionData.GetData(), InstancePositionData.Num() * sizeof(FVector4f));
	RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

	/** instance base color data */
	void* RawInstanceBaseColorData = RHILockBuffer(
		InstanceBaseColorBuffer_GPU.VertexBufferRHI, 0,
		InstanceBaseColorBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawInstanceBaseColorData, InstanceBaseColorData.GetData(), InstanceBaseColorData.Num() * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceBaseColorBuffer_GPU.VertexBufferRHI);

	/** instance edge color data */
	void* RawInstanceEdgeColorData = RHILockBuffer(
		InstanceEdgeColorBuffer_GPU.VertexBufferRHI, 0,
		InstanceEdgeColorBuffer_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawInstanceEdgeColorData, InstanceEdgeColorData.GetData(), InstanceEdgeColorData.Num() * sizeof(FVector4f));
	RHIUnlockBuffer(InstanceEdgeColorBuffer_GPU.VertexBufferRHI);
}