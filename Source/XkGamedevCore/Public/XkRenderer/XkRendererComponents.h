// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkRendererRenderUtils.h"
#include "XkRendererComponents.generated.h"


/** A TopDown Canvas Renderer.*/
UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkQuadtreeComponent"))
class XKGAMEDEVCORE_API UXkCanvasRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* The canvas render target 0 for landscape normal and height.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT0;

	/* The canvas render target 2 for landscape hexagon base&edge color and splat map.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT1;

	/* Height convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	uint8 ConvolutionRangeX;

	/* Normal convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	uint8 ConvolutionRangeY;

	/* SDF convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	uint8 ConvolutionRangeZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	FVector4f CanvasCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	FVector4f CanvasExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	float HorizonHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXU GAMEDEV]")
	UMaterialParameterCollection* CanvasMPC;

	UXkCanvasRendererComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

	virtual FVector2D GetCanvasSize() const;
	virtual void CreateBuffers(
		FXkCanvasVertexBuffer* VertexBuffer, 
		FXkCanvasIndexBuffer* IndexBuffer, 
		FXkCanvasInstanceBuffer* InstancePositionBuffer, 
		FXkCanvasInstanceBuffer* InstanceWeightBuffer);
	virtual void DrawCanvas(
		FXkCanvasVertexBuffer* VertexBuffer, 
		FXkCanvasIndexBuffer* IndexBuffer, 
		FXkCanvasInstanceBuffer* InstancePositionBuffer, 
		FXkCanvasInstanceBuffer* InstanceWeightBuffer);
	virtual FVector4f GetCanvasCenter() const { return CanvasCenter; }
	virtual void SetCanvasCenter(const FVector4f& Input) { CanvasCenter = Input; }
	virtual FVector4f GetCanvasExtent() const { return CanvasExtent; }
	virtual void SetCanvasExtent(const FVector4f& Input) { CanvasExtent = Input; }
};