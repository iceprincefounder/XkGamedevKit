// Copyright ©xukai. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkLandscapeComponents.generated.h"


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XuKai QuadtreeComponent"))
class XKGAMEDEVCORE_API UXkQuadtreeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadtree Mesh[KEVINTSUIXU GAMEDEV]")
	UMaterialInterface *Material;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialDyn;

	UXkQuadtreeComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual void PostLoad() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent interface
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XuKai LandscapeComponent"))
class XKGAMEDEVCORE_API UXkLandscapeComponent : public UXkQuadtreeComponent
{
	GENERATED_BODY()
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XuKai WaterBodyComponent"))
class XKGAMEDEVCORE_API UXkWaterBodyComponent : public UXkQuadtreeComponent
{
	GENERATED_BODY()
};