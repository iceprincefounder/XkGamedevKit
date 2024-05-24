// Copyright ©XUKAI. All Rights Reserved.


#include "XkLandscape/XkLandscapeComponents.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
#include "XkLandscape/XkLandscapeSceneProxy.h"


static bool VisQuadtreeBounds = 0;
static FAutoConsoleVariableRef CVarVisQuadtreeBounds(
	TEXT("r.xk.VisQuadtreeBounds"),
	VisQuadtreeBounds,
	TEXT("Vis Quadtree Bounds"));

UXkQuadtreeComponent::UXkQuadtreeComponent(const FObjectInitializer& ObjectInitializer) :
	UPrimitiveComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bTickInEditor = true;

	Material = CastChecked<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/Engine/EngineMaterials/WorldGridMaterial")));
}


void UXkQuadtreeComponent::PostLoad()
{
	Super::PostLoad();
}


FPrimitiveSceneProxy* UXkQuadtreeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* QuadtreeSceneProxy = NULL;
	if (Material)
	{
		MaterialDyn = UMaterialInstanceDynamic::Create(Material, GetWorld());
	}
	if (MaterialDyn)
	{
		FPrimitiveSceneProxy* Proxy = new FXkQuadtreeSceneProxy(
			this, NAME_None, MaterialDyn->GetRenderProxy());
		QuadtreeSceneProxy = Proxy;
	}
	return QuadtreeSceneProxy;
}


FBoxSphereBounds UXkQuadtreeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds BoxSphereBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(51200, 51200, 51200), 51200);
	return FBoxSphereBounds(BoxSphereBounds).TransformBy(LocalToWorld);
}


void UXkQuadtreeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (VisQuadtreeBounds)
	{
		if (FXkQuadtreeSceneProxy* QuadtreeSceneProxy = static_cast<FXkQuadtreeSceneProxy*>(SceneProxy))
		{
			const TArray<int32> VisibleNodes = QuadtreeSceneProxy->Quadtree.GetVisibleNodes();
			for (int32 i = 0; i < VisibleNodes.Num(); i++)
			{
				float Thickness = 100.0;
				int32 NodeID = VisibleNodes[i];
				FVector RootOffset = QuadtreeSceneProxy->Quadtree.GetRootOffset();
				int32 MaxLod = QuadtreeSceneProxy->Quadtree.GetCullLodDistance().Num();
				FVector Origin = QuadtreeSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetCenter();
				FVector Extent = QuadtreeSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetExtent();
				int32 Depth = QuadtreeSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeDepth();
				Origin += RootOffset;
				Thickness *= (float(MaxLod - Depth) / (float)MaxLod);
				::DrawDebugBox(GetWorld(), Origin, Extent, Hue2RGB(Depth).ToFColorSRGB(), false, -1.0f, SDPG_Foreground, Thickness);
			}
		}
	}
}


FMaterialRelevance UXkQuadtreeComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= Material->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkQuadtreeComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (MaterialDyn)
	{
		OutMaterials.Add(MaterialDyn);
	}
}


void UXkQuadtreeComponent::FetchPatchData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	if (FXkQuadtreeSceneProxy* QuadtreeSceneProxy = static_cast<FXkQuadtreeSceneProxy*>(SceneProxy))
	{
		OutVertices = QuadtreeSceneProxy->PatchData.Vertices;
		OutIndices = QuadtreeSceneProxy->PatchData.Indices;
	}
}


FPrimitiveSceneProxy* UXkSphericalLandscapeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* SphericalLandscapeSceneProxy = NULL;
	if (Material)
	{
		MaterialDyn = UMaterialInstanceDynamic::Create(Material, GetWorld());
	}
	if (MaterialDyn)
	{
		FPrimitiveSceneProxy* Proxy = new FXkSphericalLandscapeSceneProxy(
			this, NAME_None, MaterialDyn->GetRenderProxy());
		SphericalLandscapeSceneProxy = Proxy;
	}
	return SphericalLandscapeSceneProxy;
}


FPrimitiveSceneProxy* UXkSphericalLandscapeWithWaterComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* LocalSceneProxy = NULL;
	if (Material && MaterialWater)
	{
		MaterialDyn = UMaterialInstanceDynamic::Create(Material, GetWorld());
		MaterialWaterDyn = UMaterialInstanceDynamic::Create(MaterialWater, GetWorld());
	}
	if (MaterialDyn && MaterialWaterDyn)
	{
		FPrimitiveSceneProxy* Proxy = new FXkSphericalLandscapeWithWaterSceneProxy(
			this, NAME_None, MaterialDyn->GetRenderProxy(), MaterialWaterDyn->GetRenderProxy());
		LocalSceneProxy = Proxy;
	}
	return LocalSceneProxy;
}


FMaterialRelevance UXkSphericalLandscapeWithWaterComponent::GetMaterialWaterRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= MaterialWater->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkSphericalLandscapeWithWaterComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (MaterialDyn)
	{
		OutMaterials.Add(MaterialDyn);
	}
	if (MaterialWaterDyn)
	{
		OutMaterials.Add(MaterialWaterDyn);
	}
}
