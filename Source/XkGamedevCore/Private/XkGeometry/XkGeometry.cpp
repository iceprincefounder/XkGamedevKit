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


FXkGeomBounds::FXkGeomBounds(const UObject* InitObject)
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


FDynamicMesh3 FXkGeomBounds::ToDynamicMesh(const bool bLocalSpace) const
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


void FXkGeomBounds::ExpandBy(const FVector& Expand)
{
	FVector Scale = Transform.GetScale3D();
	FVector ScaledExpand = Expand / Scale;
	LocalBox = LocalBox.ExpandBy(ScaledExpand);
}


bool FXkGeomBounds::Intersect(const UWorld* World, const ECollisionChannel Channel) const
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


bool FXkGeomBounds::InsideOrOn(const FXkGeomBounds& GeomBounds) const
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


bool FXkGeomBounds::InsideOrOn(const FVector& Point) const
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


void FXkGeomBounds::DrawDebugRect(const UWorld* InWorld, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	FVector Origin = GetCenter();
	FVector Extent = GetExtent();
	::DrawDebugLine(InWorld, Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z), Origin + FVector(Extent.X, -Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(Extent.X, -Extent.Y, -Extent.Z), Origin + FVector(Extent.X, Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(Extent.X, Extent.Y, -Extent.Z), Origin + FVector(-Extent.X, Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(-Extent.X, Extent.Y, -Extent.Z), Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


void FXkGeomBounds::DrawDebugBox(const UWorld* InWorld,
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


void FXkGeomBounds::DrawDebugPoints(const UWorld* InWorld,
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


void FXkGeomBounds::DrawDebugCenter(const UWorld* InWorld, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	FVector Origin = GetCenter();
	::DrawDebugLine(InWorld, Origin + FVector(-5.0f, 0.0f, 0.0f), Origin + FVector(5.0f, 0.0f, 0.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(0.0f, -5.0f, 0.0f), Origin + FVector(0.0f, 5.0f, 0.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	::DrawDebugLine(InWorld, Origin + FVector(0.0f, 0.0f, -5.0f), Origin + FVector(0.0f, 0.0f, 5.0f), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}


bool FXkGeomBounds::CheckPointInsideBox(const FVector& Point, const TArray<FVector>& Vertices)
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


bool FXkGeomBounds::CheckPointInsideOrOnBox(const FVector& Point, const TArray<FVector>& Vertices)
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


bool FXkGeomBounds::CheckBoxIntersecting(const TArray<FVector>& LhsBoxCorners, const TArray<FVector> RhsBoxCorners)
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


bool FXkGeomBounds::CheckSphereIntersectsBoundingBox(const FVector& SphereCenter, const float SphereRadius, const FBox& Box)
{
	FVector ClosestPoint = Box.GetClosestPointTo(SphereCenter);
	float DistSqr = FVector::DistSquared(ClosestPoint, SphereCenter);
	return DistSqr <= SphereRadius * SphereRadius;
}


bool FXkGeomBounds::CheckSphereIntersectsTriangle(const FVector& SphereCenter, const float SphereRadius, const FVector& A, const FVector& B, const FVector& C)
{
	FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(SphereCenter, A, B, C);
	float DistSqr = FVector::DistSquared(SphereCenter, ClosestPoint);
	return DistSqr <= FMath::Square(SphereRadius);
}