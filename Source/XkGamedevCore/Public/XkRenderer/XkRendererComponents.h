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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canvas [KEVINTSUIXU GAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canvas [KEVINTSUIXU GAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT1;

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
		FXkCanvasInstanceBuffer* InstanceWeightBuffer,
		const FVector4f& Center, const FVector4f& Extent);
};