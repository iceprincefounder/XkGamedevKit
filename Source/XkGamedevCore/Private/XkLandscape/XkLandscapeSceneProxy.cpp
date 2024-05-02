// Copyright ©xukai. All Rights Reserved.


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

static bool FreezeQuadtreeCulling = 0;
static FAutoConsoleVariableRef CVarFreezeQuadtreeCulling(
	TEXT("r.xk.FreezeQuadtreeCulling"),
	FreezeQuadtreeCulling,
	TEXT("Freeze Quadtree Culling"));

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
		const FXkQuadtreeVertexFactory* QuadtreeVertexFactory = static_cast<const FXkQuadtreeVertexFactory*>(VertexFactory);

		FVertexBuffer* VertexBuffer = &(QuadtreeVertexFactory->SceneProxy->InstancePositionBuffer_GPU);
		check(VertexBuffer);

		if (VertexBuffer)
		{
			FVertexInputStream inStream(1, 0, QuadtreeVertexFactory->SceneProxy->InstancePositionBuffer_GPU.VertexBufferRHI);
		}
	}
};


FXkQuadtreeVertexFactory::FXkQuadtreeVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	:FVertexFactory(InFeatureLevel)
{
	SceneProxy = NULL;
}


void FXkQuadtreeVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	if (SceneProxy)
	{
		FVertexStreamComponent VertexPosStream(&SceneProxy->VertexPositionData_GPU, 0, sizeof(FVector4f), VET_Float4);

		Elements.Add(AccessStreamComponent(VertexPosStream, 0));

		FVertexStreamComponent PositionInstStream(&SceneProxy->InstancePositionBuffer_GPU, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(PositionInstStream, 1));

		FVertexStreamComponent MorphInstStream(&SceneProxy->InstanceMorphBuffer_GPU, 0, sizeof(FVector4f), VET_Float4, EVertexStreamUsage::Instancing);

		Elements.Add(AccessStreamComponent(MorphInstStream, 2));

		InitDeclaration(Elements);
	}
}


void FXkQuadtreeVertexFactory::SetSceneProxy(FXkQuadtreeSceneProxy* pProxy)
{
	SceneProxy = pProxy;
	UpdateRHI();
}


bool FXkQuadtreeVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
}


void FXkQuadtreeVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("FAR_MESH"), 0);
}

IMPLEMENT_TYPE_LAYOUT(FXkQuadtreeVertexFactoryShaderParameters);
// ----------------------------------------------------------------------------------
// Always implement the basic vertex factory so that it's there for both editor and non-editor builds :
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_Vertex, FXkQuadtreeVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_Compute, FXkQuadtreeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXkQuadtreeVertexFactory, SF_RayHitGroup, FXkQuadtreeVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_TYPE(FXkQuadtreeVertexFactory, "/Plugin/XkGamedevKit/Private/XkVertexFactory.ush",
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
	, MaterialRelevance(InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
	OwnerComponent = const_cast<UXkQuadtreeComponent*>(InComponent);
	MaterialRenderProxy = InMaterialRenderProxy;
	VertexFactory = new FXkQuadtreeVertexFactory(GetScene().GetFeatureLevel());
	Quadtree.Initialize(1024, 16);

	PatchSize = 33;
	BuildPatch(PatchPosition, PatchIndex, PatchSize);

	// Enqueue initialization of render resource
	BeginInitResource(&VertexPositionData_GPU);
	BeginInitResource(&InstancePositionBuffer_GPU);
	BeginInitResource(&InstanceMorphBuffer_GPU);
	BeginInitResource(&IndexBuffer_GPU);

	GenerateBuffers();
}


FXkQuadtreeSceneProxy::~FXkQuadtreeSceneProxy()
{
	check(IsInRenderingThread());

	VertexFactory->ReleaseResource();

	VertexPositionData_GPU.ReleaseResource();
	InstancePositionBuffer_GPU.ReleaseResource();
	InstanceMorphBuffer_GPU.ReleaseResource();
	IndexBuffer_GPU.ReleaseResource();

	OwnerComponent = nullptr;
	VertexFactory = nullptr;
}


SIZE_T FXkQuadtreeSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}


uint32 FXkQuadtreeSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}


void FXkQuadtreeSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const
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
		int16 FrameTag = View.GetOcclusionFrameCounter() % 65535;
		FVector Position = GetActorPosition();

		if (!FreezeQuadtreeCulling)
		{
			Quadtree.Cull(&View.ViewFrustum, View.ViewLocation, Position, FrameTag);
		}
		int32 NunInst = Quadtree.GetVisibleNodes().Num();
		if (NunInst > 0)
		{
			const_cast<FXkQuadtreeSceneProxy*>(this)->UpdateInstanceBuffer(FrameTag);

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
			BatchElement.MaxVertexIndex = VertexPositionData_GPU.VertexBufferRHI->GetSize() - 1;

			TRACE_CPUPROFILER_EVENT_SCOPE(Collector.AddMesh);
			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}


void FXkQuadtreeSceneProxy::CreateRenderThreadResources()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::CreateRenderThreadResources);
	check(IsInRenderingThread());

	VertexFactory->SetSceneProxy(this);
	VertexFactory->InitResource();
}


FPrimitiveViewRelevance FXkQuadtreeSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && (View->Family->EngineShowFlags.BillboardSprites);
	Result.bDynamicRelevance = true;
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
	Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	return Result;
}


void FXkQuadtreeSceneProxy::GenerateBuffers()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::GenerateBuffers);

	check(IsInGameThread());

	if (PatchPosition.Num())
	{
		FPatchData* PatchData = new FPatchData;
		PatchData->Vertices = PatchPosition;
		PatchData->Indices = PatchIndex;

		check(PatchPosition.Num());
		check(PatchIndex.Num());

		FXkQuadtreeSceneProxy* SceneProxy = this;

		ENQUEUE_RENDER_COMMAND(AcComponentSetupOcean)(
			[PatchData, SceneProxy](FRHICommandListImmediate& RHICmdList)
			{
				SceneProxy->GenerateBuffers_Renderthread(RHICmdList, PatchData);
				delete PatchData;
			});
	}
}


void FXkQuadtreeSceneProxy::GenerateBuffers_Renderthread(FRHICommandListImmediate& RHICmdList, FPatchData* PatchData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::GenerateBuffers_Renderthread);

	check(IsInRenderingThread());

	if (!PatchData->Vertices.Num())
		return;

	FRHIResourceCreateInfo CreateInfo(TEXT("QuadtreeSceneProxy"));

	/** vertex buffer */
	int32 NumSourceVerts = PatchData->Vertices.Num();
	VertexPositionData_GPU.VertexBufferRHI = RHICreateVertexBuffer(
		NumSourceVerts * sizeof(FVector4f),
		BUF_Static | BUF_ShaderResource, CreateInfo);
	void* RawVertexBuffer = RHILockBuffer(
		VertexPositionData_GPU.VertexBufferRHI, 0,
		VertexPositionData_GPU.VertexBufferRHI->GetSize(),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawVertexBuffer, &PatchData->Vertices[0], NumSourceVerts * sizeof(FVector4f));
	RHIUnlockBuffer(VertexPositionData_GPU.VertexBufferRHI);

	/** index buffer */
	int32 NumSourceIndices = PatchData->Indices.Num();
	IndexBuffer_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);
	/** index buffer */
	void* RawIndexBuffer = RHILockBuffer(
		IndexBuffer_GPU.IndexBufferRHI,
		0, NumSourceIndices * sizeof(uint32),
		RLM_WriteOnly);
	FMemory::Memcpy((char*)RawIndexBuffer, &PatchData->Indices[0], NumSourceIndices * sizeof(uint32));
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

	/** outer mesh data */
	//DynamicVertexData_OutterOcean_GPU.VertexBufferRHI = RHICreateVertexBuffer(
	//	FARMESH_VERTEX_COUNT * sizeof(FVector4),
	//	BUF_Dynamic | BUF_ShaderResource, CreateInfo);

	//NumSourceIndices = FARMESH_VERTEX_COUNT * 3;

	//IndexBuffer_OutterOcean_GPU.IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), sizeof(uint32) * NumSourceIndices, BUF_Static, CreateInfo);

	///** outer ocean index buffer */
	//RawIndicies = RHILockIndexBuffer(
	//	IndexBuffer_OutterOcean_GPU.IndexBufferRHI,
	//	0, NumSourceIndices * sizeof(uint32),
	//	RLM_WriteOnly);

	//TArray<uint32> TempOuterOceanIndex;
	//TempOuterOceanIndex.Add(0);
	//TempOuterOceanIndex.Add(4);
	//TempOuterOceanIndex.Add(1);

	//TempOuterOceanIndex.Add(1);
	//TempOuterOceanIndex.Add(4);
	//TempOuterOceanIndex.Add(5);

	//TempOuterOceanIndex.Add(1);
	//TempOuterOceanIndex.Add(5);
	//TempOuterOceanIndex.Add(2);

	//TempOuterOceanIndex.Add(5);
	//TempOuterOceanIndex.Add(6);
	//TempOuterOceanIndex.Add(2);

	//TempOuterOceanIndex.Add(6);
	//TempOuterOceanIndex.Add(7);
	//TempOuterOceanIndex.Add(3);

	//TempOuterOceanIndex.Add(6);
	//TempOuterOceanIndex.Add(3);
	//TempOuterOceanIndex.Add(2);

	//TempOuterOceanIndex.Add(0);
	//TempOuterOceanIndex.Add(7);
	//TempOuterOceanIndex.Add(4);

	//TempOuterOceanIndex.Add(0);
	//TempOuterOceanIndex.Add(3);
	//TempOuterOceanIndex.Add(7);

	//FMemory::Memcpy((char*)RawIndicies, (const void*)TempOuterOceanIndex.GetData(), NumSourceIndices * sizeof(uint32));

	//RHIUnlockIndexBuffer(IndexBuffer_OutterOcean_GPU.IndexBufferRHI);
}


void FXkQuadtreeSceneProxy::UpdateInstanceBuffer(const int16 InFrameTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FXkQuadtreeSceneProxy::UpdateInstanceBuffer);

	check(IsInRenderingThread());

	/** instance pos buffer */
	FRHIResourceCreateInfo CreateInfo(TEXT("UpdateInstanceBuffer"));
	int iNunInst = Quadtree.GetVisibleNodes().Num();
	TArray<FVector4f> InstancePositionData;
	TArray<FVector4f> InstanceMorphData;

	FVector4f v4Zero;
	v4Zero.X = 0;
	v4Zero.Y = 0;
	v4Zero.Z = 0;
	v4Zero.W = 0;

	FVector3f RootOffset = FVector3f(Quadtree.GetRootOffset());
	float fMaxX = -99999999;
	float fMinX = 99999999;;
	float fMaxY = -99999999;;
	float fMinY = 99999999;

	for (int i = 0; i < Quadtree.GetVisibleNodes().Num(); i++)
	{
		int32 iTreeIndex = Quadtree.GetVisibleNodes()[i];
		const FQuadtreeNode& QuadtreeNode = Quadtree.GetTreeNodes()[iTreeIndex];
		fMaxX = fMaxX > QuadtreeNode.GetNodeBox().Max.X ? fMaxX : QuadtreeNode.GetNodeBox().Max.X;
		fMaxY = fMaxY > QuadtreeNode.GetNodeBox().Max.Y ? fMaxY : QuadtreeNode.GetNodeBox().Max.Y;
		fMinX = fMinX < QuadtreeNode.GetNodeBox().Min.X ? fMinX : QuadtreeNode.GetNodeBox().Min.X;
		fMinY = fMinY < QuadtreeNode.GetNodeBox().Min.Y ? fMinY : QuadtreeNode.GetNodeBox().Min.Y;
	}

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

	fMaxX += RootOffset.X;
	fMaxY += RootOffset.Y;
	fMinX += RootOffset.X;
	fMinY += RootOffset.Y;

	TArray<FVector4f> Outerocean;
	float fOceanBorder = 80000 * 100;
	FVector4f TempPos;
	TempPos.Z = 0;
	TempPos.W = 1;

	TempPos.X = -fOceanBorder;
	TempPos.Y = -fOceanBorder;
	Outerocean.Add(TempPos);

	TempPos.X = fOceanBorder;
	TempPos.Y = -fOceanBorder;
	Outerocean.Add(TempPos);

	TempPos.X = fOceanBorder;
	TempPos.Y = fOceanBorder;
	Outerocean.Add(TempPos);

	TempPos.X = -fOceanBorder;
	TempPos.Y = fOceanBorder;
	Outerocean.Add(TempPos);

	TempPos.X = fMinX;
	TempPos.Y = fMinY;
	Outerocean.Add(TempPos);

	TempPos.X = fMaxX;
	TempPos.Y = fMinY;
	Outerocean.Add(TempPos);

	TempPos.X = fMaxX;
	TempPos.Y = fMaxY;
	Outerocean.Add(TempPos);

	TempPos.X = fMinX;
	TempPos.Y = fMaxY;
	Outerocean.Add(TempPos);

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

	/** outer ocean mesh */
	//void* outtermeshdata = RHILockBuffer(
	//	DynamicVertexData_OutterOcean_GPU.VertexBufferRHI, 0,
	//	DynamicVertexData_OutterOcean_GPU.VertexBufferRHI->GetSize(),
	//	RLM_WriteOnly);

	//FMemory::Memcpy((char*)outtermeshdata, Outerocean.GetData(), Outerocean.Num() * sizeof(FVector4));

	//RHIUnlockBuffer(DynamicVertexData_OutterOcean_GPU.VertexBufferRHI);
}


FXkLandscapeSceneProxy::FXkLandscapeSceneProxy(const UXkQuadtreeComponent* InComponent, const FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy)
	:FXkQuadtreeSceneProxy(InComponent, ResourceName, InMaterialRenderProxy)
{
}


FXkLandscapeSceneProxy::~FXkLandscapeSceneProxy()
{
}


FXkWaterBodySceneProxy::FXkWaterBodySceneProxy(const UXkQuadtreeComponent* InComponent, FName ResourceName, FMaterialRenderProxy* InMaterialRenderProxy)
	:FXkQuadtreeSceneProxy(InComponent, ResourceName, InMaterialRenderProxy)
{
	TArray<FDynamicMeshVertex> OutVerts;
	VertexBuffers.InitFromDynamicVertex(FarMeshVertexFactory, OutVerts);
	BeginInitResource(&IndexBuffer);

	FarMeshVertexFactory = new FLocalVertexFactory(GetScene().GetFeatureLevel(), "FXkQuadtreeSceneProxy");
}

FXkWaterBodySceneProxy::~FXkWaterBodySceneProxy()
{
	VertexBuffers.PositionVertexBuffer.ReleaseResource();
	VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
	VertexBuffers.ColorVertexBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
}