// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "XkHexagonPathfinding.generated.h"

// 1024 x 1024
#define	MAX_HEXAGON_NODE_COUNT 1048576

template<typename T0, typename T1>
extern void BuildHexagon(TArray<T0>& OutBaseVertices, TArray<T1>& OutBaseIndices, TArray<T0>& OutEdgeVertices, TArray<T1>& OutEdgeIndices,
	float Radius, float Height, float BaseInnerGap, float BaseOuterGap, float EdgeInnerGap, float EdgeOuterGap);

extern void CullHexagonalWorld(TArray<FXkHexagonNode*> OutHexagonNodes, const TMap<FIntVector, FXkHexagonNode>& HexagonalWorldNodes, const FSceneView& View, const float Distance);

static float Sin30 = FMath::Sin(UE_DOUBLE_PI / (180.0) * 30.0);
static float Cos30 = FMath::Cos(UE_DOUBLE_PI / (180.0) * 30.0);
static float XkSin30 = 0.5;
static float XkCos30 = 0.86602540378443864676372317075294;
static float XkCos30d2 = 0.43301270189221932338186158537647;
static float XkCos30x2 = 1.7320508075688772935274463415059;
static float XkCos45 = 0.70710678118654752440084436210485;
static float XkCos60 = 0.5;
static float XkCos30xCos45x2 = 1.224744871391589049098642037353;

class AXkHexagonActor;

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
	FXkHexagonNode()
	{
		Position = FVector4f::Zero();
		Color = FLinearColor::Black;
		Splatmap = 0;
		Coord = FIntVector::ZeroValue;
	};
	FXkHexagonNode(
		const FVector4f& InPosition, const FLinearColor& InColor, const uint8 InSplatmap, const FIntVector& InCoord)
		:
		Position(InPosition),
		Color(InColor),
		Splatmap(InSplatmap),
		Coord(InCoord) {};
	FXkHexagonNode(AXkHexagonActor* InActor)
	{
		Actor = MakeWeakObjectPtr(InActor);
	};
	~FXkHexagonNode()
	{
		Actor.Reset();
	};

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXU GAMEDEV]")
	FVector4f Position;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXU GAMEDEV]")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXU GAMEDEV]")
	uint8 Splatmap;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXU GAMEDEV]")
	FIntVector Coord;

	UPROPERTY(Transient)
	TWeakObjectPtr<class AXkHexagonActor> Actor;

	UPROPERTY(Transient)
	FXkPathCostValue Cost;

	FXkHexagonNode& operator= (const FXkHexagonNode& rhs)
	{
		Position = rhs.Position;
		Color = rhs.Color;
		Splatmap = rhs.Splatmap;
		Coord = rhs.Coord;
		Actor = rhs.Actor;
		Cost = rhs.Cost;
		return *this;
	};
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
	TArray<FIntVector> Backtracking(const int32 MaxStep = 9999) const;
	TArray<FIntVector> SearchArea() const;

	TArray<class AXkHexagonActor*> FindHexagonActors(const TArray<FIntVector>& Inputs) const;
	class AXkHexagonActor* FindHexagonActor(const FIntVector& Input) const;

public:
	static FXkPathCostValue CalcPathCostValue(const FIntVector& StartingPoint,
		const FIntVector& ConsideredPoint, const FIntVector& TargetPoint, int32 Offset = 0);
	static int32 CalcManhattanDistance(const FIntVector& PointA, const FIntVector& PointB);
	static FIntVector CalcHexagonCoord(const float PositionX, const float PositionY, const float XkHexagonRadius);
	/** 
	* @brief Calculate hexagon actor position by Cartesian coordinate XY index number
	* @param IndexX Cartesian coordinate X index
	* @param IndexY Cartesian coordinate Y index
	* @Distance Distance between two hexagons
	*/
	static FVector2D CalcHexagonPosition(const int32 IndexX, const int32 IndexY, const float Distance);
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