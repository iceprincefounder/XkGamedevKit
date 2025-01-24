// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "XkHexagon/XkHexagonActors.h"
#include "XkHexagon/XkHexagonComponents.h"
#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkLandscape/XkLandscapeRenderUtils.h"
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

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	int32 GroundManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	int32 ShorelineManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	FVector2D PositionRandomRange;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	TArray<FXkHexagonSplat> HexagonSplats;

	/* The splat id in the range of HexagonSplatMaskRange would do discard in VS. */
	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	FVector2D HexagonSplatMaskRange;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	UMaterialParameterCollection* HexagonMPC;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	bool bSpawnActors;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	int32 SpawnActorsMaxMhtDist;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	bool bShowSpawnedActorBaseMesh;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	bool bShowSpawnedActorEdgeMesh;

	AXkSphericalWorldWithOceanActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin Actor Interface
	void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	UFUNCTION(BlueprintCallable, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateHexagons();

	UFUNCTION(BlueprintCallable, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateHexagonalWorld();

	UFUNCTION(BlueprintCallable, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateGameWorld();

	UFUNCTION(BlueprintCallable, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateCanvas();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "MainWorldGenerate [KEVINTSUIXUGAMEDEV]")
	void RegenerateWorld();

public:
	static float CalcSphericalHeight(const FVector& CameraLocation, const FVector& WorldLocation)
	{
		return SphericalHeight(CameraLocation, WorldLocation);
	}
};