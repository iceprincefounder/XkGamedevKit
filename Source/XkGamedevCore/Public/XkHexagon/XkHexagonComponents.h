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


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, ShowCategories = (VirtualTexture), meta = (BlueprintSpawnableComponent, DisplayName = "XkHexagonalWorldComponent"))
class XKGAMEDEVCORE_API UXkHexagonalWorldComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 MaxManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowEdgeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonalPrimitive [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* EdgeMaterial;

	UXkHexagonalWorldComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual void PostLoad() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent interface

	virtual void InitHexagonalWorldTable(FXkHexagonalWorldNodeTable* Input) { HexagonalWorldTable = Input; };
	const TMap<FIntVector, FXkHexagonNode>& ModifyHexagonalWorldNodes() const { check(HexagonalWorldTable); return HexagonalWorldTable->Nodes; };
	virtual void BuildHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);
	virtual FVector2D GetHexagonalWorldExtent() const;
	virtual FVector2D GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const;

private:
	FXkHexagonalWorldNodeTable* HexagonalWorldTable;
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, ShowCategories = (VirtualTexture), meta = (BlueprintSpawnableComponent, DisplayName = "XkInstancedHexagonComponent"))
class XKGAMEDEVCORE_API UXkInstancedHexagonComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UXkInstancedHexagonComponent(const FObjectInitializer& ObjectInitializer);
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = XkGamedevCore, meta = (BlueprintSpawnableComponent, DisplayName = "XkHexagonBasedFortressComponent"))
class XKGAMEDEVCORE_API UXkHexagonBasedFortressComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UXkHexagonBasedFortressComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexagonBasedFortress [KEVINTSUIXUGAMEDEV]")
	UMaterialInterface* TrapezoidWallMaterial;

	UPROPERTY(Transient)
	TArray<FVector> TrapezoidBaseAnchors;

	UPROPERTY(Transient)
	TArray<FVector> NeighborHexagonCenters;

	//~ Begin UXkHexagonBasedFortressComponent interface
	virtual void UpdateHexagonBasedFortressBase();
	virtual void UpdateHexagonBasedFortressWall();
	virtual void UpdateHexagonBasedFortressTower();
	virtual void UpdateHexagonBasedFortressGate();
	virtual void UpdateHexagonBasedFortressPhysics();
	//~ End UXkHexagonBasedFortressComponent interface

private:
	TArray<FXkGeomEdge> GetTrapezoidBaseBoundaryEdges() const;
	void UpdateDynamicMeshInternal(const FDynamicMesh3& InDynamicMesh, const bool bForceUpdate = false);
	FVector CalcDynamicMeshCenterPivotInternal(const FDynamicMesh3& InDynamicMesh, const FTransformSRT3d& InTransform = FTransformSRT3d::Identity()) const;
	FDynamicMesh3 CalcWavePatternByBoundaryEdgesInternal(const TArray<FXkGeomEdge>& InBoundaryEdges);
	FDynamicMesh3 CalcBooleanOperationInternal(const FMeshBoolean::EBooleanOp Operation, 
		const FDynamicMesh3& MeshA, const FDynamicMesh3& MeshB, 
		const FTransformSRT3d& TransformA = FTransformSRT3d::Identity(),
		const FTransformSRT3d& TransformB = FTransformSRT3d::Identity());
private:
	TArray<TPair<FVector, FVector>> TrapezoidBaseEdges;
	TArray<TArray<FVector>> TrapezoidBaseContours;
	TArray<FVector> TrapezoidBaseIntersectionPoints;
};