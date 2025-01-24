// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkLandscapeComponents.generated.h"


UCLASS(BlueprintType, Blueprintable, ShowCategories = (VirtualTexture), ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkQuadtreeComponent"))
class XKGAMEDEVCORE_API UXkQuadtreeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkLandscapeComponent"))
class XKGAMEDEVCORE_API UXkLandscapeComponent : public UXkQuadtreeComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface *Material;

	UXkLandscapeComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual void PostLoad() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent interface

	virtual void FetchPatchData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkLandscapeWithWaterComponent"))
class XKGAMEDEVCORE_API UXkLandscapeWithWaterComponent : public UXkLandscapeComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]")
	class UWaterWavesAsset* WaterWavesAsset;

	/** Water depth at which waves start being attenuated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]", meta = (UIMin = 0, ClampMin = 0, UIMax = 10000.0))
	float TargetWaveMaskDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandscapeWithWater [KEVINTSUIXUGAMEDEV]")
	bool bDisableWaterBody;

	FMaterialRelevance GetWaterMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;

	//~ Begin UPrimitiveComponent interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent interface
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkSphericalLandscapeComponent"))
class XKGAMEDEVCORE_API UXkSphericalLandscapeWithWaterComponent : public UXkLandscapeWithWaterComponent
{
	GENERATED_BODY()
public:
	//~ Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent interface
};