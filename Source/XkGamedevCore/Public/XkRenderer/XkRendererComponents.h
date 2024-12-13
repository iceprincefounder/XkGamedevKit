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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT0;

	/* The canvas render target 2 for landscape hexagon base&edge color and splat map.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT1;

	/* Height convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeX;

	/* Normal convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeY;

	/* SDF convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeZ;

	/* XYZ for center of land, W for land height, to calculate ocean SDF.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FVector4f CanvasCenter;

	/*XY for WorldSize X and Y, ZW for Height range Min and Max.*/;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FVector4f CanvasExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	float HorizonHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FVector2D SplatMaskRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	UMaterialParameterCollection* CanvasMPC;

	UXkCanvasRendererComponent(const FObjectInitializer& ObjectInitializer);
	~UXkCanvasRendererComponent();
	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

	virtual FVector2D GetCanvasSize() const;
	virtual FVector4f GetCanvasCenter() const { return CanvasCenter; }
	virtual void SetCanvasCenter(const FVector4f& Input) { CanvasCenter = Input; }
	virtual FVector4f GetCanvasExtent() const { return CanvasExtent; }
	virtual void SetCanvasExtent(const FVector4f& Input) { CanvasExtent = Input; }

	virtual bool IsBuffersValid() const;
	virtual void CreateBuffers(
		const TArray<FVector4f> Vertices,
		const TArray<uint32> Indices,
		const TArray<FVector4f> Positions,
		const TArray<FVector4f> Weights);
	virtual void UpdateBuffers(
		const TArray<FVector4f> Positions,
		const TArray<FVector4f> Weights);
	virtual void DrawCanvas();

private:
	/* Vertex buffer for hexagonal world nodes*/
	FXkCanvasVertexBuffer VertexBuffer;
	/* Index buffer for hexagonal world nodes*/
	FXkCanvasIndexBuffer IndexBuffer;
	/* Vertex instance buffer for hexagonal world nodes*/
	FXkCanvasInstanceBuffer InstancePositionBuffer;
	/* Vertex instance buffer for hexagonal world nodes*/
	FXkCanvasInstanceBuffer InstanceWeightBuffer;
};