// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkLandscape/XkLandscapeSceneProxy.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
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
#include "MaterialDomain.h"
#include "DataDrivenShaderPlatformInfo.h"

static bool FreezeQuadtreeCulling = 0;
static FAutoConsoleVariableRef CVarFreezeQuadtreeCulling(
	TEXT("r.xk.FreezeQuadtreeCulling"),
	FreezeQuadtreeCulling,
	TEXT("Freeze Quadtree Culling"));

/* Bind shader parameter resource.*/
class FXkQuadtreeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FXkQuadtreeVertexFactoryShaderParameters, NonVirtual);
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


FXkQuadtreeVertexFactory::FXkQuadtreeVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	:FVertexFactory(InFeatureLevel)
{

}


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
void FXkQuadtreeVertexFactory::InitRHI()
#else
void FXkQuadtreeVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
#endif
{
	FVertexDeclarationElementList Elements;

	if (VertexPositionBuffer && InstancePositionBuffer && InstanceMorphBuffer)
	{
		FVertexStreamComponent VertexPosStream(VertexPositionBuffer, 0, sizeof(FVector4f), VET_Float4);

		Elements.Add(AccessStreamComponent(VertexPosStream, 0));

		FVertexStreamComponent PositionInstStream(InstancePositionBuffer, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(PositionInstStream, 1));

		FVertexStreamComponent MorphInstStream(InstanceMorphBuffer, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(MorphInstStream, 2));

		InitDeclaration(Elements);
	}
}


void FXkQuadtreeVertexFactory::SetVertexStreams(FVertexBuffer* InStream0, FVertexBuffer* InStream1, FVertexBuffer* InStream2)
{
	VertexPositionBuffer = InStream0;
	InstancePositionBuffer = InStream1;
	InstanceMorphBuffer = InStream2;

	UpdateRHI();
}


bool FXkQuadtreeVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	const bool bIsCompatible = Parameters.MaterialParameters.MaterialDomain == MD_Surface;
	if (bIsCompatible)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}
	return false;
}


void FXkQuadtreeVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("QUADTREE_VERTEX"), 1);
	OutEnvironment.SetDefine(TEXT("HEXAGON_VERTEX"), 0);
	OutEnvironment.SetDefine(TEXT("FARMESH_VERTEX"), 0);
}


IMPLEMENT_TYPE_LAYOUT(FXkQuadtreeVertexFactoryShaderParameters);
// ----------------------------------------------------------------------------------
// Always implement the basic vertex factory so that it's there for both editor and non-editor builds :
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_Vertex, FXkQuadtreeVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_Compute, FXkQuadtreeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_RayHitGroup, FXkQuadtreeVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
#define SHADER_PATH "/Plugin/XkGamedevKit/Private/XkVertexFactory_5_2.ush"
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 4
#define SHADER_PATH "/Plugin/XkGamedevKit/Private/XkVertexFactory_5_4.ush"
#else
#define SHADER_PATH "/Plugin/XkGamedevKit/Private/XkVertexFactory.ush"
#endif
IMPLEMENT_VERTEX_FACTORY_TYPE(FXkQuadtreeVertexFactory, SHADER_PATH,
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPrecisePrevWorldPos
	| EVertexFactoryFlags::SupportsPrimitiveIdStream
	| EVertexFactoryFlags::SupportsRayTracing
	| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
	| EVertexFactoryFlags::SupportsPSOPrecaching
);

void BuildPatch(TArray<FVector4f>& PatchPosition, TArray<uint32>& PatchIndex, const uint8 PatchSize)
{
	FVector4f v4Zero;
	v4Zero.X = 0;
	v4Zero.Y = 0;
	v4Zero.Z = 0;
	v4Zero.W = 0;

	for (int i = 0; i < PatchSize; i++)
	{
		for (int j = 0; j < PatchSize; j++)
		{
			FVector4f NewPos;
			NewPos.X = j * 1.0f / (PatchSize - 1);
			NewPos.Y = i * 1.0f / (PatchSize - 1);
			NewPos.Z = 0;
			NewPos.W = 0;
			PatchPosition.Add(NewPos);
		}
	}

	int iNumIndex = (PatchSize - 1) * (PatchSize - 1) * 2;
	for (int i = 0; i < PatchSize - 1; i++)
	{
		for (int j = 0; j < PatchSize - 1; j++)
		{
			//------
			//| 1/|
			//| / |
			//|/ 2|
			//------

			// Right Hand Coordinate Triangle 1
			PatchIndex.Add(j + (i + 1) * PatchSize);
			PatchIndex.Add(j + 1 + i * PatchSize);
			PatchIndex.Add(j + i * PatchSize);

			// Right Hand Coordinate Triangle 2
			PatchIndex.Add(j + (i + 1) * PatchSize);
			PatchIndex.Add(j + 1 + (i + 1) * PatchSize);
			PatchIndex.Add(j + 1 + i * PatchSize);
		}
	}
};


FXkQuadtreeSceneProxy::FXkQuadtreeSceneProxy(const UXkQuadtreeComponent* InComponent, const FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy)
	:FPrimitiveSceneProxy(InComponent, ResourceName)
{
	OwnerComponent = const_cast<UXkQuadtreeComponent*>(InComponent);
	VertexFactory = new FXkQuadtreeVertexFactory(GetScene().GetFeatureLevel());
	Quadtree.Initialize(512, 16);
}


FXkQuadtreeSceneProxy::~FXkQuadtreeSceneProxy()
{
	OwnerComponent = nullptr;
	VertexFactory = nullptr;
}


FXkLandscapeSceneProxy::FXkLandscapeSceneProxy(const UXkLandscapeComponent* InComponent, const FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy)
	:FXkQuadtreeSceneProxy(InComponent, ResourceName, InMaterialRenderProxy),
	MaterialRenderProxy(InMaterialRenderProxy),
	MaterialRelevance(InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel())),
	bDisableLandscapeBody(InComponent->bDisableLandscapeBody)
{
	PatchSize = 33;
	BuildPatch(PatchData.Vertices, PatchData.Indices, PatchSize);
}


FXkLandscapeSceneProxy::~FXkLandscapeSceneProxy()
{
}


SIZE_T FXkLandscapeSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}


uint32 FXkLandscapeSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}


void FXkLandscapeSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::GetDynamicMeshElements);

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

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView& View = *Views[ViewIndex];
		const_cast<FXkLandscapeSceneProxy*>(this)->UpdateBuffers(View);

		const int32 NunInst = Quadtree.GetVisibleNodes().Num();
		if (NunInst > 0)
		{
			if (bDisableLandscapeBody)
			{
				return;
			}

			// Draw the far mesh.
			FMeshBatch& Mesh = Collector.AllocateMesh();
			Mesh.bWireframe = bWireframe;
			Mesh.bUseForMaterial = true;
			Mesh.bUseWireframeSelectionColoring = IsSelected();
			Mesh.VertexFactory = VertexFactory;
			Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : MaterialRenderProxy;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.Type = PT_TriangleList;
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.bCanApplyViewModeOverrides = false;

			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.NumInstances = NunInst;
			BatchElement.IndexBuffer = &IndexBuffer_GPU;

			// We need the uniform buffer of this primitive because it stores the proper value for the bOutputVelocity flag.
			// The identity primitive uniform buffer simply stores false for this flag which leads to missing motion vectors.
			BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = IndexBuffer_GPU.IndexBufferRHI->GetSize() / (3 * sizeof(uint32));
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = VertexPositionBuffer_GPU.VertexBufferRHI->GetSize() - 1;

			TRACE_CPUPROFILER_EVENT_SCOPE(Collector.AddMesh);
			Collector.AddMesh(ViewIndex, Mesh);
		}
		break;
	}
}


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
void FXkLandscapeSceneProxy::CreateRenderThreadResources()
#else
void FXkLandscapeSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
#endif
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeSceneProxy::CreateRenderThreadResources);
	check(IsInRenderingThread());

	// Enqueue initialization of render resource
	BeginInitResource(&VertexPositionBuffer_GPU);
	BeginInitResource(&InstancePositionBuffer_GPU);
	BeginInitResource(&InstanceMorphBuffer_GPU);
	BeginInitResource(&IndexBuffer_GPU);

	GenerateBuffers();

	VertexFactory->SetVertexStreams(&VertexPositionBuffer_GPU, &InstancePositionBuffer_GPU, &InstanceMorphBuffer_GPU);
	VertexFactory->InitResource();
}


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
void FXkLandscapeSceneProxy::DestroyRenderThreadResources()
#else
void FXkLandscapeSceneProxy::DestroyRenderThreadResources(FRHICommandListBase& RHICmdList)
#endif
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeSceneProxy::DestroyRenderThreadResources);
	check(IsInRenderingThread());

	VertexPositionBuffer_GPU.ReleaseResource();
	InstancePositionBuffer_GPU.ReleaseResource();
	InstanceMorphBuffer_GPU.ReleaseResource();
	IndexBuffer_GPU.ReleaseResource();

	VertexFactory->ReleaseResource();
}


FPrimitiveViewRelevance FXkLandscapeSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = bCastDynamicShadow;
	Result.bStaticRelevance = bCastStaticShadow;
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bRenderInDepthPass = ShouldRenderInDepthPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	// @Note: To render Blend Mode - Translucent 
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}


void FXkLandscapeSceneProxy::GenerateBuffers()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeSceneProxy::GenerateBuffers);

	if (PatchData.Vertices.Num())
	{
		check(PatchData.Vertices.Num());
		check(PatchData.Indices.Num());

		FPatchData* PatchDataRef = new FPatchData();
		PatchDataRef->Vertices = PatchData.Vertices;
		PatchDataRef->Indices = PatchData.Indices;

		ENQUEUE_RENDER_COMMAND(GenerateBuffers)(
			[this, PatchDataRef](FRHICommandListImmediate& RHICmdList)
			{
				if (!PatchDataRef->Vertices.Num())
					return;

				FRHIResourceCreateInfo CreateInfo(TEXT("FXkLandscapeSceneProxy::GenerateBuffers_Renderthread"));

				/** vertex buffer */
				int32 NumSourceVerts = PatchDataRef->Vertices.Num();
				VertexPositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					NumSourceVerts * sizeof(FVector4f),
					BUF_Static | BUF_ShaderResource, CreateInfo);
				void* RawVertexBuffer = RHILockBuffer(
					VertexPositionBuffer_GPU.VertexBufferRHI, 0,
					VertexPositionBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memcpy((char*)RawVertexBuffer, &PatchDataRef->Vertices[0], NumSourceVerts * sizeof(FVector4f));
				RHIUnlockBuffer(VertexPositionBuffer_GPU.VertexBufferRHI);

				/** index buffer */
				int32 NumSourceIndices = PatchDataRef->Indices.Num();
				IndexBuffer_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);
				/** index buffer */
				void* RawIndexBuffer = RHILockBuffer(
					IndexBuffer_GPU.IndexBufferRHI,
					0, NumSourceIndices * sizeof(uint32),
					RLM_WriteOnly);
				FMemory::Memcpy((char*)RawIndexBuffer, &PatchDataRef->Indices[0], NumSourceIndices * sizeof(uint32));
				RHIUnlockBuffer(IndexBuffer_GPU.IndexBufferRHI);

				/** instance position */
				InstancePositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f),
					BUF_Dynamic | BUF_ShaderResource, CreateInfo);
				void* RawInstancePositionBuffer = RHILockBuffer(
					InstancePositionBuffer_GPU.VertexBufferRHI, 0,
					InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memset((char*)RawInstancePositionBuffer, 0, MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f));
				RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

				/** instance vertex morph */
				InstanceMorphBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f),
					BUF_Dynamic | BUF_ShaderResource, CreateInfo);
				void* RawInstanceMorphBuffer = RHILockBuffer(
					InstanceMorphBuffer_GPU.VertexBufferRHI, 0,
					InstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memset((char*)RawInstanceMorphBuffer, 0, MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f));
				RHIUnlockBuffer(InstanceMorphBuffer_GPU.VertexBufferRHI);

				// Important: delete the patch data after use to avoid memory leak
				delete PatchDataRef;
			});
	}
}


void FXkLandscapeSceneProxy::UpdateBuffers(const FSceneView& View)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeSceneProxy::UpdateBuffers);

	int16 FrameTag = View.GetOcclusionFrameCounter() % 65535;
	FVector Position = GetActorPosition();

	if (!FreezeQuadtreeCulling)
	{
		Quadtree.Cull(&View.ViewFrustum, View.ViewLocation, Position, FrameTag);
	}

	/** instance pos buffer */
	int iNunInst = Quadtree.GetVisibleNodes().Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceMorphData;

	FVector3f RootOffset = FVector3f(Quadtree.GetRootOffset());

	for (int i = 0; i < iNunInst; i++)
	{
		int32 iTreeIndex = Quadtree.GetVisibleNodes()[i];
		const FQuadtreeNode& QuadtreeNode = Quadtree.GetTreeNodes()[iTreeIndex];
		FVector3f Extent3D = FVector3f(QuadtreeNode.GetNodeBox().GetExtent());
		FVector2f vExtent = FVector2f(Extent3D.X, Extent3D.Y);

		FVector4f InstancePositionValue;
		FVector4f InstanceMorphValue;

		InstancePositionValue.X = QuadtreeNode.GetNodeBox().Min.X + RootOffset.X;
		InstancePositionValue.Y = QuadtreeNode.GetNodeBox().Min.Y + RootOffset.Y;
		InstancePositionValue.Z = (QuadtreeNode.GetNodeBox().Min.Z + QuadtreeNode.GetNodeBox().Max.Z) / 2.0;
		InstancePositionValue.W = vExtent.X * 2.0;

		// LOD Level
		InstanceMorphValue.X = Quadtree.GetMaxDepth() - QuadtreeNode.GetNodeDepth() - 1;
		// LOD Scale
		InstanceMorphValue.Y = Quadtree.GetMinNodeSize() * FQuadtree::UnrealUnitScale;
		// Quad Size
		InstanceMorphValue.Z = PatchSize - 1;
		// Node Depth
		InstanceMorphValue.W = QuadtreeNode.GetNodeDepth();

		InstancePositionData.Add(InstancePositionValue);
		InstanceMorphData.Add(InstanceMorphValue);
	}

	ENQUEUE_RENDER_COMMAND(UpdateBuffers)(
		[this, InstancePositionData, InstanceMorphData, iNunInst](FRHICommandListImmediate& RHICmdList)
		{
			/** instance position data */
			void* RawInstancePositionData = RHILockBuffer(
				InstancePositionBuffer_GPU.VertexBufferRHI, 0,
				InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstancePositionData, InstancePositionData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

			/** instance morph data */
			void* RawInstanceMorphData = RHILockBuffer(
				InstanceMorphBuffer_GPU.VertexBufferRHI, 0,
				InstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstanceMorphData, InstanceMorphData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(InstanceMorphBuffer_GPU.VertexBufferRHI);
		});
}


FXkLandscapeWithWaterSceneProxy::FXkLandscapeWithWaterSceneProxy(const UXkLandscapeWithWaterComponent* InComponent, const FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy, FMaterialRenderProxy* InWaterMaterialRenderProxy)
	:FXkLandscapeSceneProxy(InComponent, ResourceName, InMaterialRenderProxy),
	WaterMaterialRenderProxy(InWaterMaterialRenderProxy),
	WaterMaterialRelevance(InComponent->GetWaterMaterialRelevance(GetScene().GetFeatureLevel())),
	bDisableWaterBody(InComponent->bDisableWaterBody)
{
	WaterVertexFactory = new FXkQuadtreeVertexFactory(GetScene().GetFeatureLevel());

	WaterPatchSize = 33;
	BuildPatch(WaterPatchData.Vertices, WaterPatchData.Indices, WaterPatchSize);
}


FXkLandscapeWithWaterSceneProxy::~FXkLandscapeWithWaterSceneProxy()
{
}


void FXkLandscapeWithWaterSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeWithWaterSceneProxy::GetDynamicMeshElements);

	check(IsInRenderingThread());

	FXkLandscapeSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);

	if (bDisableWaterBody)
	{
		return;
	}

	// Set up wire frame material (if needed)
	const bool bWireframe = AllowDebugViewmodes() && (ViewFamily.EngineShowFlags.Wireframe);
	FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
	if (bWireframe)
	{
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL, FColor::Cyan);
		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView& View = *Views[ViewIndex];

		int32 NunInst = Quadtree.GetVisibleNodes().Num();
		if (NunInst > 0)
		{
			// Draw the far mesh.
			FMeshBatch& Mesh = Collector.AllocateMesh();
			Mesh.bWireframe = bWireframe;
			Mesh.bUseForMaterial = true;
			Mesh.bUseWireframeSelectionColoring = IsSelected();
			Mesh.VertexFactory = WaterVertexFactory;
			Mesh.MaterialRenderProxy = (WireframeMaterialInstance != nullptr) ? WireframeMaterialInstance : WaterMaterialRenderProxy;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.Type = PT_TriangleList;
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.bCanApplyViewModeOverrides = false;

			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.NumInstances = NunInst;
			BatchElement.IndexBuffer = &WaterIndexBuffer_GPU;

			// We need the uniform buffer of this primitive because it stores the proper value for the bOutputVelocity flag.
			// The identity primitive uniform buffer simply stores false for this flag which leads to missing motion vectors.
			BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = WaterIndexBuffer_GPU.IndexBufferRHI->GetSize() / (3 * sizeof(uint32));
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = WaterVertexPositionBuffer_GPU.VertexBufferRHI->GetSize() - 1;

			TRACE_CPUPROFILER_EVENT_SCOPE(Collector.AddMesh);
			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
void FXkLandscapeWithWaterSceneProxy::CreateRenderThreadResources()
#else
void FXkLandscapeWithWaterSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
#endif
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::CreateRenderThreadResources);
	check(IsInRenderingThread());

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
	FXkLandscapeSceneProxy::CreateRenderThreadResources();
#else
	FXkLandscapeSceneProxy::CreateRenderThreadResources(RHICmdList);
#endif
	// Enqueue initialization of render resource
	BeginInitResource(&WaterVertexPositionBuffer_GPU);
	BeginInitResource(&WaterInstancePositionBuffer_GPU);
	BeginInitResource(&WaterInstanceMorphBuffer_GPU);
	BeginInitResource(&WaterIndexBuffer_GPU);

	GenerateBuffers();

	WaterVertexFactory->SetVertexStreams(&WaterVertexPositionBuffer_GPU, &WaterInstancePositionBuffer_GPU, &WaterInstanceMorphBuffer_GPU);
	WaterVertexFactory->InitResource();
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
void FXkLandscapeWithWaterSceneProxy::DestroyRenderThreadResources()
#else
void FXkLandscapeWithWaterSceneProxy::DestroyRenderThreadResources(FRHICommandListBase& RHICmdList)
#endif
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::DestroyRenderThreadResources);
	check(IsInRenderingThread());

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2
	FXkLandscapeSceneProxy::DestroyRenderThreadResources();
#else
	FXkLandscapeSceneProxy::DestroyRenderThreadResources(RHICmdList);
#endif

	WaterVertexPositionBuffer_GPU.ReleaseResource();
	WaterInstancePositionBuffer_GPU.ReleaseResource();
	WaterInstanceMorphBuffer_GPU.ReleaseResource();
	WaterIndexBuffer_GPU.ReleaseResource();

	WaterVertexFactory->ReleaseResource();
}


FPrimitiveViewRelevance FXkLandscapeWithWaterSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result = Super::GetViewRelevance(View);
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = bCastDynamicShadow;
	Result.bStaticRelevance = bCastStaticShadow;
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bRenderInDepthPass = ShouldRenderInDepthPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	WaterMaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	return Result;
}


void FXkLandscapeWithWaterSceneProxy::GenerateBuffers()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeWithWaterSceneProxy::GenerateBuffers);

	FXkLandscapeSceneProxy::GenerateBuffers();

	if (WaterPatchData.Vertices.Num())
	{
		check(WaterPatchData.Vertices.Num());
		check(WaterPatchData.Indices.Num());

		FPatchData* PatchDataRef = new FPatchData();
		PatchDataRef->Vertices = WaterPatchData.Vertices;
		PatchDataRef->Indices = WaterPatchData.Indices;

		ENQUEUE_RENDER_COMMAND(GenerateBuffers)(
			[this, PatchDataRef](FRHICommandListImmediate& RHICmdList)
			{
				if (!PatchDataRef->Vertices.Num())
					return;

				FRHIResourceCreateInfo CreateInfo(TEXT("XkLandscapeWithWaterSceneProxy"));

				/** vertex buffer */
				int32 NumSourceVerts = PatchDataRef->Vertices.Num();
				WaterVertexPositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					NumSourceVerts * sizeof(FVector4f),
					BUF_Static | BUF_ShaderResource, CreateInfo);
				void* RawVertexBuffer = RHILockBuffer(
					WaterVertexPositionBuffer_GPU.VertexBufferRHI, 0,
					WaterVertexPositionBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memcpy((char*)RawVertexBuffer, &PatchDataRef->Vertices[0], NumSourceVerts * sizeof(FVector4f));
				RHIUnlockBuffer(WaterVertexPositionBuffer_GPU.VertexBufferRHI);

				/** index buffer */
				int32 NumSourceIndices = PatchDataRef->Indices.Num();
				WaterIndexBuffer_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);
				/** index buffer */
				void* RawIndexBuffer = RHILockBuffer(
					WaterIndexBuffer_GPU.IndexBufferRHI,
					0, NumSourceIndices * sizeof(uint32),
					RLM_WriteOnly);
				FMemory::Memcpy((char*)RawIndexBuffer, &PatchDataRef->Indices[0], NumSourceIndices * sizeof(uint32));
				RHIUnlockBuffer(WaterIndexBuffer_GPU.IndexBufferRHI);

				/** instance position */
				WaterInstancePositionBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f),
					BUF_Dynamic | BUF_ShaderResource, CreateInfo);
				void* RawInstancePositionBuffer = RHILockBuffer(
					WaterInstancePositionBuffer_GPU.VertexBufferRHI, 0,
					WaterInstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memset((char*)RawInstancePositionBuffer, 0, MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f));
				RHIUnlockBuffer(WaterInstancePositionBuffer_GPU.VertexBufferRHI);

				/** instance vertex morph */
				WaterInstanceMorphBuffer_GPU.VertexBufferRHI = RHICreateVertexBuffer(
					MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f),
					BUF_Dynamic | BUF_ShaderResource, CreateInfo);
				void* RawInstanceMorphBuffer = RHILockBuffer(
					WaterInstanceMorphBuffer_GPU.VertexBufferRHI, 0,
					WaterInstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
					RLM_WriteOnly);
				FMemory::Memset((char*)RawInstanceMorphBuffer, 0, MAX_VISIBLE_NODE_COUNT * sizeof(FVector4f));
				RHIUnlockBuffer(WaterInstanceMorphBuffer_GPU.VertexBufferRHI);

				delete PatchDataRef;
			});
	}
}


void FXkLandscapeWithWaterSceneProxy::UpdateBuffers(const FSceneView& View)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkLandscapeWithWaterSceneProxy::UpdateBuffers);

	FXkLandscapeSceneProxy::UpdateBuffers(View);

	/** instance pos buffer */
	int iNunInst = Quadtree.GetVisibleNodes().Num();
	TArray<FVector4f> WaterInstancePositionData;
	TArray<FVector4f> WaterInstanceMorphData;

	FVector3f RootOffset = FVector3f(Quadtree.GetRootOffset());

	for (int i = 0; i < iNunInst; i++)
	{
		int32 iTreeIndex = Quadtree.GetVisibleNodes()[i];
		const FQuadtreeNode& QuadtreeNode = Quadtree.GetTreeNodes()[iTreeIndex];
		FVector3f Extent3D = FVector3f(QuadtreeNode.GetNodeBox().GetExtent());
		FVector2f vExtent = FVector2f(Extent3D.X, Extent3D.Y);

		FVector4f WaterInstancePositionValue;
		FVector4f WaterInstanceMorphValue;

		WaterInstancePositionValue.X = QuadtreeNode.GetNodeBox().Min.X + RootOffset.X;
		WaterInstancePositionValue.Y = QuadtreeNode.GetNodeBox().Min.Y + RootOffset.Y;
		WaterInstancePositionValue.Z = 0.0;
		WaterInstancePositionValue.W = vExtent.X * 2.0;

		// LOD Level
		WaterInstanceMorphValue.X = Quadtree.GetMaxDepth() - QuadtreeNode.GetNodeDepth() - 1;
		// LOD Scale
		WaterInstanceMorphValue.Y = Quadtree.GetMinNodeSize() * FQuadtree::UnrealUnitScale;
		// Quad Size
		WaterInstanceMorphValue.Z = WaterPatchSize - 1;
		// Node Depth
		WaterInstanceMorphValue.W = QuadtreeNode.GetNodeDepth();

		WaterInstancePositionData.Add(WaterInstancePositionValue);
		WaterInstanceMorphData.Add(WaterInstanceMorphValue);
	}

	ENQUEUE_RENDER_COMMAND(UpdateBuffers)(
		[this, WaterInstancePositionData, WaterInstanceMorphData, iNunInst](FRHICommandListImmediate& RHICmdList)
		{
			/** instance position data */
			void* RawInstancePositionData = RHILockBuffer(
				WaterInstancePositionBuffer_GPU.VertexBufferRHI, 0,
				WaterInstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstancePositionData, WaterInstancePositionData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(WaterInstancePositionBuffer_GPU.VertexBufferRHI);

			/** instance morph data */
			void* RawInstanceMorphData = RHILockBuffer(
				WaterInstanceMorphBuffer_GPU.VertexBufferRHI, 0,
				WaterInstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstanceMorphData, WaterInstanceMorphData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(WaterInstanceMorphBuffer_GPU.VertexBufferRHI);
		});
}


FXkSphericalLandscapeWithWaterSceneProxy::FXkSphericalLandscapeWithWaterSceneProxy(const UXkSphericalLandscapeWithWaterComponent* InComponent, const FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy, FMaterialRenderProxy* InWaterMaterialRenderProxy)
	:FXkLandscapeWithWaterSceneProxy(InComponent, ResourceName, InMaterialRenderProxy, InWaterMaterialRenderProxy)
{
}


FXkSphericalLandscapeWithWaterSceneProxy::~FXkSphericalLandscapeWithWaterSceneProxy()
{
}


void FXkSphericalLandscapeWithWaterSceneProxy::UpdateBuffers(const FSceneView& View)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkSphericalLandscapeWithWaterSceneProxy::UpdateBuffers);

	Quadtree.InitProcessFunc([this](FQuadtreeNode& OutNode, const FVector& InCameraPos, const int32 InNodeID)
		{
			// "/Plugin/XkGamedevKit/Public/MaterialExpressions.ush" : XkSphericalWorldBending
			if (OutNode.GetNodeDepth() != 0)
			{
				const FVector WorldPosition = OutNode.GetNodeBox().GetCenter() + Quadtree.GetRootOffset();
				float Z = SphericalHeight(InCameraPos, WorldPosition);
				FVector BoxMin = OutNode.GetNodeBox().Min;
				BoxMin.Z = Z;
				FVector BoxMax = OutNode.GetNodeBox().Max;
				//BoxMax.Z = OutNode.GetNodeBox().GetExtent().X / 4.0 + y;
				OutNode.GetNodeBoxRaw().Min = BoxMin;
				OutNode.GetNodeBoxRaw().Max = BoxMax;
			}
		});

	int16 FrameTag = View.GetOcclusionFrameCounter() % 65535;
	FVector Position = GetActorPosition();

	if (!FreezeQuadtreeCulling)
	{
		Quadtree.Cull(&View.ViewFrustum, View.ViewLocation, Position, FrameTag);
	}

	/** instance pos buffer */
	int iNunInst = Quadtree.GetVisibleNodes().Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceMorphData;
	TArray<FVector4f> WaterInstancePositionData;
	TArray<FVector4f> WaterInstanceMorphData;

	FVector3f RootOffset = FVector3f(Quadtree.GetRootOffset());

	for (int i = 0; i < iNunInst; i++)
	{
		int32 iTreeIndex = Quadtree.GetVisibleNodes()[i];
		const FQuadtreeNode& QuadtreeNode = Quadtree.GetTreeNodes()[iTreeIndex];
		FVector3f Extent3D = FVector3f(QuadtreeNode.GetNodeBox().GetExtent());
		FVector2f vExtent = FVector2f(Extent3D.X, Extent3D.Y);

		FVector4f InstancePositionValue;
		FVector4f InstanceMorphValue;
		FVector4f WaterInstancePositionValue;
		FVector4f WaterInstanceMorphValue;

		InstancePositionValue.X = QuadtreeNode.GetNodeBox().Min.X + RootOffset.X;
		InstancePositionValue.Y = QuadtreeNode.GetNodeBox().Min.Y + RootOffset.Y;
		InstancePositionValue.Z = 0.0;
		InstancePositionValue.W = vExtent.X * 2.0;
		WaterInstancePositionValue = InstancePositionValue;

		// LOD Level
		InstanceMorphValue.X = Quadtree.GetMaxDepth() - QuadtreeNode.GetNodeDepth() - 1;
		WaterInstanceMorphValue.X = Quadtree.GetMaxDepth() - QuadtreeNode.GetNodeDepth() - 1;
		// LOD Scale
		InstanceMorphValue.Y = Quadtree.GetMinNodeSize() * FQuadtree::UnrealUnitScale;
		WaterInstanceMorphValue.Y = Quadtree.GetMinNodeSize() * FQuadtree::UnrealUnitScale;
		// Quad Size
		InstanceMorphValue.Z = PatchSize - 1;
		WaterInstanceMorphValue.Z = WaterPatchSize - 1;
		// Node Depth
		InstanceMorphValue.W = QuadtreeNode.GetNodeDepth();
		WaterInstanceMorphValue.W = QuadtreeNode.GetNodeDepth();

		InstancePositionData.Add(InstancePositionValue);
		InstanceMorphData.Add(InstanceMorphValue);
		WaterInstancePositionData.Add(WaterInstancePositionValue);
		WaterInstanceMorphData.Add(WaterInstanceMorphValue);
	}

	ENQUEUE_RENDER_COMMAND(UpdateBuffers)(
		[this, InstancePositionData, InstanceMorphData, WaterInstancePositionData, WaterInstanceMorphData, iNunInst](FRHICommandListImmediate& RHICmdList)
		{
			/** instance position data */
			void* RawInstancePositionData = RHILockBuffer(
				InstancePositionBuffer_GPU.VertexBufferRHI, 0,
				InstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstancePositionData, InstancePositionData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(InstancePositionBuffer_GPU.VertexBufferRHI);

			/** instance morph data */
			void* RawInstanceMorphData = RHILockBuffer(
				InstanceMorphBuffer_GPU.VertexBufferRHI, 0,
				InstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawInstanceMorphData, InstanceMorphData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(InstanceMorphBuffer_GPU.VertexBufferRHI);

			/** instance position data */
			void* RawWaterInstancePositionData = RHILockBuffer(
				WaterInstancePositionBuffer_GPU.VertexBufferRHI, 0,
				WaterInstancePositionBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawWaterInstancePositionData, WaterInstancePositionData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(WaterInstancePositionBuffer_GPU.VertexBufferRHI);

			/** instance morph data */
			void* RawWaterInstanceMorphData = RHILockBuffer(
				WaterInstanceMorphBuffer_GPU.VertexBufferRHI, 0,
				WaterInstanceMorphBuffer_GPU.VertexBufferRHI->GetSize(),
				RLM_WriteOnly);
			FMemory::Memcpy((char*)RawWaterInstanceMorphData, WaterInstanceMorphData.GetData(), iNunInst * sizeof(FVector4f));
			RHIUnlockBuffer(WaterInstanceMorphBuffer_GPU.VertexBufferRHI);
		});
}