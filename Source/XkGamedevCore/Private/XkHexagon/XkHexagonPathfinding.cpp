// Fill out your copyright notice in the Description page of Project Settings.


#include "XkHexagon/XkHexagonPathfinding.h"
#include "XkHexagon/XkHexagonActors.h"

#include "Engine/World.h"
#include "EngineUtils.h"

static const TArray<FIntVector> GXkHexagonNearVectors = {
	FIntVector(1, 1, 0),
	FIntVector(0, 1, 1),
	FIntVector(-1, 0, 1),
	FIntVector(-1, -1, 0),
	FIntVector(0, -1, -1),
	FIntVector(1, 0, -1)
};


FXkHexagonNode::FXkHexagonNode(AXkHexagonActor* InActor) 
{
	Actor = MakeWeakObjectPtr(InActor);
}

FXkHexagonNode::~FXkHexagonNode()
{
	Actor.Reset();
}


FXkHexagonNode& FXkHexagonNode::operator=(const FXkHexagonNode& other)
{
	Actor = other.Actor;
	Cost = other.Cost;
	return *this;
}


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


TArray<FIntVector> FXkHexagonAStarPathfinding::Backtracking(int32 MaxStep) const
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


FIntVector FXkHexagonAStarPathfinding::CalcHexagonCoord(float PositionX, float PositionY, float XkHexagonRadius)
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


FVector2D FXkHexagonAStarPathfinding::CalcHexagonPosition(int32 IndexX, int32 IndexY, float Radius)
{
	float Pos_X = Radius * 1.5 * IndexX;
	float Pos_Y = XkCos30 * Radius * 2.0 * IndexY;
	if (IndexX % 2 != 0)
	{
		Pos_Y += (XkCos30 * Radius);
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
