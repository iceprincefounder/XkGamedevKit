// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkHexagon/XkHexagonActors.h"

#include "Engine/World.h"
#include "EngineUtils.h"


FXkHexagonAStarPathfinding::FXkHexagonAStarPathfinding()
{
}

FXkHexagonAStarPathfinding::~FXkHexagonAStarPathfinding()
{
	OpenList.Empty();
	ClosedList.Empty();
	HexagonalWorldTable = nullptr;
}


void FXkHexagonAStarPathfinding::Init(FXkHexagonalWorldNodeTable* InNodeTable)
{
	OpenList.Empty();
	ClosedList.Empty();
	HexagonalWorldTable = InNodeTable;
}


void FXkHexagonAStarPathfinding::Reinit()
{
	OpenList.Empty();
	ClosedList.Empty();
	BlockList.Empty();
}


void FXkHexagonAStarPathfinding::Blocking(const TArray<FIntVector>& Input)
{
	BlockList = Input;
}


bool FXkHexagonAStarPathfinding::Pathfinding(const FIntVector& StartingPoint, const FIntVector& TargetPoint, int32 MaxStep)
{
	TheStartPoint = StartingPoint;
	TheTargetPoint = TargetPoint;
	TMap<FIntVector, FXkHexagonNode>& NodeMap = HexagonalWorldTable->Nodes;

	FIntVector ConsideredPoint = StartingPoint;
	OpenList.Add(StartingPoint);
	ClosedList.Add(StartingPoint);
	NodeMap[StartingPoint].Cost = CalcPathCostValue(StartingPoint, StartingPoint, TargetPoint);
	int32 StepIndex = 0;
	while (StepIndex < MaxStep && !ClosedList.Contains(TargetPoint))
	{
		/////////////////////////////////////
		// Add all near point into open list
		TArray<FIntVector> NearPoints;
		for (const FIntVector& NearPoint : CalcHexagonNeighboringCoord(ConsideredPoint))
		{
			if (ClosedList.Contains(NearPoint))
			{
				continue;
			}
			if (NodeMap.Contains(NearPoint))
			{
				// Make sure near point not in BlockList which XkHexagon might be occupied by a character
				if (!BlockList.Contains(NearPoint))
				{
					FXkHexagonNode* HexagonNode = NodeMap.Find(NearPoint);
					if (HexagonNode && !EnumHasAnyFlags(HexagonNode->Type, EXkHexagonType::Unavailable))
					{
						NearPoints.Add(NearPoint);
						if (!OpenList.Contains(NearPoint))
						{
							OpenList.Add(NearPoint);
							HexagonNode->Cost = CalcPathCostValue(StartingPoint, NearPoint, TargetPoint);
						}
					}
				}
			}
		}
		///////////////////////////////////////////////////////////
		// Find all potential ConsideredPoint and minimal F nearby
		TArray<FIntVector> ConsideredPoints;
		int32 MinimalF = 9999;
		if (NearPoints.Num() > 0)
		{
			for (const FIntVector& NearPoint : NearPoints)
			{
				FXkPathCostValue Cost = NodeMap[NearPoint].Cost;
				MinimalF = FMath::Min(MinimalF, Cost.F);
				ConsideredPoints.Add(NearPoint);
			}
		}
		else
		{
			for (const FIntVector& NextPoint : OpenList)
			{
				if (!ClosedList.Contains(NextPoint))
				{
					FXkPathCostValue Cost = NodeMap[NextPoint].Cost;
					MinimalF = FMath::Min(MinimalF, Cost.F);
					ConsideredPoints.Add(NextPoint);
				}
			}
		}
		//////////////////////////////////////////////////
		// If there are multiple points have same MinimalF,
		// add those points into a list and compare G.
		TMap<FIntVector, FXkPathCostValue> ConsideredPointsResort;
		for (const FIntVector& CurrentPoint : ConsideredPoints)
		{
			if (NodeMap[CurrentPoint].Cost.F == MinimalF)
			{
				ConsideredPointsResort.Add(CurrentPoint, NodeMap[CurrentPoint].Cost);
			}
		}
		/////////////////////////////////////////////////////
		// Sort by the greater G if those points have same F.
		ConsideredPointsResort.ValueSort([](FXkPathCostValue A, FXkPathCostValue B)->bool { return A.G > B.G; });
		TArray<FIntVector> ResortConsideredPointsByG;
		ConsideredPointsResort.GenerateKeyArray(ResortConsideredPointsByG);
		if (ResortConsideredPointsByG.Num() > 0)
		{
			ConsideredPoint = ResortConsideredPointsByG[0];
			ClosedList.Add(ConsideredPoint);
		}
		else
		{
			break;
		}
		StepIndex++;
	}
	return ClosedList.Contains(TargetPoint);
}


TArray<FIntVector> FXkHexagonAStarPathfinding::Backtracking(const int32 MaxStep) const
{
	TMap<FIntVector, FXkHexagonNode>& NodeMap = HexagonalWorldTable->Nodes;

	TArray<FIntVector> BackTrackingList;
	BackTrackingList.Add(TheTargetPoint);
	TArray<FIntVector> ConsideredPointsList;
	FIntVector ConsideredPoint = TheTargetPoint;
	int32 StepIndex = 0;
	while (StepIndex < MaxStep && !BackTrackingList.Contains(TheStartPoint))
	{
		// Add all near point into near point list
		TArray<FIntVector> NearPoints;
		while (NearPoints.IsEmpty() && !BackTrackingList.IsEmpty())
		{
			for (const FIntVector& NearPoint : CalcHexagonNeighboringCoord(ConsideredPoint))
			{
				// Skip the point not in ClosedList
				if (!ClosedList.Contains(NearPoint))
				{
					continue;
				}
				// Add point not in ConsideredPoints, make sure the tacking does not go backward
				if (!ConsideredPointsList.Contains(NearPoint))
				{
					NearPoints.Add(NearPoint);
				}
			}
			// if couldn't find available neighborhood,
			// go back to the last ConsideredPoint and check neighborhood again,
			// ConsideredPointsList would make it switch to the new road
			if (NearPoints.IsEmpty())
			{
				// remove last add
				BackTrackingList.Remove(ConsideredPoint);
				// pop the one on the top and never shrink the list
				ConsideredPoint = BackTrackingList.Pop(false);
			}
		}

		for (const FIntVector& NearPoint : NearPoints)
		{
			// Init first TheTargetPoint
			if (BackTrackingList.Contains(ConsideredPoint))
			{
				ConsideredPoint = NearPoint;
			}
			// Compare G between ConsideredPoint and NearPoint
			if (NodeMap[ConsideredPoint].Cost.G > NodeMap[NearPoint].Cost.G)
			{
				ConsideredPoint = NearPoint;
			}
		}

		if (!BackTrackingList.Contains(ConsideredPoint))
		{
			ConsideredPointsList.Add(ConsideredPoint);
			BackTrackingList.Add(ConsideredPoint);
		}
		StepIndex++;
	}

	return BackTrackingList;
}


TArray<FIntVector> FXkHexagonAStarPathfinding::SearchArea() const
{
	return ClosedList;
}


FXkPathCostValue FXkHexagonAStarPathfinding::CalcPathCostValue(const FIntVector& StartingPoint, const FIntVector& ConsideredPoint, const FIntVector& TargetPoint, int32 Offset)
{
	int32 G = CalcManhattanDistance(StartingPoint, ConsideredPoint);
	int32 H = CalcManhattanDistance(ConsideredPoint, TargetPoint);
	int32 R = Offset;
	return FXkPathCostValue(G, H, R);
}


int32 FXkHexagonAStarPathfinding::CalcManhattanDistance(const FIntVector& PointA, const FIntVector& PointB)
{
	return FMath::Max3(abs(PointA.X - PointB.X), abs(PointA.Y - PointB.Y), abs(PointA.Z - PointB.Z));
}


FIntVector FXkHexagonAStarPathfinding::CalcHexagonCoord(const float PositionX, const float PositionY, const float HexagonRadius)
{
	FVector Position = FVector(PositionX, PositionY, 0.0);

	double HexagonUnitLength = HexagonRadius + HexagonRadius * XkCos60;

	// Caclulate projected unit length.
	float X = FVector::DotProduct(Position, HexagonNodeXAxis()) / HexagonUnitLength;
	float Y = FVector::DotProduct(Position, HexagonNodeYAxis()) / HexagonUnitLength;
	float Z = FVector::DotProduct(Position, HexagonNodeZAxis()) / HexagonUnitLength;

    // Ensure X + Y + Z = 0
    double Sum = X + Y + Z;
    if (FMath::Abs(X) >= FMath::Abs(Y) && FMath::Abs(X) >= FMath::Abs(Z))
    {
        X -= Sum;
    }
    else if (FMath::Abs(Y) >= FMath::Abs(X) && FMath::Abs(Y) >= FMath::Abs(Z))
    {
        Y -= Sum;
    }
    else
    {
        Z -= Sum;
    }

    // Round to the nearest integer coordinates
    int32 RoundedX = FMath::RoundToInt(X);
    int32 RoundedY = FMath::RoundToInt(Y);
    int32 RoundedZ = FMath::RoundToInt(Z);
	
    // Ensure the sum of coordinates is 0
    int32 RoundedSum = RoundedX + RoundedY + RoundedZ;
    if (RoundedSum != 0)
    {
        if (FMath::Abs(X - RoundedX) >= FMath::Abs(Y - RoundedY) && FMath::Abs(X - RoundedX) >= FMath::Abs(Z - RoundedZ))
        {
            RoundedX -= RoundedSum;
        }
        else if (FMath::Abs(Y - RoundedY) >= FMath::Abs(X - RoundedX) && FMath::Abs(Y - RoundedY) >= FMath::Abs(Z - RoundedZ))
        {
            RoundedY -= RoundedSum;
        }
        else
        {
            RoundedZ -= RoundedSum;
        }
    }

	// Debugging
	// GEngine->AddOnScreenDebugMessage(0, 3.0, FColor::Red, *FString::Printf(TEXT("%f | %f"), PositionX, PositionY));
	// GEngine->AddOnScreenDebugMessage(1, 3.0, FColor::Red, *FString::Printf(TEXT("%f | %f | %f"), X, Y, Z));
	// GEngine->AddOnScreenDebugMessage(2, 3.0, FColor::Red, *FString::Printf(TEXT("%i | %i | %i"), RoundedX, RoundedY, RoundedZ));

	// Return hexagon coordinates
	return FIntVector(RoundedX, RoundedY, RoundedZ);
}


FVector2D FXkHexagonAStarPathfinding::CalcHexagonPosition(const int32 IndexX, const int32 IndexY, const float Distance)
{
	double Pos_X = Distance * 1.5 * IndexX;
	double Pos_Y = XkCos30 * Distance * 2.0 * IndexY;
	if (IndexX % 2 != 0)
	{
		Pos_Y += (XkCos30 * Distance);
	}
	return FVector2D(Pos_X, Pos_Y);
}


TArray<FIntVector> FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(const FIntVector& InputCoord)
{
	//	x
	//	| Neighboring Coords
	//	|  5/ \0
	//	| 4|   |1
	//	|  3\ /2
	//	| Clockwise (C.W.)
	//	---------y

	const TArray<FIntVector> HexagonNearVectors = {
		FIntVector(1, -1, 0),
		FIntVector(0, -1, 1),
		FIntVector(-1, 0, 1),
		FIntVector(-1, 1, 0),
		FIntVector(0, 1, -1),
		FIntVector(1, 0, -1)
	};
	TArray<FIntVector> Ret;
	for (const FIntVector NearVector : HexagonNearVectors)
	{
		Ret.AddUnique(InputCoord + NearVector);
	}
	return Ret;
}


TArray<FIntVector> FXkHexagonAStarPathfinding::CalcHexagonSurroundingCoord(const TArray<FIntVector>& InputCoords)
{
	TArray<FIntVector> AllNeighbors;
	for (const FIntVector& CurrCorrds : InputCoords)
	{
		TArray<FIntVector> Neighbors = CalcHexagonNeighboringCoord(CurrCorrds);
		for (const FIntVector& Neighbor : Neighbors)
		{
			AllNeighbors.AddUnique(Neighbor);
		}
	}

	TArray<FIntVector> Results;
	for (const FIntVector& Neighbor : AllNeighbors)
	{
		if (!InputCoords.Contains(Neighbor))
		{
			Results.Add(Neighbor);
		}
	}
	return Results;
}

template<typename T0, typename T1>
void BuildHexagon(TArray<T0>& OutBaseVertices, TArray<T1>& OutBaseIndices, TArray<T0>& OutEdgeVertices, TArray<T1>& OutEdgeIndices,
	float Radius, float Height, float BaseInnerGap, float BaseOuterGap, float EdgeInnerGap, float EdgeOuterGap)
{
	TArray<T0> TopBoundaryVertices;
	TArray<T0> BtmBoundaryVertices;
	{
		//	x
		//	|   1
		//	| 2/ \6
		//	| | 0 |
		//	| 3\ /5
		//	|   4
		//	---------y

		OutBaseVertices.Add(T0(0.0, 0.0, Height));
		TopBoundaryVertices.Empty();
		TopBoundaryVertices.Add(T0((Radius - BaseInnerGap), 0.0, Height));
		TopBoundaryVertices.Add(T0((Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundaryVertices.Add(T0(-(Radius - BaseInnerGap) * XkSin30, -XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundaryVertices.Add(T0(-(Radius - BaseInnerGap), 0.0, Height));
		TopBoundaryVertices.Add(T0(-(Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		TopBoundaryVertices.Add(T0((Radius - BaseInnerGap) * XkSin30, XkCos30 * (Radius - BaseInnerGap), Height));
		for (const T0& Vert : TopBoundaryVertices)
		{
			OutBaseVertices.Add(Vert);
		}
		OutBaseIndices = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0 , 5, 6, 0, 6, 1 };
		BtmBoundaryVertices.Empty();
		BtmBoundaryVertices.Add(T0((Radius - BaseOuterGap), 0.0, 0.0));
		BtmBoundaryVertices.Add(T0((Radius - BaseOuterGap) * XkSin30, -XkCos30 * Radius, 0.0));
		BtmBoundaryVertices.Add(T0(-(Radius - BaseOuterGap) * XkSin30, -XkCos30 * Radius, 0.0));
		BtmBoundaryVertices.Add(T0(-(Radius - BaseOuterGap), 0.0, 0.0));
		BtmBoundaryVertices.Add(T0(-(Radius - BaseOuterGap) * XkSin30, XkCos30 * Radius, 0.0));
		BtmBoundaryVertices.Add(T0((Radius - BaseOuterGap) * XkSin30, XkCos30 * Radius, 0.0));
		for (int32 i = 0; i < TopBoundaryVertices.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % TopBoundaryVertices.Num();
			int32 IndexTopB = (i + 1 + 1) % TopBoundaryVertices.Num();
			int32 IndexBtmA = (i + 1) % TopBoundaryVertices.Num();
			int32 IndexBtmB = (i + 1 + 1) % TopBoundaryVertices.Num();

			T0 VertTopA = TopBoundaryVertices[IndexTopA];
			T0 VertTopB = TopBoundaryVertices[IndexTopB];
			T0 VertBtmA = BtmBoundaryVertices[IndexBtmA];
			T0 VertBtmB = BtmBoundaryVertices[IndexBtmB];

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
		TopBoundaryVertices.Empty();
		TopBoundaryVertices.Add(T0((Radius - EdgeInnerGap), 0.0, EdgeHeight));
		TopBoundaryVertices.Add(T0((Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundaryVertices.Add(T0(-(Radius - EdgeInnerGap) * XkSin30, -XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundaryVertices.Add(T0(-(Radius - EdgeInnerGap), 0.0, EdgeHeight));
		TopBoundaryVertices.Add(T0(-(Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		TopBoundaryVertices.Add(T0((Radius - EdgeInnerGap) * XkSin30, XkCos30 * (Radius - EdgeInnerGap), EdgeHeight));
		BtmBoundaryVertices.Empty();
		BtmBoundaryVertices.Add(T0(Radius - EdgeOuterGap, 0.0, EdgeHeight));
		BtmBoundaryVertices.Add(T0((Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundaryVertices.Add(T0(-(Radius - EdgeOuterGap) * XkSin30, -XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundaryVertices.Add(T0(-(Radius - EdgeOuterGap), 0.0, EdgeHeight));
		BtmBoundaryVertices.Add(T0(-(Radius - EdgeOuterGap) * XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));
		BtmBoundaryVertices.Add(T0((Radius - EdgeOuterGap) * XkSin30, XkCos30 * (Radius - EdgeOuterGap), EdgeHeight));

		for (int32 i = 0; i < TopBoundaryVertices.Num(); i++)
		{
			int32 IndexTopA = (i + 1) % TopBoundaryVertices.Num();
			int32 IndexTopB = (i + 1 + 1) % TopBoundaryVertices.Num();
			int32 IndexBtmA = (i + 1) % TopBoundaryVertices.Num();
			int32 IndexBtmB = (i + 1 + 1) % TopBoundaryVertices.Num();

			T0 VertTopA = TopBoundaryVertices[IndexTopA];
			T0 VertTopB = TopBoundaryVertices[IndexTopB];
			T0 VertBtmA = BtmBoundaryVertices[IndexBtmA];
			VertBtmA.Z = VertTopA.Z;
			T0 VertBtmB = BtmBoundaryVertices[IndexBtmB];
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