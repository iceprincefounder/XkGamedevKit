// Copyright ©xukai. All Rights Reserved.


#include "XkLandscape/XkLandscapeComponents.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
#include "XkLandscape/XkLandscapeSceneProxy.h"


static bool VisQuadtreeBounds = 1;
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
	FBoxSphereBounds BoxSphereBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX), HALF_WORLD_MAX);
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