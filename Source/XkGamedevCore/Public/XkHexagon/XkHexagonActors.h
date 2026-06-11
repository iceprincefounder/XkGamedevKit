// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonComponents.h"
#include "XkHexagonPathfinding.h"
#include "XkHexagonActors.generated.h"

#define BASE_SECTION_INDEX 0
#define EDGE_SECTION_INDEX 0
#define PIVOT_SECTION_INDEX 0
#define SIDE_SECTION_INDEX 0
#define HEXAGON_RADIUS 100.0f
#define HEXAGON_HEIGHT 10.0f
#define HORIZION_HEIGHT 100.0f
#define HEXAGON_GAP_WIDTH 0.0f
#define HEXAGON_BASE_INNER_GAP 0.0f
#define HEXAGON_BASE_OUTER_GAP 0.0f
#define HEXAGON_EDGE_INNER_GAP 9.0f
#define HEXAGON_EDGE_OUTER_GAP 1.0f

class UProceduralMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonActor : public AActor
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UProceduralMeshComponent* ProcMeshBase;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UProceduralMeshComponent* ProcMeshEdge;
#endif

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UStaticMeshComponent* StaticMeshBase;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UStaticMeshComponent* StaticMeshEdge;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UStaticMeshComponent* StaticMeshPivot;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UStaticMeshComponent* StaticMeshSide;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* EdgeMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* PivotMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInterface* SideMaterial;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* BaseMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* EdgeMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* PivotMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* SideMID;

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
	virtual void OnPivotHighlight(const FLinearColor& InColor = FLinearColor::White);
	virtual void OnSideHighlight(const FLinearColor& InColor = FLinearColor::White);
	virtual UStaticMeshComponent* GetStaticMeshBase() const { return StaticMeshBase; };
	virtual UStaticMeshComponent* GetStaticMeshEdge() const { return StaticMeshEdge; };
	virtual UStaticMeshComponent* GetStaticMeshPivot() const { return StaticMeshPivot; };
	virtual UStaticMeshComponent* GetStaticMeshSide() const { return StaticMeshSide; };
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

private:
	UPROPERTY(Transient)
	mutable FXkHexagonalWorldNodeTable HexagonalWorldTable;

	UPROPERTY(Transient)
	mutable FXkHexagonAStarPathfinding HexagonAStarPathfinding;
};