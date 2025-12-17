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
	UPROPERTY(VisibleDefaultsOnly, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkSkydomeComponent> SkydomeComponent;

	UPROPERTY(VisibleAnywhere, Category = "SphericalWorldWithOcean [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkSphericalLandscapeWithWaterComponent> SphericalLandscapeComponent;

	UPROPERTY(VisibleAnywhere, Category = "SphericalWorldWithOcean [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkCanvasRendererComponent> CanvasRendererComponent;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	int32 GroundManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	int32 ShorelineManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	float ShorelineEachStepHeight;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	FVector2D PositionRandomRange;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	UMaterialParameterCollection* HexagonMPC;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* SkydomeMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	float SkydomeSinkDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	bool bSpawnActors;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	int32 SpawnActorsMaxMhtDist;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	bool bShowSpawnedActorBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	bool bShowSpawnedActorEdgeMesh;

	AXkSphericalWorldWithOceanActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin Actor Interface
	void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	UFUNCTION(BlueprintCallable, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateHexagons();
	UFUNCTION(BlueprintCallable, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateHexagonalWorld();
	UFUNCTION(BlueprintCallable, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	virtual void GenerateCanvas();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "HexagonalWorldInGame [KEVINTSUIXUGAMEDEV]")
	void RegenerateWorld();

public:
	static FVector CalcSphericalLocation(const FVector& CameraLocation, const FVector& WorldLocation)
	{
		float HeightZ = SphericalHeight(CameraLocation, WorldLocation);
		// Compensate for floating-point precision loss at large distances to reduce terrain height errors
		float Dist = FVector::Dist2D(CameraLocation, WorldLocation);
		float Compensate = FMath::Max(Dist - 6400.0f, 0.0) / 6400.0f * 250.0;
		return FVector(WorldLocation.X, WorldLocation.Y, HeightZ - Compensate);
	}
};