// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "XkHexagonPathfinding.h"
#include "XkHexagonComponents.generated.h"

UCLASS(ClassGroup = Utility, hidecategories = (Object, LOD, Physics, Lighting, TextureStreaming, Activation, "Components|Activation", Collision), editinlinenew, meta = (BlueprintSpawnableComponent))
class XKGAMEDEVCORE_API UXkHexagonArrowComponent : public UArrowComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Color to draw arrow step*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	float ArrowZOffset;

	/** Color to draw arrow step*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	float ArrowMarkStep;

	/** Color to draw arrow step*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	float ArrowMarkWidth;

	/** Color to draw x arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	FColor ArrowXColor;

	/** Color to draw y arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	FColor ArrowYColor;

	/** Color to draw z arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXU GAMEDEV]")
	FColor ArrowZColor;

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkHexagonalWorldComponent"))
class XKGAMEDEVCORE_API UXkHexagonalWorldComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 MaxManhattanDistance;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]")
	UPROPERTY()
	TMap<FIntVector, FXkHexagonNode> HexagonalWorldNodes;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowEdgeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]")
	UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonalPrimitive [KEVINTSUIXU GAMEDEV]")
	UMaterialInterface* EdgeMaterial;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BaseMaterialDyn;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* EdgeMaterialDyn;

	UXkHexagonalWorldComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual void PostLoad() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent interface

	virtual void FetchHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);
	virtual FVector2D GetHexagonalWorldExtent() const;
	virtual FVector2D GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const;
};