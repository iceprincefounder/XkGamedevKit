// Copyright ©xukai. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonComponents.h"
#include "XkHexagonPathfinding.h"
#include "XkHexagonActors.generated.h"

// when EXkHexagonType is greater that AVAILABLEMARK,
// which mean that actor is unreachable!
#define AVAILABLEMARK 2 // Frost for now
#define BASE_SECTION_INDEX 0
#define EDGE_SECTION_INDEX 1

class UProceduralMeshComponent;


UENUM(BlueprintType, Blueprintable)
enum class EXkHexagonType : uint8
{
	Land = 0,
	Road,
	Forest,
	Cliff
};


UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonActor : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UProceduralMeshComponent* ProcMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	bool bShowBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	bool bShowEdgeMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	FLinearColor EdgeColor;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UMaterialInstanceDynamic* BaseMID;

	UPROPERTY(VisibleAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	class UMaterialInstanceDynamic* EdgeMID;

	UPROPERTY(EditAnywhere, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	EXkHexagonType HexagonType;

public:
	AXkHexagonActor();

	UFUNCTION(CallInEditor, Category = "HexagonActor [KEVINTSUIXU GAMEDEV]")
	void SnapToLandscape();

	void ConstructionScripts();
	void OnConstruction(const FTransform& Transform) override;

public:
	bool IsAccessible() const;
	int32 CalcCostOffset() const;

	//~ Begin AXkHexagonActor Interface
	float GetRadius() const { return Radius; };
	void SetRadius(float Input) { Radius = Input; };
	float GetBaseInnerGap() const { return BaseInnerGap; };
	void SetBaseInnerGap(float Input) { BaseInnerGap = Input; };
	float GetEdgeInnerGap() const { return EdgeInnerGap; };
	void SetEdgeInnerGap(float Input) { EdgeInnerGap = Input; };
	float GetEdgeOuterGap() const { return EdgeOuterGap; };
	void SetEdgeOuterGap(float Input) { EdgeOuterGap = Input; };
	float GetHeight() const { return Height; };
	void SetHeight(float Input) { Height = Input; };
	FIntVector GetCoord() const { return Coord; };
	void SetCoord(const FIntVector& Input) { Coord = Input; };
	bool IsShowBaseMesh() const{ return bShowBaseMesh; };
	void SetShowBaseMesh(const bool Input) { bShowBaseMesh = Input; };
	bool IsShowEdgeMesh() const { return bShowEdgeMesh; };
	void SetShowEdgeMesh(const bool Input) { bShowEdgeMesh = Input; };
	FLinearColor GetBaseColor() const { return BaseColor; };
	void SetBaseColor(const FLinearColor& Input);
	FLinearColor GetEdgeColor() const { return EdgeColor; };
	void SetEdgeColor(const FLinearColor& Input);
	EXkHexagonType GetHexagonType() const { return HexagonType; };
	void SetHexagonType(EXkHexagonType Input) { HexagonType = Input; };
	TWeakObjectPtr<class AXkHexagonalWorldActor> GetHexagonalWorld() const { return CachedHexagonalWorld; };
	void SetHexagonWorld(class AXkHexagonalWorldActor* Input);
	//~ End AXkHexagonActor Interface

	void OnBaseSelecting(bool bSelecting, const FLinearColor& SelectingColor = FLinearColor::White);
	void OnBaseHighlight(bool bHighlight, const FLinearColor& HighlightColor = FLinearColor::White);
	void OnEdgeSelecting(bool bSelecting, const FLinearColor& SelectingColor = FLinearColor::White);
	void OnEdgeHighlight(bool bHighlight, const FLinearColor& HighlightColor = FLinearColor::White);

	void UpdateMaterial();
	void UpdateProcMesh();

private:
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
};


UCLASS(BlueprintType, Blueprintable)
class XKGAMEDEVCORE_API AXkHexagonalWorldActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TObjectPtr<class UXkHexagonArrowComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 PathfindingMaxStep;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 BacktrackingMaxStep;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class AXkHexagonActor> HexagonStarter;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class AXkHexagonActor> HexagonTargeter;

	UPROPERTY(EditAnywhere, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class AXkHexagonActor>> HexagonBlockers;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Radius;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float BaseInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeInnerGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float EdgeOuterGap;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float GapWidth;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 XAxisCount;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	int32 YAxisCount;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowBaseMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bShowEdgeMesh;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* EdgeMaterial;

public:
	UFUNCTION(CallInEditor, Category = "HexagonalWorld [KEVINTSUIXU GAMEDEV]")
	void Generate();

	UFUNCTION(CallInEditor, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	void Pathfinding();

	UFUNCTION(CallInEditor, Category = "PathfindingDebug [KEVINTSUIXU GAMEDEV]")
	void VisBoundary();

	//~ Begin Actor Interface
	void BeginPlay() override;
	void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface
public:
	FORCEINLINE virtual bool IsHexagonActorsNeighboring(const AXkHexagonActor* A, const AXkHexagonActor* B) const;

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

	FORCEINLINE virtual int32 GetHexagonManhattanDistance(const FVector& A, const FVector& B) const;
	FORCEINLINE virtual int32 GetHexagonManhattanDistance(const AXkHexagonActor* A, const AXkHexagonActor* B) const;

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
	FXkHexagonAStarPathfinding HexagonAStarPathfinding;
};