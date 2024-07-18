// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "XkHexagon/XkHexagonActors.h"
#include "XkHexagon/XkHexagonComponents.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkRenderer/XkRendererRenderUtils.h"
#include "XkGameWorld.generated.h"

UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkSphericalWorldWithOceanActor : public AXkHexagonalWorldActor
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category = "SphericalWorldWithOcean [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkSphericalLandscapeWithWaterComponent> SphericalLandscapeComponent;

	UPROPERTY(VisibleAnywhere, Category = "SphericalWorldWithOcean [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkCanvasRendererComponent> CanvasRendererComponent;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	uint32 GeneratingMaxStep;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	uint32 CenterFieldRange;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	FVector2D PositionRandom;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	float PositionScale;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	float FalloffRadius;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	FVector2D FalloffCenter;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	FVector2D FalloffExtent;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	float FalloffCornerRadii;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	TArray<FXkHexagonSplat> HexagonSplats;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	UMaterialParameterCollection* HexagonMPC;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	bool bSpawnActors;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	int32 SpawnActorsMaxMhtDist;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	int32 XAxisCount;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	int32 YAxisCount;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	bool bShowSpawnedActorBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|MainWorldGenerate")
	bool bShowSpawnedActorEdgeMesh;

	AXkSphericalWorldWithOceanActor();

	//~ Begin Actor Interface
	void PostLoad() override;
	void BeginPlay() override;
	void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	//~ Begin AXkHexagonalWorldActor Interface
	virtual void UpdateHexagonalWorld() override;
	virtual TArray<AXkHexagonActor*> PathfindingToTargetAlways(const AXkHexagonActor* StartActor, const AXkHexagonActor* TargetActor, const TArray<FIntVector>& BlockList);
	//~ End AXkHexagonalWorldActor Interface

	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void GenerateHexagons();

	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void GenerateWorld();

	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void GenerateInstancedHexagons();

	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void GenerateCanvas();

	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void RegenerateAll();
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