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
	class UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	EXkHexagonType HexagonType;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* BaseMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	class UMaterialInstanceDynamic* EdgeMID;
public:
	AXkHexagonActor();

	UFUNCTION(CallInEditor, Category = "HexagonActor [KEVINTSUIXUGAMEDEV]")
	void SnapToLandscape();

	virtual void ConstructionScripts();
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	void PostEditMove(bool bFinished) override;
#endif

public:
	//~ Begin AXkHexagonActor Interface
	virtual FIntVector GetCoord() const { return Coord; };
	virtual void SetCoord(const FIntVector& Input) { Coord = Input; };
	virtual EXkHexagonType GetHexagonType() const { return HexagonType; };
	virtual void SetHexagonType(EXkHexagonType Input) { HexagonType = Input; };
	virtual FXkHexagonNode* GetHexagonNode() { return HexagonNode; };
	virtual void SetHexagonNode(FXkHexagonNode* Input) { HexagonNode = Input;};
	virtual TWeakObjectPtr<class AXkHexagonalWorldActor> GetHexagonalWorld() const { return CachedHexagonalWorld; };
	virtual void SetHexagonWorld(class AXkHexagonalWorldActor* Input);
	//~ End AXkHexagonActor Interface

	/** OnBaseSelecting color is temporary and override the OnBaseHighlight color*/
	virtual void OnBaseSelecting(bool bSelecting, const FLinearColor& SelectingColor = FLinearColor::White);
	/** the basic base color*/
	virtual void OnBaseHighlight(bool bHighlight, const FLinearColor& HighlightColor = FLinearColor::White);
	/** OnEdgeSelecting color is temporary and override the OnEdgeHighlight color*/
	virtual void OnEdgeSelecting(bool bSelecting, const FLinearColor& SelectingColor = FLinearColor::White);
	/** the basic edge color*/
	virtual void OnEdgeHighlight(bool bHighlight, const FLinearColor& HighlightColor = FLinearColor::White);

public:
	void UpdateMaterial();
#if WITH_EDITOR
	void UpdateProcMesh();
#endif

protected:
	UPROPERTY()
	bool bCachedBaseHighlight;
	UPROPERTY()
	FLinearColor CachedBaseHighlightColor;
	UPROPERTY()
	bool bCachedEdgeHighlight;
	UPROPERTY()
	FLinearColor CachedEdgeHighlightColor;
	UPROPERTY()
	TWeakObjectPtr<class AXkHexagonalWorldActor> CachedHexagonalWorld;

private:
	FXkHexagonNode* HexagonNode;
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

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	int32 PathfindingMaxStep;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	int32 BacktrackingMaxStep;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonStarter;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonTargeter;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	TArray<TObjectPtr<class AXkHexagonActor>> HexagonBlockers;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	int32 MaxManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	UMaterialInterface* HexagonBaseMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]|HexagonalPrimitive")
	UMaterialInterface* HexagonEdgeMaterial;

	friend class AXkHexagonActor;

	AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
public:
	UFUNCTION(CallInEditor, Category = "HexagonalWorld[KEVINTSUIXUGAMEDEV]")
	void Pathfinding();

	//~ Begin Actor Interface
	void BeginPlay() override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	virtual void UpdateHexagonalWorld();
public:
	FORCEINLINE virtual bool IsHexagonActorsNeighboring(const AXkHexagonActor* A, const AXkHexagonActor* B) const;

	/**
	* @brief Find a hexagon node by input coordinate
	* @param InCoord The coordinate a hexagon pretend to be
	*/
	FORCEINLINE virtual FXkHexagonNode* GetHexagonNodeByCoord(const FIntVector& InCoord);

	/**
	* @brief Find a hexagon node by input position
	* @param InPosition The position a hexagon node pretend to be
	*/
	FORCEINLINE virtual FXkHexagonNode* GetHexagonNodeByLocation(const FVector& InPosition);

	FORCEINLINE virtual int32 GetHexagonManhattanDistance(const FVector& A, const FVector& B) const;
	FORCEINLINE virtual int32 GetHexagonManhattanDistance(const AXkHexagonActor* A, const AXkHexagonActor* B) const;

	/**
	* @brief Find a hexagon by input coordinate
	* @param InCoord The coordinate a hexagon pretend to be
	*/
	FORCEINLINE virtual AXkHexagonActor* GetHexagonActorByCoord(const FIntVector& InCoord) const;

	/**
	* @brief Find a hexagon by input position
	* @param InPosition The position a hexagon pretend to be
	*/
	FORCEINLINE virtual AXkHexagonActor* GetHexagonActorByLocation(const FVector& InPosition) const;

	/**
	* @brief Find all valid neighbors of input hexagon
	* @param InputActor The actor's neighbors would be search
	*/
	FORCEINLINE virtual TArray<AXkHexagonActor*> GetHexagonActorNeighbors(const AXkHexagonActor* InputActor) const;

	/**
	* @brief Find all valid neighbors of input hexagon
	* @param InputActor The actor's neighbors would be search
	* @param NeighborDistance The distance of search neighbors
	*/
	FORCEINLINE virtual TArray<AXkHexagonActor*> GetHexagonActorNeighbors(const AXkHexagonActor* InputActor, const uint8 NeighborDistance) const;

	/**
	* @brief Find all valid neighbors of input hexagon recursively, if all neighbors in BlockList, recursive search expand area.
	* @param InputActor The actor's neighbors would be search
	* @param BlockList Make sure finding neighbors not in BlockList
	*/
	FORCEINLINE virtual TArray<AXkHexagonActor*> GetHexagonActorNeighborsRecursively(const AXkHexagonActor* InputActor, const TArray<FIntVector>& BlockList) const;

	/**
	* @brief Find random neighbor hexagon of input
	* @param InputActor The actor's neighbors would be search
	* @param BlockList Make sure the neighbors not in block list
	*/
	FORCEINLINE virtual AXkHexagonActor* GetHexagonActorRandomNeighbor(const AXkHexagonActor* InputActor, const TArray<FIntVector>& BlockList) const;

	/** 
	* @brief Find nearest hexagon of target to input
	* @param InputActor The actor nearest to
	* @param TargetActor The actor's neighbors would be search and measure
	* @param BlockList Make sure the neighbors not in block list
	*/
	FORCEINLINE virtual AXkHexagonActor* GetHexagonActorNearestNeighbor(const AXkHexagonActor* InputActor, const AXkHexagonActor* TargetActor, const TArray<FIntVector>& BlockList) const;


	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList);
	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const AXkHexagonActor* StartActor, const AXkHexagonActor* EndActor, const TArray<FIntVector>& BlockList);
	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const FVector& StartLocation, const FVector& EndLocation, const TArray<FIntVector>& BlockList);

	const TMap<FIntVector, FXkHexagonNode>& GetHexagonalWorldNodes() const { return HexagonalWorldTable.Nodes; };
	virtual void FetchHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices);
	virtual FVector2D GetHexagonalWorldExtent() const;
	virtual FVector2D GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const;

	TArray<class AXkHexagonActor*> FindHexagonActors(const TArray<FIntVector>& Inputs) const;
	class AXkHexagonActor* FindHexagonActor(const FIntVector& Input) const;
protected:
	UPROPERTY()
	FXkHexagonalWorldNodeTable HexagonalWorldTable;
	UPROPERTY()
	FXkHexagonAStarPathfinding HexagonAStarPathfinding;
};


/**
* Class could spawn into hexagonal world in hexagonal coordinated space.
*/
UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonalSpawnableActor : public AActor
{
	GENERATED_BODY()
};