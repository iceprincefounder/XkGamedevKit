// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

// STL
#include <random>

#include "CoreMinimal.h"
#include "XkHexagonPathfinding.generated.h"

// 1024 x 1024
#define	MAX_HEXAGON_NODE_COUNT 1048576

static float Sin30 = FMath::Sin(UE_DOUBLE_PI / (180.0) * 30.0);
static float Cos30 = FMath::Cos(UE_DOUBLE_PI / (180.0) * 30.0);
static float XkSin30 = 0.5;
static float XkCos30 = 0.86602540378443864676372317075294;
static float XkCos30d2 = 0.43301270189221932338186158537647;
static float XkCos30x2 = 1.7320508075688772935274463415059;
static float XkCos45 = 0.70710678118654752440084436210485;
static float XkCos60 = 0.5;
static float XkCos30xCos45x2 = 1.224744871391589049098642037353;

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
UENUM(BlueprintType, meta = (Bitflags))
enum class EXkHexagonType : uint8
{
	None		= 0x00,
	Unavailable = 1 << 0,
	Land		= 1 << 1,
	Beach		= 1 << 2,
	Ocean		= 1 << 3,
};
ENUM_CLASS_FLAGS(EXkHexagonType);


/**
 * Hexagon Splat
 */
USTRUCT(BlueprintType, Blueprintable)
struct FXkHexagonSplat
{
	GENERATED_BODY()

	FXkHexagonSplat() : TargetType(EXkHexagonType::Unavailable),  Splats() {};
public:
	UPROPERTY(EditAnywhere, Category = "HexagonSplat [KEVINTSUIXUGAMEDEV]")
	EXkHexagonType TargetType;

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
		Splatmap = 0;
		Coord = FIntVector::ZeroValue;
		CustomData = FVector4f::Zero();
	};
	FXkHexagonNode(
		const EXkHexagonType InType, const FVector4f& InPosition, const uint8 InSplatmap, const FIntVector& InCoord) :
		Type(InType),
		Position(InPosition),
		Splatmap(InSplatmap),
		Coord(InCoord),
		CustomData(FVector4f::Zero())
		{};
	~FXkHexagonNode()
	{
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	EXkHexagonType Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FVector4f Position; // Position.W for hexagon radius.

	/* Material texture id.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	uint8 Splatmap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FIntVector Coord;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FVector4f CustomData;

	UPROPERTY(Transient)
	FXkPathCostValue Cost;

	FXkHexagonNode& operator= (const FXkHexagonNode& rhs)
	{
		Type = rhs.Type;
		Position = rhs.Position;
		CustomData = rhs.CustomData;
		Splatmap = rhs.Splatmap;
		Coord = rhs.Coord;
		Cost = rhs.Cost;
		return *this;
	};

	FVector GetLocation() const { return FVector(Position.X, Position.Y, Position.Z); }
	float GetRadius() const { return Position.W; }
	TArray<FVector> GetVertices() const {
		TArray<FVector> Results;
		FVector Location = FVector(Position.X, Position.Y, Position.Z);
		float Radius = Position.W;
		Results.Add(Location + FVector(Radius, 0.0, 0.0));
		Results.Add(Location + FVector(Radius * XkSin30, -XkCos30 * Radius, 0.0));
		Results.Add(Location + FVector(-Radius * XkSin30, -XkCos30 * Radius, 0.0));
		Results.Add(Location + FVector(-Radius, 0.0, 0.0));
		Results.Add(Location + FVector(-Radius * XkSin30, XkCos30 * Radius, 0.0));
		Results.Add(Location + FVector(Radius * XkSin30, XkCos30 * Radius, 0.0));
		return Results;
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
	static FXkPathCostValue CalcPathCostValue(const FIntVector& StartingPoint, const FIntVector& ConsideredPoint, const FIntVector& TargetPoint, int32 Offset = 0);
	static int32 CalcManhattanDistance(const FIntVector& PointA, const FIntVector& PointB);
	static FIntVector CalcHexagonCoord(const float PositionX, const float PositionY, const float XkHexagonRadius);
	/** 
	* @brief Calculate hexagon actor position by Cartesian coordinate XY index number
	* @param IndexX Cartesian coordinate X index
	* @param IndexY Cartesian coordinate Y index
	* @return Distance between two hexagons
	*/
	static FVector2D CalcHexagonPosition(const int32 IndexX, const int32 IndexY, const float Distance);
	/**
	* @brief Calculate hexagon neighbors coords
	* @param InputCoord hexagon coord to calculate
	* @return Array of hexagon neighbors coords
	*/
	static TArray<FIntVector> CalcHexagonNeighboringCoord(const FIntVector& InputCoord);
	/**
	* @brief Calculate hexagon surrounding coords
	* @param InputCoords hexagon coords to calculate
	* @return Array of hexagon surrounding coords
	*/
	static TArray<FIntVector> CalcHexagonSurroundingCoord(const TArray<FIntVector>& InputCoords);
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

template<typename T0, typename T1>
extern void BuildHexagon(TArray<T0>& OutBaseVertices, TArray<T1>& OutBaseIndices, TArray<T0>& OutEdgeVertices, TArray<T1>& OutEdgeIndices,
	float Radius, float Height, float BaseInnerGap, float BaseOuterGap, float EdgeInnerGap, float EdgeOuterGap);

FORCEINLINE static int RandRangeIntMT(int seed, int min, int max)
{
	std::mt19937 gen(seed); // Initialize Mersenne Twister algorithm generator with seed value
	std::uniform_int_distribution<> dis(min, max); // Define a uniform distribution from min to max
	return dis(gen); // Generate random number
};

FORCEINLINE static float RandRangeFloatMT(int seed, float min, float max)
{
	std::mt19937 gen(seed); // Initialize Mersenne Twister algorithm generator with seed value
	std::uniform_real_distribution<> dis(min, max); // Define a uniform distribution from min to max
	return dis(gen); // Generate random number
};

FORCEINLINE static bool RandRangeBoolMT(int seed)
{
	std::mt19937 gen(seed); // Initialize Mersenne Twister algorithm generator with seed value
	std::uniform_int_distribution<> dis(0, 1); // Define a uniform distribution from 0 to 1
	return dis(gen) == 1; // Generate random number
};


FORCEINLINE uint32_t PackFloatsToUint32(float a, float b, float c) 
{
	uint32_t binaryA, binaryB, binaryC;
	std::memcpy(&binaryA, &a, sizeof(float));
	std::memcpy(&binaryB, &b, sizeof(float));
	std::memcpy(&binaryC, &c, sizeof(float));

	uint32_t result = ((binaryA >> 22) & 0x3FF) << 20 |
		((binaryB >> 22) & 0x3FF) << 10 |
		((binaryC >> 22) & 0x3FF);
	return result;
}

FORCEINLINE static FVector HexagonNodeXAxis()
{
	FVector XAxis = FVector(1.0, 0.0, 0.0);
	XAxis.Normalize();
	return XAxis;
}

FORCEINLINE static FVector HexagonNodeYAxis()
{
	FVector YAxis = FRotator(0, -120, 0).RotateVector(HexagonNodeXAxis());
	YAxis.Normalize();
	return YAxis;
}

FORCEINLINE static FVector HexagonNodeZAxis()
{
	FVector ZAxis = FRotator(0, 120, 0).RotateVector(HexagonNodeXAxis());
	ZAxis.Normalize();
	return ZAxis;
}

FORCEINLINE static bool HexagonNodeIsValidLowLevel(const FXkHexagonNode* Node)
{
	if (Node)
	{
		return true;
	}
	return false;
}

FORCEINLINE static bool HexagonNodeIsValid(const FXkHexagonNode* Node)
{
	return Node && !EnumHasAnyFlags(Node->Type, EXkHexagonType::Unavailable);
}

FORCEINLINE static bool HexagonNodeHasType(const FXkHexagonNode* Node, const EXkHexagonType InType)
{
	return Node && EnumHasAnyFlags(Node->Type, InType);
}

FORCEINLINE static int32 HexagonNodeRandomSeed(FXkHexagonNode* Node)
{
	check(Node);
	FVector Vector = FVector(Node->Position.X, Node->Position.Y, Node->Position.Z);
	FString Seed = Vector.ToString();
	//uint32 Hash = PackFloatsToUint32(Vector.X, Vector.Y, Vector.Z);
	uint32 Hash = GetTypeHash(Seed);
	return static_cast<int32>(Hash);
}

FORCEINLINE static float HexagonNodeGetZ(FXkHexagonNode* Node)
{
	check(Node);
	return Node->Position.Z;
}

FORCEINLINE static void HexagonNodeSetZ(FXkHexagonNode* Node, const float Height)
{
	check(Node);
	Node->Position.Z = Height;
}

FORCEINLINE static uint8 HexagonNodeGetSplatID(FXkHexagonNode* Node)
{
	check(Node);
	return Node->Splatmap;
}