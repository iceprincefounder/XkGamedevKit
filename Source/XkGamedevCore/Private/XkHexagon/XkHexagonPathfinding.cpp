// Copyright ©XUKAI. All Rights Reserved.


#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkHexagon/XkHexagonActors.h"

#include "Engine/World.h"
#include "EngineUtils.h"


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


void CullHexagonalWorld(TArray<FXkHexagonNode*> OutHexagonNodes, const TMap<FIntVector, FXkHexagonNode>& HexagonalWorldNodes, const FSceneView& View, const float Distance)
{
}

static const TArray<FIntVector> GXkHexagonNearVectors = {
	FIntVector(1, 1, 0),
	FIntVector(0, 1, 1),
	FIntVector(-1, 0, 1),
	FIntVector(-1, -1, 0),
	FIntVector(0, -1, -1),
	FIntVector(1, 0, -1)
};


FXkHexagonAStarPathfinding::FXkHexagonAStarPathfinding()
{
}

FXkHexagonAStarPathfinding::~FXkHexagonAStarPathfinding()
{
	OpenList.Empty();
	ClosedList.Empty();
	HexagonalWorldTable.Empty();
}

void FXkHexagonAStarPathfinding::Init(UWorld* InWorld)
{
	check(InWorld)
	OpenList.Empty();
	ClosedList.Empty();
	HexagonalWorldTable.Empty();
	for (TActorIterator<AXkHexagonActor> It(InWorld); It; ++It)
	{
		AXkHexagonActor* HexagonActor = (*It);
		HexagonalWorldTable.Add(HexagonActor->GetCoord(), FXkHexagonNode(HexagonActor));
	}
}


void FXkHexagonAStarPathfinding::Reinit()
{
	OpenList.Empty();
	ClosedList.Empty();
}


void FXkHexagonAStarPathfinding::Blocking(const TArray<FIntVector>& Input)
{
	BlockList = Input;
}


bool FXkHexagonAStarPathfinding::Pathfinding(const FIntVector& StartingPoint, const FIntVector& TargetPoint, int32 MaxStep)
{
	TheStartPoint = StartingPoint;
	TheTargetPoint = TargetPoint;

	FIntVector ConsideredPoint = StartingPoint;
	OpenList.Add(StartingPoint);
	ClosedList.Add(StartingPoint);
	HexagonalWorldTable[StartingPoint].Cost = CalcPathCostValue(StartingPoint, StartingPoint, TargetPoint);
	int32 StepIndex = 0;
	while (StepIndex < MaxStep && !ClosedList.Contains(TargetPoint))
	{
		/////////////////////////////////////
		// Add all near point into open list
		TArray<FIntVector> NearPoints;
		for (const FIntVector& NearVector : GXkHexagonNearVectors)
		{
			FIntVector NearPoint = ConsideredPoint + NearVector;
			if (ClosedList.Contains(NearPoint))
			{
				continue;
			}
			if (HexagonalWorldTable.Contains(NearPoint))
			{
				// Make sure near point not in BlockList which XkHexagon might be occupied by a character
				if (!BlockList.Contains(NearPoint))
				{
					FXkHexagonNode* XkHexagonNode = HexagonalWorldTable.Find(NearPoint);
					if (XkHexagonNode && XkHexagonNode->Actor.IsValid())
					{
						TWeakObjectPtr<AXkHexagonActor> Actor = XkHexagonNode->Actor;
						if (Actor.IsValid() && Actor->IsAccessible())
						{
							NearPoints.Add(NearPoint);
							if (!OpenList.Contains(NearPoint))
							{
								OpenList.Add(NearPoint);
								int32 Offset = Actor->CalcCostOffset();
								XkHexagonNode->Cost = CalcPathCostValue(StartingPoint, NearPoint, TargetPoint, Offset);
							}
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
				FXkPathCostValue Cost = HexagonalWorldTable[NearPoint].Cost;
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
					FXkPathCostValue Cost = HexagonalWorldTable[NextPoint].Cost;
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
			if (HexagonalWorldTable[CurrentPoint].Cost.F == MinimalF)
			{
				ConsideredPointsResort.Add(CurrentPoint, HexagonalWorldTable[CurrentPoint].Cost);
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
			for (const FIntVector& NearVector : GXkHexagonNearVectors)
			{
				FIntVector NearPoint = ConsideredPoint + NearVector;
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
			if (HexagonalWorldTable[ConsideredPoint].Cost.G > HexagonalWorldTable[NearPoint].Cost.G)
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


TArray<class AXkHexagonActor*> FXkHexagonAStarPathfinding::FindHexagonActors(const TArray<FIntVector>& Inputs) const
{
	TArray<class AXkHexagonActor*> Ret;
	for (const FIntVector& CurrentPoint : Inputs)
	{
		if (AXkHexagonActor* HexagonActor = FindHexagonActor(CurrentPoint))
		{
			Ret.Insert(HexagonActor, 0);
		}
	}
	return Ret;
}


AXkHexagonActor* FXkHexagonAStarPathfinding::FindHexagonActor(const FIntVector& Input) const
{
	if (HexagonalWorldTable.Contains(Input))
	{
		return HexagonalWorldTable[Input].Actor.Get();
	}
	return nullptr;
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


FIntVector FXkHexagonAStarPathfinding::CalcHexagonCoord(const float PositionX, const float PositionY, const float XkHexagonRadius)
{
	FIntVector XkHexagonCoord = FIntVector(0, 0, 0);
	FVector XkHexagonVec = FVector(PositionX, PositionY, 0.0);
	float Length = XkHexagonVec.Size();
	XkHexagonVec.Normalize();
	FVector YAxis = FRotator(0, 60, 0).RotateVector(FVector(1, 0, 0));
	YAxis.Normalize();
	FVector ZAxis = FRotator(0, 120, 0).RotateVector(FVector(1, 0, 0));
	ZAxis.Normalize();
	float YProjectionLength = Length * FVector::DotProduct(XkHexagonVec, YAxis);
	float ZProjectionLength = Length * FVector::DotProduct(XkHexagonVec, ZAxis);
	float X = PositionX / (XkHexagonRadius * 1.5);
	float Y = YProjectionLength / (XkHexagonRadius * 1.5);
	float Z = ZProjectionLength / (XkHexagonRadius * 1.5);
	XkHexagonCoord.X = round(X);
	XkHexagonCoord.Y = round(Y);
	XkHexagonCoord.Z = round(Z);
	return XkHexagonCoord;
}


FVector2D FXkHexagonAStarPathfinding::CalcHexagonPosition(const int32 IndexX, const int32 IndexY, const float Distance)
{
	float Pos_X = Distance * 1.5 * IndexX;
	float Pos_Y = XkCos30 * Distance * 2.0 * IndexY;
	if (IndexX % 2 != 0)
	{
		Pos_Y += (XkCos30 * Distance);
	}
	return FVector2D(Pos_X, Pos_Y);
}


TArray<FIntVector> FXkHexagonAStarPathfinding::CalcHexagonNeighboringCoord(const FIntVector& InputCoord)
{
	TArray<FIntVector> Ret;
	for (const FIntVector NearVector : GXkHexagonNearVectors)
	{
		Ret.Add(InputCoord + NearVector);
	}
	return Ret;
}