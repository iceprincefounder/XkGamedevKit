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

/**
 * Hexagon Type
 */
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EXkHexagonType : uint32
{
	Land = 1 << 0,
	Grass = 1 << 1,
	Forest = 1 << 2,
	Sand = 1 << 3,
	Mountain = 1 << 4,
	Frost = 1 << 5,
	ShallowWater = 1 << 6,
	DeepWater = 1 << 7,
	Unavailable = 1 << 8
};

inline EXkHexagonType operator|(EXkHexagonType lhs, EXkHexagonType rhs)
{
	return static_cast<EXkHexagonType>(static_cast<uint32>(lhs) | static_cast<uint32>(rhs));
}

inline EXkHexagonType operator|=(EXkHexagonType& lhs, EXkHexagonType rhs)
{
	return lhs = static_cast<EXkHexagonType>(static_cast<uint32>(lhs) | static_cast<uint32>(rhs));
}

inline EXkHexagonType operator&(EXkHexagonType lhs, EXkHexagonType rhs)
{
	return static_cast<EXkHexagonType>(static_cast<uint32>(lhs) & static_cast<uint32>(rhs));
}

inline EXkHexagonType operator&=(EXkHexagonType& lhs, EXkHexagonType rhs)
{
	return rhs = static_cast<EXkHexagonType>(static_cast<uint32>(lhs) & static_cast<uint32>(rhs));
}

inline bool operator==(EXkHexagonType lhs, EXkHexagonType rhs)
{
	return ((static_cast<uint32>(lhs) & static_cast<uint32>(rhs))) == static_cast<uint32>(rhs);
}


/**
 * Hexagon Splat
 */
USTRUCT(BlueprintType, Blueprintable)
struct FXkHexagonSplat
{
	GENERATED_BODY()

	FXkHexagonSplat() : TargetType(EXkHexagonType::Unavailable), Height(0.0f), Color(FLinearColor::White), Splats() {};
public:
	UPROPERTY(EditAnywhere, Category = "HexagonSplat [KEVINTSUIXUGAMEDEV]")
	EXkHexagonType TargetType;

	UPROPERTY(EditAnywhere, Category = "HexagonSplat [KEVINTSUIXUGAMEDEV]")
	float Height;

	UPROPERTY(EditAnywhere, Category = "HexagonSplat [KEVINTSUIXUGAMEDEV]")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, Category = "HexagonSplat [KEVINTSUIXUGAMEDEV]")
	TArray<uint8> Splats;
};

/**
 * Hexagon Node
 */
USTRUCT(BlueprintType, Blueprintable)
struct XKGAMEDEVCORE_API FXkHexagonNode
{
	GENERATED_BODY()

public:
	FXkHexagonNode()
	{
		Type = EXkHexagonType::Unavailable;
		Position = FVector4f::Zero();
		BaseColor = FLinearColor::Black;
		EdgeColor = FLinearColor::Black;
		Splatmap = 0;
		Coord = FIntVector::ZeroValue;
	};
	FXkHexagonNode(
		const EXkHexagonType InType, const FVector4f& InPosition, const FLinearColor& InBaseColor, const FLinearColor& InEdgeColor, const uint8 InSplatmap, const FIntVector& InCoord):
		Type(InType),
		Position(InPosition),
		BaseColor(InBaseColor),
		EdgeColor(InEdgeColor),
		Splatmap(InSplatmap),
		Coord(InCoord) {};
	~FXkHexagonNode()
	{
	};

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	EXkHexagonType Type;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FVector4f Position;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FLinearColor BaseColor;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FLinearColor EdgeColor;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	uint8 Splatmap;

	UPROPERTY(EditAnywhere, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FIntVector Coord;

	UPROPERTY(Transient)
	FXkPathCostValue Cost;

	FXkHexagonNode& operator= (const FXkHexagonNode& rhs)
	{
		Type = rhs.Type;
		Position = rhs.Position;
		BaseColor = rhs.BaseColor;
		EdgeColor = rhs.EdgeColor;
		Splatmap = rhs.Splatmap;
		Coord = rhs.Coord;
		Cost = rhs.Cost;
		return *this;
	};
};


/**
 * Hexagon Node Table
 */
USTRUCT(BlueprintType, Blueprintable)
struct XKGAMEDEVCORE_API FXkHexagonalWorldNodeTable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "HexagonTable [KEVINTSUIXUGAMEDEV]")
	TMap<FIntVector, FXkHexagonNode> Nodes;
};


/**
 * Hexagon AStar Pathfinding Algorithm
 * https://blog.theknightsofunity.com/pathfinding-on-lhs-hexagonal-grid-lhs-algorithm/
 */
USTRUCT(BlueprintType, Blueprintable)
struct XKGAMEDEVCORE_API FXkHexagonAStarPathfinding
{
	GENERATED_BODY()

public:
	FXkHexagonAStarPathfinding();
	~FXkHexagonAStarPathfinding();

public:
	void Init(FXkHexagonalWorldNodeTable* InNodeTable);
	void Reinit();
	void Blocking(const TArray<FIntVector>& Input);
	bool Pathfinding(const FIntVector& StartingPoint, const FIntVector& TargetPoint, int32 MaxStep = 9999);
	TArray<FIntVector> Backtracking(const int32 MaxStep = 9999) const;
	TArray<FIntVector> SearchArea() const;
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
	FIntVector TheStartPoint; // starting point
	UPROPERTY(Transient)
	FIntVector TheTargetPoint; // target point

	FXkHexagonalWorldNodeTable* HexagonalWorldTable;
};


/**
 * XkHexagon DStar Pathfinding Algorithm
 */
class XkHexagonDStarPathfinding
{
};