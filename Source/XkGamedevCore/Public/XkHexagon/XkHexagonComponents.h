// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "XkHexagonEditor.generated.h"


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