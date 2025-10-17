// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkGeometry/XkGeometry.h"
#include "Components/SceneComponent.h"
#include "Components/ModelComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "Engine/StaticMeshActor.h"
#include "PhysicsEngine/BodySetup.h"
#include "EngineUtils.h"
#include "Engine/Brush.h"
#include "Engine/HitResult.h"
#include "MaterialDomain.h"

#include "Model.h"
#include "IndexTypes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMesh/MeshTransforms.h"
#include "Operations/MeshBoolean.h"
#include "StaticMeshAttributes.h"
#include "Remesher.h"
#include "QueueRemesher.h"
#include "MeshBoundaryLoops.h"
#include "MeshConstraints.h"
#include "MeshConstraintsUtil.h"
#include "DynamicMeshToMeshDescription.h"
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "Operations/MinimalHoleFiller.h"
#include "Polygon2.h"
#include "Curve/GeneralPolygon2.h"
#include "ConstrainedDelaunay2.h"

using namespace UE::Geometry;


bool FXkGeomEdge::IsPointOnEdge(const FVector& Point, const float Tolerance) const
{
	// Determines whether the given point lies on this segment within a specified tolerance. 
	// The function calculates the distance from A to B (segment length), from A to Point, and from Point to B. 
	// If the sum of AP and PB is nearly equal to AB (within Tolerance), and both AP and PB are not longer than AB, 
	// it means the point is on the segment (including endpoints), considering floating-point errors.
	float AB = (B - A).Size();
	float AP = (Point - A).Size();
	float PB = (B - Point).Size();
	return FMath::IsNearlyEqual(AB, AP + PB, Tolerance) && AP <= AB && PB <= AB;
}


bool FXkGeomEdge::IsPointOnEdge(const FVector2d& Point, const float Tolerance) const
{
	FVector2d A2d = FVector2d(A.X, A.Y);
	FVector2d B2d = FVector2d(B.X, B.Y);
	float AB = (B2d - A2d).Size();
	float AP = (Point - A2d).Size();
	float PB = (B2d - Point).Size();
	return FMath::IsNearlyEqual(AB, AP + PB, Tolerance) && AP <= AB && PB <= AB;
}


bool FXkGeomEdge::IsPointAtStartOrEnd(const FVector& Point, const float Tolerance) const
{
	return A.Equals(Point, Tolerance) || B.Equals(Point, Tolerance);
}


bool FXkGeomEdge::IsPointAtStartOrEnd(const FVector2d& Point, const float Tolerance) const
{
	return FVector2d(A.X, A.Y).Equals(Point, Tolerance) || FVector2d(B.X, B.Y).Equals(Point, Tolerance);
}


bool FXkGeomEdge::FindPointOnEdge(FVector& OutPoint, const FVector2d& InPoint2D, const float Tolerance) const
{
	if (IsPointOnEdge(InPoint2D, Tolerance))
	{
		// Project the 2D point onto the 3D edge
		FVector2d A2d = FVector2d(A.X, A.Y);
		FVector2d B2d = FVector2d(B.X, B.Y);
		FVector2d AB = B2d - A2d;
		FVector2d AP = InPoint2D - A2d;
		float AB_LengthSquared = AB.SizeSquared();
		if (AB_LengthSquared > KINDA_SMALL_NUMBER)
		{
			float T = FVector2d::DotProduct(AP, AB) / AB_LengthSquared;
			T = FMath::Clamp(T, 0.0f, 1.0f);
			OutPoint = A + T * (B - A);
			return true;
		}
	}
	return false;
}


bool FXkGeomEdge::FindIntersection(FVector2d& OutPoint, const FXkGeomEdge& OtherEdge, const float Tolerance) const
{
	const FVector2d VectorA = FVector2d(B.X - A.X, B.Y - A.Y);
	const FVector2d VectorB = FVector2d(OtherEdge.B.X - OtherEdge.A.X, OtherEdge.B.Y - OtherEdge.A.Y);

	const FVector2d::FReal S = (-VectorA.Y * (A.X - OtherEdge.A.X) + VectorA.X * (A.Y - OtherEdge.A.Y)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);
	const FVector2d::FReal T = (VectorB.X * (A.Y - OtherEdge.A.Y) - VectorB.Y * (A.X - OtherEdge.A.X)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);

	const bool bIntersects = (S >= -Tolerance) && (S <= 1.0f + Tolerance) && (T >= -Tolerance) && (T <= 1.0f + Tolerance);

	if (bIntersects)
	{
		OutPoint.X = A.X + (T * VectorA.X);
		OutPoint.Y = A.Y + (T * VectorA.Y);
		return true;
	}
	return false;
}


bool FXkGeomEdge::FindIntersection(FVector& OutPoint, const FXkGeomEdge& OtherEdge, const float Tolerance) const
{
	if (FVector2d Intersection2D; FindIntersection(Intersection2D, OtherEdge, Tolerance))
	{
		FVector PointOnA; FindPointOnEdge(PointOnA, Intersection2D, Tolerance);
		FVector PointOnB; OtherEdge.FindPointOnEdge(PointOnB, Intersection2D, Tolerance);
		float AverageZ = (PointOnA.Z + PointOnB.Z) * 0.5f;
		OutPoint = FVector(Intersection2D.X, Intersection2D.Y, AverageZ);
		return true;
	}
	return false;
}


TArray<FXkGeomEdge> FXkGeomEdge::Split(const FVector& Point, const float Tolerance) const
{
	TArray<FXkGeomEdge> Result;
	if (IsPointOnEdge(Point, Tolerance))
	{
		if (!A.Equals(Point, Tolerance))
		{
			Result.Add(FXkGeomEdge(A, Point));
		}
		if (!B.Equals(Point, Tolerance))
		{
			Result.Add(FXkGeomEdge(Point, B));
		}
	}
	else
	{
		Result.Add(*this);
	}
	return Result;
}


TArray<FXkGeomEdge> FXkGeomEdge::Split(const FVector2d& Point, const float Tolerance) const
{
	if (FVector NewPoint; FindPointOnEdge(NewPoint, Point, Tolerance))
	{
		return Split(NewPoint, Tolerance);
	}
	return TArray<FXkGeomEdge>{ *this };
}


bool FXkGeomEdge::CheckIsEdgeIntersected2D(const FVector2D& A1, const FVector2D& A2, const FVector2D& B1, const FVector2D& B2, const float Tolerance)
{
	const FVector2D VectorA = A2 - A1;
	const FVector2D VectorB = B2 - B1;

	const FVector::FReal S = (-VectorA.Y * (A1.X - B1.X) + VectorA.X * (A1.Y - B1.Y)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);
	const FVector::FReal T = (VectorB.X * (A1.Y - B1.Y) - VectorB.Y * (A1.X - B1.X)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);

	const bool bIntersects = (S >= -Tolerance) && (S <= 1.0f + Tolerance) && (T >= -Tolerance) && (T <= 1.0f + Tolerance);

	return bIntersects;
}


bool FXkGeomEdge::CheckIsPointInsideEdgeLoops2D(const FVector2D& P, const TArray<FVector2D>& Loops, const float Tolerance)
{
	int32 NumCrossings = 0;
	int32 NumVerts = Loops.Num();
	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector2D& A = Loops[i];
		const FVector2D& B = Loops[(i + 1) % NumVerts];

		// check if the ray from Vertex1 to +X direction crosses edge AB
		if (((A.Y > P.Y) != (B.Y > P.Y)) &&
			(P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y + Tolerance) + A.X))
		{
			NumCrossings++;
		}
	}
	return ((NumCrossings % 2) == 1);
}


bool FXkGeomEdge::CheckIsPointInsideEdgeLoops2D(const FVector& P, const TArray<FVector>& Loops, const float Tolerance)
{
	TArray<FVector2d> Loops2D;
	for (const FVector& Vertex : Loops)
	{
		Loops2D.Add(FVector2d(Vertex.X, Vertex.Y));
	}
	return CheckIsPointInsideEdgeLoops2D(FVector2d(P.X, P.Y), Loops2D, Tolerance);
}


void FXkGeomEdge::DrawDebugEdge(const UWorld* InWorld, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	::DrawDebugLine(InWorld, A, B, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


FXkGeomBound::FXkGeomBound(const UObject* InitObject)
{
	LocalBox = FBox(ForceInit);
	if (InitObject && InitObject->IsA<UClass>())
	{
		UObject* DefaultObject = Cast<UClass>(const_cast<UObject*>(InitObject))->GetDefaultObject();
	}
	else if (InitObject && InitObject->IsA<UStaticMesh>())
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(const_cast<UObject*>(InitObject));
		if (StaticMesh && StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
		{
			LocalBox = StaticMesh->GetRenderData()->Bounds.GetBox();
		}
	}
	else if (InitObject && InitObject->IsA<USkeletalMesh>())
	{
		// Not support skeletal mesh yet.
		USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(const_cast<UObject*>(InitObject));
		LocalBox = SkeletalMesh->GetImportedBounds().GetBox();
	}
	Transform = FTransform::Identity;
	Parent = MakeSoftObjectPtr(InitObject);
}


FDynamicMesh3 FXkGeomBound::ToDynamicMesh(const bool bLocalSpace) const
{
	TArray<FVector> Vertices = GetVertices();
	if (bLocalSpace)
	{
		FBox Box = GetLocalBox();
		FVector LocalVertices[8]; Box.GetVertices(LocalVertices);
		for (int32 i = 0; i < 8; ++i)
		{
			Vertices[i] = LocalVertices[i];
		}
	}
	FDynamicMesh3 DynamicMesh;
	DynamicMesh.EnableAttributes();
	for (int32 i = 0; i < 8; ++i)
	{
		DynamicMesh.AppendVertex(Vertices[i]);
	}

	for (int32 i = 0; i < 6; ++i)
	{
		DynamicMesh.AppendTriangle(Faces[i][0], Faces[i][1], Faces[i][2]);
		DynamicMesh.AppendTriangle(Faces[i][0], Faces[i][2], Faces[i][3]);
	}
	return DynamicMesh;
}


void FXkGeomBound::ExpandBy(const FVector& Expand)
{
	FVector Scale = Transform.GetScale3D();
	FVector ScaledExpand = Expand / Scale;
	LocalBox = LocalBox.ExpandBy(ScaledExpand);
}


bool FXkGeomBound::Intersect(const UWorld* World, const ECollisionChannel Channel) const
{
	if (!World || !IsValid(World))
	{
		return false;
	}
	FVector Origin = GetCenter();
	FVector Extent = GetExtent();
	FRotator Rotator = GetRotator();
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DefaultQueryParam), true);
	if (World->SweepTestByChannel(
		Origin, Origin,
		Rotator.Quaternion(),
		Channel,
		FCollisionShape::MakeBox(Extent),
		TraceParams))
	{
		return true;
	}
	return false;
}


bool FXkGeomBound::InsideOrOn(const FXkGeomBound& GeomBounds) const
{
	TArray<FVector> OwnerVertices = GetVertices();
	// Iterate through each vertex of the box
	TArray<FVector> BoxVertices = GeomBounds.GetVertices(-1.0f /*Shrink a little bit*/);
	for (const FVector& Vertex : BoxVertices)
	{
		// Check if the vertex is inside the box
		if (!CheckPointInsideOrOnBox(Vertex, OwnerVertices))
		{
			return false;
		}
	}

	// All points are within the bounds, return true
	return true;
}


bool FXkGeomBound::InsideOrOn(const FVector& Point) const
{
	FVector LocalPoint = Transform.InverseTransformPosition(Point);
	if (LocalBox.IsInsideOrOn(LocalPoint))
	{
		if (LocalPoint.Z >= LocalBox.Max.Z)
		{
			return false;
		}
		return true;
	}
	return false;
}


void FXkGeomBound::DrawDebugRect(const UWorld* InWorld, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	FVector Origin = GetCenter();
	FVector Extent = GetExtent();
	::DrawDebugLine(InWorld, Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z), Origin + FVector(Extent.X, -Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(Extent.X, -Extent.Y, -Extent.Z), Origin + FVector(Extent.X, Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(Extent.X, Extent.Y, -Extent.Z), Origin + FVector(-Extent.X, Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(-Extent.X, Extent.Y, -Extent.Z), Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


void FXkGeomBound::DrawDebugBox(const UWorld* InWorld,
		FColor const& Color, 
		bool bPersistentLines, 
		float LifeTime, 
		uint8 DepthPriority, 
		float Thickness) const
{
	FVector Origin = GetCenter();
	FVector Extent = GetExtent();
	FRotator Rotator = GetRotator();
	::DrawDebugBox(InWorld, Origin, Extent, Rotator.Quaternion(), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


void FXkGeomBound::DrawDebugPoints(const UWorld* InWorld,
	FColor const& Color,
	bool bPersistentLines,
	float LifeTime,
	uint8 DepthPriority,
	float Size) const
{
	TArray<FVector> LhsBoxCorners = GetVertices();
	
	for (FVector& Corner : LhsBoxCorners)
	{
		::DrawDebugPoint(InWorld, Corner, Size, Color, bPersistentLines, LifeTime, DepthPriority);
	}
}


void FXkGeomBound::DrawDebugCenter(const UWorld* InWorld, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	FVector Origin = GetCenter();
	::DrawDebugLine(InWorld, Origin + FVector(-5.0f, 0.0f, 0.0f), Origin + FVector(5.0f, 0.0f, 0.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(0.0f, -5.0f, 0.0f), Origin + FVector(0.0f, 5.0f, 0.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(0.0f, 0.0f, -5.0f), Origin + FVector(0.0f, 0.0f, 5.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


bool FXkGeomBound::CheckPointInsideBox(const FVector& Point, const TArray<FVector>& Vertices)
{
	for (int32 i = 0; i < 6; ++i)
	{
		// Fetch the vertices of the face
		const FVector& A = Vertices[Faces[i][0]];
		const FVector& B = Vertices[Faces[i][1]];
		const FVector& C = Vertices[Faces[i][2]];

		// Normal, right hand anti-clockwise
		FVector Normal = FVector::CrossProduct(C - A, B - A).GetSafeNormal();

		// Calculate the direction from the point to the plane
		float Direction = FVector::DotProduct(Point - A, Normal);

		// if the point is outside the box
		if (Direction > 0)
		{
			return false;
		}
	}

	// Check if the point is inside the box
	return true;
}


bool FXkGeomBound::CheckPointInsideOrOnBox(const FVector& Point, const TArray<FVector>& Vertices)
{
	for (int32 i = 0; i < 6; ++i)
	{
		// Fetch the vertices of the face
		const FVector& A = Vertices[Faces[i][0]];
		const FVector& B = Vertices[Faces[i][1]];
		const FVector& C = Vertices[Faces[i][2]];

		// Normal, right hand anti-clockwise
		FVector Normal = FVector::CrossProduct(C - A, B - A).GetSafeNormal();

		// Calculate the direction from the point to the plane
		float Direction = FVector::DotProduct(Point - A, Normal);

		// if the point is outside the box
		if (Direction > KINDA_SMALL_NUMBER)
		{
			return false;
		}
	}

	// Check if the point is inside the box
	return true;
}


bool FXkGeomBound::CheckBoxIntersecting(const TArray<FVector>& LhsBoxCorners, const TArray<FVector> RhsBoxCorners)
{
	TArray<FVector> Axes;
	Axes.Add(LhsBoxCorners[1] - LhsBoxCorners[0]); // X-axis of Box1
	Axes.Add(LhsBoxCorners[2] - LhsBoxCorners[0]); // Y-axis of Box1
	Axes.Add(LhsBoxCorners[4] - LhsBoxCorners[0]); // Z-axis of Box1
	Axes.Add(RhsBoxCorners[1] - RhsBoxCorners[0]); // X-axis of Box2
	Axes.Add(RhsBoxCorners[2] - RhsBoxCorners[0]); // Y-axis of Box2
	Axes.Add(RhsBoxCorners[4] - RhsBoxCorners[0]); // Z-axis of Box2

	// Add cross product axes
	for (int32 i = 0; i < 3; ++i)
	{
		for (int32 j = 0; j < 3; ++j)
		{
			Axes.Add(FVector::CrossProduct(Axes[i], Axes[j + 3]));
		}
	}

	// Check each axis
	for (const FVector& Axis : Axes)
	{
		if (!Axis.IsNearlyZero())
		{
			// Project the vertices of Box1 onto the axis
			float Min1 = FLT_MAX, Max1 = -FLT_MAX;
			for (const FVector& Corner : LhsBoxCorners)
			{
				float Projection = FVector::DotProduct(Corner, Axis);
				Min1 = FMath::Min(Min1, Projection);
				Max1 = FMath::Max(Max1, Projection);
			}

			// Project the vertices of Box2 onto the axis
			float Min2 = FLT_MAX, Max2 = -FLT_MAX;
			for (const FVector& Corner : RhsBoxCorners)
			{
				float Projection = FVector::DotProduct(Corner, Axis);
				Min2 = FMath::Min(Min2, Projection);
				Max2 = FMath::Max(Max2, Projection);
			}

			// Check if the projections overlap
			if (Max1 < Min2 || Max2 < Min1)
			{
				return false; // No overlap, boxes are not intersecting
			}
		}
	}

	return true; // Overlap on all axes, boxes are intersecting
}


bool FXkGeomBound::CheckSphereIntersectsBoundingBox(const FVector& SphereCenter, const float SphereRadius, const FBox& Box)
{
	FVector ClosestPoint = Box.GetClosestPointTo(SphereCenter);
	float DistSqr = FVector::DistSquared(ClosestPoint, SphereCenter);
	return DistSqr <= SphereRadius * SphereRadius;
}


bool FXkGeomBound::CheckSphereIntersectsTriangle(const FVector& SphereCenter, const float SphereRadius, const FVector& A, const FVector& B, const FVector& C)
{
	FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(SphereCenter, A, B, C);
	float DistSqr = FVector::DistSquared(SphereCenter, ClosestPoint);
	return DistSqr <= FMath::Square(SphereRadius);
}