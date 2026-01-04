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
#include "MaterialDomain.h"
#include "DataDrivenShaderPlatformInfo.h"


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

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
void FXkHexagonalWorldVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
#else
void FXkHexagonalWorldVertexFactory::InitRHI()
#endif
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