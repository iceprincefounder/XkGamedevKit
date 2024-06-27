// Copyright ©XUKAI. All Rights Reserved.


#include "XkLandscape/XkLandscapeComponents.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
#include "XkLandscape/XkLandscapeSceneProxy.h"


static bool VisQuadtreeBounds = 0;
static FAutoConsoleVariableRef CVarVisQuadtreeBounds(
	TEXT("r.xk.VisQuadtreeBounds"),
	VisQuadtreeBounds,
	TEXT("Vis Quadtree Bounds"));


UXkLandscapeComponent::UXkLandscapeComponent(const FObjectInitializer& ObjectInitializer) :
	UXkQuadtreeComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bTickInEditor = true;

	Material = CastChecked<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/Engine/EngineMaterials/WorldGridMaterial")));
}


void UXkLandscapeComponent::PostLoad()
{
	Super::PostLoad();
}


FPrimitiveSceneProxy* UXkLandscapeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* LocalSceneProxy = NULL;
	if (Material)
	{
		MaterialDyn = UMaterialInstanceDynamic::Create(Material, GetWorld());
	}
	if (MaterialDyn)
	{
		FPrimitiveSceneProxy* Proxy = new FXkLandscapeSceneProxy(
			this, NAME_None, MaterialDyn->GetRenderProxy());
		LocalSceneProxy = Proxy;
	}
	return LocalSceneProxy;
}


FBoxSphereBounds UXkLandscapeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds BoxSphereBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(51200, 51200, 51200), 51200);
	return FBoxSphereBounds(BoxSphereBounds).TransformBy(LocalToWorld);
}


void UXkLandscapeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (VisQuadtreeBounds)
	{
		if (FXkLandscapeSceneProxy* LocalSceneProxy = static_cast<FXkLandscapeSceneProxy*>(SceneProxy))
		{
			const TArray<int32> VisibleNodes = LocalSceneProxy->Quadtree.GetVisibleNodes();
			for (int32 i = 0; i < VisibleNodes.Num(); i++)
			{
				float Thickness = 100.0;
				int32 NodeID = VisibleNodes[i];
				FVector RootOffset = LocalSceneProxy->Quadtree.GetRootOffset();
				int32 MaxLod = LocalSceneProxy->Quadtree.GetCullLodDistance().Num();
				FVector Origin = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetCenter();
				FVector Extent = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeBox().GetExtent();
				int32 Depth = LocalSceneProxy->Quadtree.GetTreeNodes()[NodeID].GetNodeDepth();
				Origin += RootOffset;
				Thickness *= (float(MaxLod - Depth) / (float)MaxLod);
				::DrawDebugBox(GetWorld(), Origin, Extent, Hue2RGB(Depth).ToFColorSRGB(), false, -1.0f, SDPG_Foreground, Thickness);
			}
		}
	}
}


FMaterialRelevance UXkLandscapeComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= Material->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkLandscapeComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (MaterialDyn)
	{
		OutMaterials.Add(MaterialDyn);
	}
}


void UXkLandscapeComponent::FetchPatchData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	if (FXkLandscapeSceneProxy* LocalSceneProxy = static_cast<FXkLandscapeSceneProxy*>(SceneProxy))
	{
		OutVertices = LocalSceneProxy->PatchData.Vertices;
		OutIndices = LocalSceneProxy->PatchData.Indices;
	}
}


FMaterialRelevance UXkLandscapeWithWaterComponent::GetWaterMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= WaterMaterial->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkLandscapeWithWaterComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (MaterialDyn)
	{
		OutMaterials.Add(MaterialDyn);
	}
	if (WaterMaterialDyn)
	{
		OutMaterials.Add(WaterMaterialDyn);
	}
}


FPrimitiveSceneProxy* UXkSphericalLandscapeWithWaterComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* LocalSceneProxy = NULL;
	if (Material && WaterMaterial)
	{
		MaterialDyn = UMaterialInstanceDynamic::Create(Material, GetWorld());
		WaterMaterialDyn = UMaterialInstanceDynamic::Create(WaterMaterial, GetWorld());
	}
	if (MaterialDyn && WaterMaterialDyn)
	{
		FPrimitiveSceneProxy* Proxy = new FXkSphericalLandscapeWithWaterSceneProxy(
			this, NAME_None, MaterialDyn->GetRenderProxy(), WaterMaterialDyn->GetRenderProxy());
		LocalSceneProxy = Proxy;
	}
	return LocalSceneProxy;
}