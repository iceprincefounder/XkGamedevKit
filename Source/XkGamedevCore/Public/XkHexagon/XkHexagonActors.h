// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonComponents.h"
#include "XkHexagonPathfinding.h"
#include "XkHexagonActors.generated.h"

// when EXkHexagonType is greater that AVAILABLEMARK,
// which mean that actor is unreachable!
#define BASE_SECTION_INDEX 0
#define EDGE_SECTION_INDEX 1

class UProceduralMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonActor : public AActor
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UProceduralMeshComponent* ProcMesh;
#endif

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UStaticMeshComponent* StaticProcMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* EdgeMaterial;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* BaseMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* EdgeMID;
public:
	AXkHexagonActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin AActor Interface
	virtual void ConstructionScripts();
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	void PostEditMove(bool bFinished) override;
#endif
	//~ End AActor Interface

	//~ Begin AXkHexagonActor Interface
	virtual FIntVector GetCoord() const { return Coord; };
	virtual void SetCoord(const FIntVector& Input) { Coord = Input; };
	virtual TWeakObjectPtr<class AXkHexagonalWorldActor> GetHexagonalWorld() const { return ParentHexagonalWorld; };
	virtual void SetHexagonWorld(class AXkHexagonalWorldActor* Input);
	virtual void OnBaseHighlight(const FLinearColor& InColor = FLinearColor::White);
	virtual void OnEdgeHighlight(const FLinearColor& InColor = FLinearColor::White);
	virtual UStaticMeshComponent* GetStaticProcMesh() const { return StaticProcMesh; };
	//~ End AXkHexagonActor Interface

protected:

	virtual void UpdateMaterial();
#if WITH_EDITOR
	virtual void UpdateProcMesh();
#endif
	virtual void InitHexagon(const FIntVector& InCoord);
	virtual void FreeHexagon();

	UPROPERTY()
	bool bCachedBaseHighlight;
	UPROPERTY()
	bool bCachedEdgeHighlight;
	UPROPERTY()
	TWeakObjectPtr<class AXkHexagonalWorldActor> ParentHexagonalWorld;
};


UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonalWorldActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class UXkHexagonArrowComponent> SceneRoot;

	UPROPERTY(VisibleDefaultsOnly, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	//UPROPERTY()
	TObjectPtr<class UXkInstancedHexagonComponent> InstancedHexagonComponent;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	int32 PathfindingMaxStep;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	int32 BacktrackingMaxStep;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonStarter;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonTargeter;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	TArray<TObjectPtr<class AXkHexagonActor>> HexagonBlockers;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	int32 MaxManhattanDistance;

	friend class AXkHexagonActor;

	AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UFUNCTION(CallInEditor, Category = "HexagonalWorld [KEVINTSUIXUGAMEDEV]")
	void DebugPathfinding();

	//~ Begin Actor Interface
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

public:
	/**
	* @brief Find a hexagon node by input coordinate
	* @param InCoord The coordinate a hexagon pretend to be
	*/
	FORCEINLINE virtual FXkHexagonNode* GetHexagonNode(const FIntVector& InCoord) const;

	/**
	* @brief Find a hexagon node by input position
	* @param InPosition The position a hexagon node pretend to be
	*/
	FORCEINLINE virtual FXkHexagonNode* GetHexagonNode(const FVector& InPosition) const;

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonNodeNeighbors(const FIntVector& InCoord) const;

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonNodeSurrounders(const TArray<FIntVector>& InCoords) const;

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonNodeCoverages(const FIntVector& InCoord, const int32 InRange) const;

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonNodesPath(const FIntVector& StartCoord, const FIntVector& EndCoord);

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonNodesPathfinding(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList = TArray<FIntVector>());

	FORCEINLINE virtual TArray<FXkHexagonNode*> GetHexagonalWorldNodes(const EXkHexagonType HexagonType) const;

	FORCEINLINE virtual int32 GetHexagonManhattanDistance(const FVector& A, const FVector& B) const;

	FORCEINLINE virtual FVector2D GetHexagonalWorldExtent() const;

	FORCEINLINE virtual FVector2D GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const;

	FORCEINLINE virtual void BuildHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);

	FORCEINLINE virtual TMap<FIntVector, FXkHexagonNode>& ModifyHexagonalWorldNodes() const { return HexagonalWorldTable.Nodes; };

	FORCEINLINE virtual void CreateAll() {};
	FORCEINLINE virtual void UpdateAll() {};

private:
	UPROPERTY(Transient)
	mutable FXkHexagonalWorldNodeTable HexagonalWorldTable;

	UPROPERTY(Transient)
	mutable FXkHexagonAStarPathfinding HexagonAStarPathfinding;
};