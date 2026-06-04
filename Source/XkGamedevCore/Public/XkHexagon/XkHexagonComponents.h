// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "XkHexagonPathfinding.h"
#include "XkGeometry/XkGeometry.h"
#include "XkHexagonComponents.generated.h"


UCLASS(ClassGroup = Utility, hidecategories = (Object, LOD, Physics, Lighting, TextureStreaming, Activation, "Components|Activation", Collision), editinlinenew, meta = (BlueprintSpawnableComponent))
class XKGAMEDEVCORE_API UXkHexagonArrowComponent : public UArrowComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Arrow height*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	float ArrowHeight;

	/** Extra arrow height offset*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	float ArrowZOffset;

	/** Color to draw arrow step*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	float ArrowUnitStep;

	/** Color to draw arrow step*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	float ArrowMarkWidth;

	/** Color to draw x arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	FColor ArrowXColor;

	/** Color to draw y arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	FColor ArrowYColor;

	/** Color to draw z arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonArrow [KEVINTSUIXUGAMEDEV]")
	FColor ArrowZColor;

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, ShowCategories = (VirtualTexture), meta = (BlueprintSpawnableComponent, DisplayName = "XkInstancedHexagonComponent"))
class XKGAMEDEVCORE_API UXkInstancedHexagonComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UXkInstancedHexagonComponent(const FObjectInitializer& ObjectInitializer);
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, ShowCategories = (VirtualTexture), meta = (BlueprintSpawnableComponent, DisplayName = "XkInstancedHexagonComponent"))
class XKGAMEDEVCORE_API UXkSkydomeComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UXkSkydomeComponent(const FObjectInitializer& ObjectInitializer);
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkHexagonBasedFortressComponent"))
class XKGAMEDEVCORE_API UXkHexagonBasedFortressComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UXkHexagonBasedFortressComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* PalisadeWallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* PalisadeWallTopMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* TrapezoidWallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* TrapezoidWallTopMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* TrapezoidTowerTopMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* TrapezoidGateTopMaterial;

	UPROPERTY(Transient)
	TArray<FVector> FortressBaseAnchors;

	UPROPERTY(Transient)
	TArray<FVector> NeighborHexagonCenters;

	//~ Begin UXkHexagonBasedFortressComponent interface
	virtual void UpdateHexagonBasedPalisadeWall();
	virtual void UpdateHexagonBasedTrapezoidBase();
	virtual void UpdateHexagonBasedTrapezoidWall();
	virtual void UpdateHexagonBasedTrapezoidTower();
	virtual void UpdateHexagonBasedTrapezoidGate();
	virtual void UpdateHexagonBasedFortressPhysics();
	//~ End UXkHexagonBasedFortressComponent interface

private:
	TArray<FXkGeomEdge> GetPalisadeBaseCenterLines() const;
	TArray<FXkGeomEdge> GetTrapezoidBaseBoundaryEdges() const;
	void UpdateDynamicMeshInternal(const FDynamicMesh3& InDynamicMesh, const bool bForceUpdate = false);
	FVector CalcDynamicMeshCenterPivotInternal(const FDynamicMesh3& InDynamicMesh, const FTransformSRT3d& InTransform = FTransformSRT3d::Identity()) const;
	FDynamicMesh3 CalcWavePatternByBoundaryEdgesInternal(const TArray<FXkGeomEdge>& InBoundaryEdges);
	FDynamicMesh3 CalcBooleanOperationInternal(const FMeshBoolean::EBooleanOp Operation, 
		const FDynamicMesh3& MeshA, const FDynamicMesh3& MeshB, 
		const FTransformSRT3d& TransformA = FTransformSRT3d::Identity(),
		const FTransformSRT3d& TransformB = FTransformSRT3d::Identity());
private:
	TArray<TPair<FVector, FVector>> PalisadeBaseLines;
	TArray<TPair<FVector, FVector>> TrapezoidBaseEdges;
	TArray<TArray<FVector>> TrapezoidBaseContours;
	TArray<FVector> TrapezoidBaseIntersectionPoints;
};