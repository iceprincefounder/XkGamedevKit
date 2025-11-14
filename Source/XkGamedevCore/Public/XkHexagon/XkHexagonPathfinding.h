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
	Unavailable = 0x00,
	Ground		= 1 << 0,
	Beach		= 1 << 1,
	Ocean		= 1 << 2,
	CustomType1	= 1 << 3,
	CustomType2	= 1 << 4,
	CustomType3	= 1 << 5,
	CustomType4 = 1 << 6,
	CustomType5 = 1 << 7,
};
ENUM_CLASS_FLAGS(EXkHexagonType);


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
		Weights = FVector4f::Zero();
		Coord = FIntVector::ZeroValue;
		CustomData = FVector4f::Zero();
	};
	FXkHexagonNode(
		const EXkHexagonType InType, const FVector4f& InPosition, const FVector4f InWeights, const FIntVector& InCoord) :
		Type(InType),
		Position(InPosition),
		Weights(InWeights),
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

	/* Material texture weights.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HexagonNode [KEVINTSUIXUGAMEDEV]")
	FVector4f Weights;

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
		Weights = rhs.Weights;
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
	static FIntVector CalcHexagonCoord(const float PositionX, const float PositionY, const float HexagonRadius);
	static FVector2D CalcHexagonPosition(const FIntVector& InputCoord, const float HexagonRadius);
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
	float Radius, float Height, float BaseInnerGap, float BaseOuterGap, float EdgeInnerGap, float EdgeOuterGap)
{
	TArray<T0> TopBoundary;
	TArray<T0> BtmBoundary;
	{
		//	x
		//	|   1
		//	| 2/ \6
		//	| | 0 |
		//	| 3\ /5
		//	|   4
		//	---------y

		OutBaseVertices.Add(T0(0.0, 0.0, Height));
		TopBoundary.Empty();
		TopBoundary.Add(T0((Radius - BaseInnerGap), 0.0, Height));
		TopBoundary.Add(T0((Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundary.Add(T0(-(Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundary.Add(T0(-(Radius - BaseInnerGap), 0.0, Height));
		TopBoundary.Add(T0(-(Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundary.Add(T0((Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		for (const T0& Vert : TopBoundary)
		{
			OutBaseVertices.Add(Vert);
		}
		OutBaseIndices = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0 , 5, 6, 0, 6, 1 };
		BtmBoundary.Empty();
		BtmBoundary.Add(T0((Radius - BaseOuterGap), 0.0, 0.0));
		BtmBoundary.Add(T0((Radius - BaseOuterGap) * XkSin30, -XkCos30 * Radius, 0.0));
		BtmBoundary.Add(T0(-(Radius - BaseOuterGap) * XkSin30, -XkCos30 * Radius, 0.0));
		BtmBoundary.Add(T0(-(Radius - BaseOuterGap), 0.0, 0.0));
		BtmBoundary.Add(T0(-(Radius - BaseOuterGap) * XkSin30, XkCos30 * Radius, 0.0));
		BtmBoundary.Add(T0((Radius - BaseOuterGap) * XkSin30, XkCos30 * Radius, 0.0));
		for (int32 i = 0; i < TopBoundary.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % TopBoundary.Num();
			int32 IndexTopB = (i + 1 + 1) % TopBoundary.Num();
			int32 IndexBtmA = (i + 1) % TopBoundary.Num();
			int32 IndexBtmB = (i + 1 + 1) % TopBoundary.Num();

			T0 VertTopA = TopBoundary[IndexTopA];
			T0 VertTopB = TopBoundary[IndexTopB];
			T0 VertBtmA = BtmBoundary[IndexBtmA];
			T0 VertBtmB = BtmBoundary[IndexBtmB];

			int32 CurrIndex = OutBaseVertices.Num();
			OutBaseVertices.Add(VertTopA);
			OutBaseVertices.Add(VertTopB);
			OutBaseVertices.Add(VertBtmA);
			OutBaseVertices.Add(VertBtmB);

			OutBaseIndices.Add(CurrIndex);
			OutBaseIndices.Add(CurrIndex + 2);
			OutBaseIndices.Add(CurrIndex + 1);
			OutBaseIndices.Add(CurrIndex + 1);
			OutBaseIndices.Add(CurrIndex + 2);
			OutBaseIndices.Add(CurrIndex + 3);
		}
		//TrianglesArray = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0 , 5, 6, 0, 6, 1,
		//1, 7, 2, 2, 7, 8, 2, 8, 3, 3, 8, 9, 3, 9, 4, 4, 9, 10, 4, 10, 5, 5, 10, 11, 5, 11, 6, 6, 11, 12};
	}

	{
		// XkHexagon Edge ->|-----------|------------|-------------|----------> XkHexagon Center
		//		    BaseOuterGap EdgeOuterGap EdgeInnerGap BaseInnerGap
		float EdgeHeight = Height + 1; // Fix Z-fighting
		TopBoundary.Empty();
		TopBoundary.Add(T0((Radius - EdgeInnerGap), 0.0, EdgeHeight));
		TopBoundary.Add(T0((Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundary.Add(T0(-(Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundary.Add(T0(-(Radius - EdgeInnerGap), 0.0, EdgeHeight));
		TopBoundary.Add(T0(-(Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundary.Add(T0((Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		BtmBoundary.Empty();
		BtmBoundary.Add(T0(Radius - EdgeOuterGap, 0.0, EdgeHeight));
		BtmBoundary.Add(T0((Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundary.Add(T0(-(Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundary.Add(T0(-(Radius - EdgeOuterGap), 0.0, EdgeHeight));
		BtmBoundary.Add(T0(-(Radius - EdgeOuterGap) * XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundary.Add(T0((Radius - EdgeOuterGap) * XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));

		for (int32 i = 0; i < TopBoundary.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % TopBoundary.Num();
			int32 IndexTopB = (i + 1 + 1) % TopBoundary.Num();
			int32 IndexBtmA = (i + 1) % TopBoundary.Num();
			int32 IndexBtmB = (i + 1 + 1) % TopBoundary.Num();

			T0 VertTopA = TopBoundary[IndexTopA];
			T0 VertTopB = TopBoundary[IndexTopB];
			T0 VertBtmA = BtmBoundary[IndexBtmA];
			VertBtmA.Z = VertTopA.Z;
			T0 VertBtmB = BtmBoundary[IndexBtmB];
			VertBtmB.Z = VertTopB.Z;

			int32 CurrIndex = OutEdgeVertices.Num();
			OutEdgeVertices.Add(VertTopA);
			OutEdgeVertices.Add(VertTopB);
			OutEdgeVertices.Add(VertBtmA);
			OutEdgeVertices.Add(VertBtmB);

			OutEdgeIndices.Add(CurrIndex);
			OutEdgeIndices.Add(CurrIndex + 2);
			OutEdgeIndices.Add(CurrIndex + 1);
			OutEdgeIndices.Add(CurrIndex + 1);
			OutEdgeIndices.Add(CurrIndex + 2);
			OutEdgeIndices.Add(CurrIndex + 3);
		}
	}
}

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

FORCEINLINE static float RandRangeFloatSin(int32 seed)
{
	// use sin function to generate a pseudo-random float value between 0 and 1
	// magic numbers: 12.9898 and 43758.5453
	float value = FMath::Sin(seed * 12.9898f) * 43758.5453f;
	value = value - FMath::FloorToFloat(value); // 取小数部分
	return value; // 结果在 0~1 之间	
};

FORCEINLINE static bool RandRangeBoolSin(int32 seed)
{
	return RandRangeFloatSin(seed) > 0.5f;
};

FORCEINLINE static int32 RandRangeIntSin(int32 seed)
{
	return static_cast<int32>(RandRangeFloatSin(seed) * INT32_MAX);
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
	return HexagonNodeIsValidLowLevel(Node) && Node->Type != EXkHexagonType::Unavailable;
}

FORCEINLINE static bool HexagonNodeHasAnyFlags(const FXkHexagonNode* Node, const EXkHexagonType InType)
{
	return Node && EnumHasAnyFlags(Node->Type, InType);
}

FORCEINLINE static int32 HexagonNodeRandomSeed(FXkHexagonNode* Node)
{
	check(Node);
	FIntVector SeedInt = Node->Coord;
	uint32 Hash = GetTypeHash(SeedInt);
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
	return (uint8)(Node->Weights.W * 255.0f);
}

FORCEINLINE static void HexagonNodeSetSplatID(FXkHexagonNode* Node, const uint8 SplatID)
{
	check(Node);
	Node->Weights.W = (float)SplatID / 255.0f;
}