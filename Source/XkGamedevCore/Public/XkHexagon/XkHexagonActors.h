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

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UProceduralMeshComponent* ProcMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	bool bShowBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	bool bShowEdgeMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	EXkHexagonType HexagonType;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UMaterialInstanceDynamic* BaseMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UMaterialInstanceDynamic* EdgeMID;
public:
	AXkHexagonActor();

	UFUNCTION(CallInEditor, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	void SnapToLandscape();

	virtual void ConstructionScripts();
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	void PostEditMove(bool bFinished) override;
#endif

public:
	//~ Begin AXkHexagonActor Interface
	virtual float GetRadius() const { return Radius; };
	virtual void SetRadius(float Input) { Radius = Input; };
	virtual float GetHeight() const { return Height; };
	virtual void SetHeight(float Input) { Height = Input; };
	virtual float GetGapWidth() const { return GapWidth; };
	virtual void SetGapWidth(float Input) { GapWidth = Input; };
	virtual float GetBaseInnerGap() const { return BaseInnerGap; };
	virtual void SetBaseInnerGap(float Input) { BaseInnerGap = Input; };
	virtual float GetBaseOuterGap() const { return BaseOuterGap; };
	virtual void SetBaseOuterGap(float Input) { BaseOuterGap = Input; };
	virtual float GetEdgeInnerGap() const { return EdgeInnerGap; };
	virtual void SetEdgeInnerGap(float Input) { EdgeInnerGap = Input; };
	virtual float GetEdgeOuterGap() const { return EdgeOuterGap; };
	virtual void SetEdgeOuterGap(float Input) { EdgeOuterGap = Input; };
	virtual FLinearColor GetBaseColor() const { return BaseColor; };
	virtual void SetBaseColor(const FLinearColor& Input) { BaseColor = Input; };
	virtual FLinearColor GetEdgeColor() const { return EdgeColor; };
	virtual void SetEdgeColor(const FLinearColor& Input) { EdgeColor = Input; };
	virtual bool IsShowBaseMesh() const { return bShowBaseMesh; };
	virtual void SetShowBaseMesh(const bool Input) { bShowBaseMesh = Input; };
	virtual bool IsShowEdgeMesh() const { return bShowEdgeMesh; };
	virtual void SetShowEdgeMesh(const bool Input) { bShowEdgeMesh = Input; };
	virtual FIntVector GetCoord() const { return Coord; };
	virtual void SetCoord(const FIntVector& Input) { Coord = Input; };
	virtual EXkHexagonType GetHexagonType() const { return HexagonType; };
	virtual void SetHexagonType(EXkHexagonType Input) { HexagonType = Input; };
	virtual FXkHexagonNode* GetHexagonNode() { return HexagonNode; };
	virtual void SetHexagonNode(FXkHexagonNode* Input) { HexagonNode = Input; HexagonNode->Actor = MakeWeakObjectPtr(this); };
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
	void UpdateProcMesh();

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
	UPROPERTY(VisibleAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	TObjectPtr<class UXkHexagonArrowComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	TObjectPtr<class UXkHexagonalWorldComponent> HexagonalWorldComponent;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	int32 PathfindingMaxStep;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	int32 BacktrackingMaxStep;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonStarter;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	TObjectPtr<class AXkHexagonActor> HexagonTargeter;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	TArray<TObjectPtr<class AXkHexagonActor>> HexagonBlockers;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	uint32 GeneratingMaxStep;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	uint32 CenterFieldRange;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	FVector2D PositionRandom;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	float PositionScale;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	float FalloffRadius;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	FVector2D FalloffCenter;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	FVector2D FalloffExtent;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	float FalloffCornerRadii;

	UPROPERTY(EditAnywhere, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	TArray<FXkHexagonSplat> HexagonSplats;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float BaseOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	bool bSpawnActors;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	int32 SpawnActorsMaxMhtDist;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	int32 XAxisCount;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	int32 YAxisCount;

	/* Max Manhattan distance to center.*/
	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	int32 MaxManhattanDistance;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	bool bShowHexagonActorsBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	bool bShowHexagonActorsEdgeMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	bool bShowHexagonalWorldBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	bool bShowHexagonalWorldEdgeMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	class UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	class UMaterialInterface* EdgeMaterial;

	friend class AXkHexagonActor;

	AXkHexagonalWorldActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
public:
	UFUNCTION(CallInEditor, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	void GenerateHexagons();

	UFUNCTION(CallInEditor, Category = "MainWorldGenerate [KEVINTSUIXU GAMEDEV]")
	void GenerateWorld();

	UFUNCTION(CallInEditor, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	void Pathfinding();

	UFUNCTION(CallInEditor, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	void VisBoundary();

	//~ Begin Actor Interface
	void BeginPlay() override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	virtual void UpdateHexagonalWorld() {};
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

	/**
	* @brief Find current hexagonal world boundary
	* @return List of hexagon actor on boundary
	*/
	FORCEINLINE virtual TArray<AXkHexagonActor*> GetHexagonalWorldBoundary() const;

	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const FIntVector& StartCoord, const FIntVector& EndCoord, const TArray<FIntVector>& BlockList);
	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const AXkHexagonActor* StartActor, const AXkHexagonActor* EndActor, const TArray<FIntVector>& BlockList);
	virtual TArray<AXkHexagonActor*> PathfindingHexagonActors(const FVector& StartLocation, const FVector& EndLocation, const TArray<FIntVector>& BlockList);
private:
	UPROPERTY()
	FXkHexagonNodeTable HexagonalWorldTable;
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