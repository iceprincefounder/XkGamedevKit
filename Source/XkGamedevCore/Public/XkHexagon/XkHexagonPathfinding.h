// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonPathfinding.generated.h"

class AXkHexagonActor;

static float XkSin30 = 0.5;
static float XkCos30 = FMath::Cos(UE_DOUBLE_PI / (180.0) * 30.0);

USTRUCT(BlueprintType, Blueprintable)
struct FXkPathCostValue
{
	GENERATED_BODY()

public:
	FXkPathCostValue() : F(0), G(0), H(0), R(0) {};
	FXkPathCostValue(int32 A, int32 B) : F(A + B), G(A), H(B), R(0) {};
	FXkPathCostValue(int32 A, int32 B, int32 C) : F(A + B + C), G(A), H(B), R(C) {};

	int32 F; // F = G + H
	int32 G; // the starting point to the considered point.
	int32 H; // the considered point to the target point.
	int32 R; // the considered point cost offset.

	bool operator>(const FXkPathCostValue& other)
	{
		return (F > other.F);
	};

	bool operator>=(const FXkPathCostValue& other)
	{
		return (F >= other.F);
	};

	bool operator<(const FXkPathCostValue& other)
	{
		return (F < other.F);
	};

	bool operator<=(const FXkPathCostValue& other)
	{
		return (F <= other.F);
	};

	bool operator==(const FXkPathCostValue& other)
	{
		return (F == other.F);
	};

	FXkPathCostValue& operator=(const FXkPathCostValue& other)
	{
		// Guard self assignment
		if (this == &other)
			return *this;
		F = other.F;
		G = other.G;
		H = other.H;
		R = other.R;
		return *this;
	}
};


USTRUCT(BlueprintType, Blueprintable)
struct XKGAMEDEVCORE_API FXkHexagonNode
{
	GENERATED_BODY()

public:
	FXkHexagonNode() {};
	FXkHexagonNode(class AXkHexagonActor* InActor);
	~FXkHexagonNode();

	UPROPERTY()
	TWeakObjectPtr<class AXkHexagonActor> Actor;
	UPROPERTY()
	FXkPathCostValue Cost;

	FXkHexagonNode& operator=(const FXkHexagonNode& other);
};


/**
 * Hexagon AStar Pathfinding Algorithm
 * https://blog.theknightsofunity.com/pathfinding-on-a-hexagonal-grid-a-algorithm/
 */
USTRUCT(BlueprintType, Blueprintable)
struct XKGAMEDEVCORE_API FXkHexagonAStarPathfinding
{
	GENERATED_BODY()

public:
	FXkHexagonAStarPathfinding();
	~FXkHexagonAStarPathfinding();

public:
	void Init(UWorld* InWorld);
	void Reinit();
	void Blocking(const TArray<FIntVector>& Input);
	bool Pathfinding(const FIntVector& StartingPoint, const FIntVector& TargetPoint, int32 MaxStep = 9999);
	TArray<FIntVector> Backtracking(int32 MaxStep = 9999) const;
	TArray<FIntVector> SearchArea() const;

	TArray<class AXkHexagonActor*> FindHexagonActors(const TArray<FIntVector>& Inputs) const;
	class AXkHexagonActor* FindHexagonActor(const FIntVector& Input) const;

public:
	static FXkPathCostValue CalcPathCostValue(const FIntVector& StartingPoint,
		const FIntVector& ConsideredPoint, const FIntVector& TargetPoint, int32 Offset = 0);
	static int32 CalcManhattanDistance(const FIntVector& PointA, const FIntVector& PointB);
	static FIntVector CalcHexagonCoord(float PositionX, float PositionY, float XkHexagonRadius);
	/** 
	* @brief Calculate hexagon actor position by Cartesian coordinate XY index number
	* @param IndexX Cartesian coordinate X index
	* @param IndexY Cartesian coordinate Y index
	* @Distance Distance between two hexagons
	*/
	static FVector2D CalcHexagonPosition(int32 IndexX, int32 IndexY, float Distance);
	static TArray<FIntVector> CalcHexagonNeighboringCoord(const FIntVector& InputCoord);
protected:
	UPROPERTY(Transient)
	TArray<FIntVector> OpenList;
	UPROPERTY(Transient)
	TArray<FIntVector> ClosedList;
	UPROPERTY(Transient)
	TArray<FIntVector> BlockList;
	UPROPERTY(Transient)
	TMap<FIntVector, FXkHexagonNode> HexagonalWorldTable;
	UPROPERTY(Transient)
	FIntVector TheStartPoint; // starting point
	UPROPERTY(Transient)
	FIntVector TheTargetPoint; // target point
};


/**
 * XkHexagon DStar Pathfinding Algorithm
 */
class XkHexagonDStarPathfinding
{
};