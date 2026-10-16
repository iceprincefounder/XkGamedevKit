// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
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

	UPROPERTY(VisibleAnywhere, Transient, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT0_Cached;

	UPROPERTY(VisibleAnywhere, Transient, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	class UTextureRenderTarget2D* CanvasRT1_Cached;

	/* Height convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeX;

	/* Normal convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeY;

	/* SDF convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeZ;

	/* Weight convolution range (aka Filter).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	uint8 ConvolutionRangeW;

	/* XYZ for center of land, W for land height, to calculate ocean SDF.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FVector4f CanvasCenter;

	/*XY for WorldSize X and Y, ZW for Height range Min and Max.*/;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FVector4f CanvasExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	float HorizonHeight;

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
	virtual bool IsRenderDataValid() const;
	virtual void CreateBuffers(
		const TArray<FVector4f> Vertices,
		const TArray<uint32> Indices,
		const TArray<FVector4f> Positions,
		const TArray<FVector4f> Weights);
	virtual void UpdateBuffers(
		const TArray<FVector4f> Positions,
		const TArray<FVector4f> Weights);

	/* Enqueue the full GPU heightmap/weightmap draw pipeline as a multi-frame task sequence.
	 * The CPU-side RebuildCPUHeightField is appended as the final task, so the collision
	 * heightfield is only (re)built AFTER the GPU draw has fully completed. */
	virtual void DrawHeightWeightCanvas_MultiFrame(
		const TArray<FVector4f>& Vertices,
		const TArray<uint32>& Indices,
		const TArray<FVector4f>& InstancePositions);

	//~ Begin CPU HeightField (for runtime collision generation)
	/* Resolution of the CPU heightfield. Independent from GPU RT size. */
	UPROPERTY(EditAnywhere, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	FIntPoint CPUHeightFieldResolution = FIntPoint(1024, 1024);

	/* Default world Z for pixels not covered by any hex instance. */
	UPROPERTY(EditAnywhere, Category = "CanvasRenderer [KEVINTSUIXUGAMEDEV]")
	float CPUHeightFieldDefaultWorldZ = 0.0f;

	/* Rebuild CPU heightfield from hex instance data. Owner's world transform is sampled on game thread. */
	virtual void RebuildCPUHeightField(
		const TArray<FVector4f>& Vertices,
		const TArray<uint32>& Indices,
		const TArray<FVector4f>& InstancePositions);

	/* Bilinear sample world Z at a world XY (XY in actor-local space is also fine when owner has identity rotation). */
	virtual float SampleWorldZCPU(const FVector2D& WorldXY) const;

	/* True once CPU heightfield data is ready. */
	FORCEINLINE bool IsCPUHeightFieldValid() const { return CPUHeightFieldWorldZ.Num() > 0; }

	FORCEINLINE FIntPoint GetCPUHeightFieldSize() const { return CPUHeightFieldSize; }
	FORCEINLINE FVector2D GetCPUHeightFieldWorldMin() const { return CPUHeightFieldWorldMin; }
	FORCEINLINE FVector2D GetCPUHeightFieldWorldSize() const { return CPUHeightFieldWorldSize; }
	FORCEINLINE const TArray<float>& GetCPUHeightFieldWorldZ() const { return CPUHeightFieldWorldZ; }

	/* Broadcast on game thread as the FINAL step of the multi-frame task queue,
	 * right after RebuildCPUHeightField completes. This is a pure data-ready notification
	 * (not a task-queue extension point); subscribers run their own schedulers in parallel. */
	FSimpleMulticastDelegate OnCPUHeightFieldRebuilt;
	//~ End CPU HeightField

private:
	/* CPU heightfield data: world Z per pixel, row-major (y major), after box blur. */
	TArray<float> CPUHeightFieldWorldZ;
	FIntPoint CPUHeightFieldSize = FIntPoint(0, 0);
	FVector2D CPUHeightFieldWorldMin = FVector2D::ZeroVector;
	FVector2D CPUHeightFieldWorldSize = FVector2D::ZeroVector;
	FVector2D CPUHeightFieldPixelSize = FVector2D(1, 1);

private:
	void InitCanvasRTCached_Internal();
	void ReleaseCanvasRTCached_Internal();
	void DrawPrimaryCanvas_Internal(UTextureRenderTarget2D* Output0, UTextureRenderTarget2D* Output1);
	template<typename T>
	void DrawFilteringCanvas_Internal(UTextureRenderTarget2D* Input0, UTextureRenderTarget2D* Input1, UTextureRenderTarget2D* Output);
	void CopyCanvasRT_Internal(UTextureRenderTarget2D* Input, UTextureRenderTarget2D* Output);
	
	/* Vertex buffer for hexagonal world nodes*/
	FXkCanvasVertexBuffer VertexBuffer;
	/* Index buffer for hexagonal world nodes*/
	FXkCanvasIndexBuffer IndexBuffer;
	/* Vertex instance buffer for hexagonal world nodes*/
	FXkCanvasInstanceBuffer InstancePositionBuffer;
	/* Vertex instance buffer for hexagonal world nodes*/
	FXkCanvasInstanceBuffer InstanceWeightBuffer;

	/* Frame-paced GPU task queue. Each Tick dequeues one task (one GPU pipeline step).
	 * SPSC: enqueue on game thread (DrawHeightWeightCanvas_MultiFrame), dequeue on game thread (Tick). */
	TQueue<TFunction<void()>, EQueueMode::Spsc> PendingDrawTasks;

	/* Drain one task per frame from PendingDrawTasks. Decoupled so Tick stays slim. */
	void ProcessPendingDrawTasks();
};