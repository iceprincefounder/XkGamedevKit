// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Operations/MeshBoolean.h"
#include "XkGeometry.generated.h"

#ifndef UE_CM_TO_M
#define UE_CM_TO_M 0.01f
#endif

using namespace UE::Geometry;

/** XkGeomBounds
 * AABB local box with transform and parent(reference to actor or assets).
*/
USTRUCT(BlueprintType)
struct XKGAMEDEVCORE_API FXkGeomBounds
{
	GENERATED_BODY()

	FXkGeomBounds()
		: LocalBox(FBox(ForceInit))
		, Transform(FTransform::Identity)
		, Parent(nullptr)
	{}

	FXkGeomBounds(const UObject* InitObject);

	FXkGeomBounds(const UObject* InitObject, const FTransform& InTransform)
		: FXkGeomBounds(InitObject)
	{
		Transform = InTransform;
	}

	void ExpandBy(const FVector& Expand);

	bool Intersect(const FXkGeomBounds& GeomBounds) const { return CheckBoxIntersecting(GetVertices(), GeomBounds.GetVertices()); };

	bool Intersect(const UWorld* World, const ECollisionChannel Channel = ECC_WorldStatic) const;

	bool InsideOrOn(const FXkGeomBounds& GeomBounds) const;

	bool InsideOrOn(const FVector& Point) const;

	bool Overlap(const FXkGeomBounds& GeomBounds) const { return Intersect(GeomBounds) || InsideOrOn(GeomBounds); };

	FDynamicMesh3 ToDynamicMesh(const bool bLocalSpace = true) const;

	inline TArray<FVector> GetVertices(const float Expand = 0.0f) const
	{
		TArray<FVector> Results;
		Results.Init(FVector::ZeroVector, 8);
		FVector Vertices[8];

		FBox Box = GetLocalBox().ExpandBy(Expand);
		Box.GetVertices(Vertices);
		for (int32 i = 0; i < 8; ++i)
		{
			Results[i] = (Transform.TransformPosition(Vertices[i]));
		}
		return Results;
	}

	inline FVector GetCenter() const
	{
		FBox WorldBox = LocalBox.TransformBy(Transform);
		return WorldBox.GetCenter();
	}

	inline FVector GetExtent() const
	{
		return LocalBox.GetExtent() * Transform.GetScale3D().GetAbs();
	}

	inline FVector GetMaxExtent() const
	{
		TArray<FVector> Vertices = GetVertices();
		FBox MaxBox(Vertices);
		return MaxBox.GetExtent();
	}

	inline FRotator GetRotator() const
	{
		return Transform.Rotator();
	}

	inline FBox GetLocalBox() const
	{
		return LocalBox;
	}

	inline uint32 GetHash() const
	{
		uint32 Hash = HashCombine(GetTypeHash(LocalBox.Min), GetTypeHash(LocalBox.Max));
		Hash = HashCombine(Hash, GetTypeHash(Transform));
		Hash = HashCombine(Hash, GetTypeHash(Parent));
		return Hash;
	}

	inline FTransform GetTransformNoScale() const
	{
		FTransform Result = Transform;
		Result.SetScale3D(FVector::OneVector);
		return Result;
	}

	inline FTransform GetTransform() const { return Transform; }

	inline TSoftObjectPtr<> GetParent() const { return Parent; }

	inline bool operator == (const FXkGeomBounds& Rhs) const
	{
		return LocalBox == Rhs.LocalBox && Transform.Equals(Rhs.Transform) && Parent == Rhs.Parent;
	}

public:
	void DrawDebugRect(const UWorld* InWorld, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = SDPG_World, float Thickness = 0.f) const;

	void DrawDebugBox(const UWorld* InWorld, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = SDPG_World, float Thickness = 0.f) const;

	void DrawDebugPoints(const UWorld* InWorld, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = SDPG_World, float Size = 0.f) const;

	void DrawDebugCenter(const UWorld* InWorld, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = SDPG_World, float Thickness = 0.f) const;
public:
	static bool CheckPointInsideBox(const FVector& Point, const TArray<FVector>& Vertices);

	static bool CheckPointInsideOrOnBox(const FVector& Point, const TArray<FVector>& Vertices);

	static bool CheckBoxIntersecting(const TArray<FVector>& LhsBoxCorners, const TArray<FVector> RhsBoxCorners);

	static bool CheckSphereIntersectsBoundingBox(const FVector& SphereCenter, const float SphereRadius, const FBox& Box);

	static bool CheckSphereIntersectsTriangle(const FVector& SphereCenter, const float SphereRadius, const FVector& A, const FVector& B, const FVector& C);
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeomBounds")
	FBox LocalBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeomBounds")
	FTransform Transform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeomBounds")
	TSoftObjectPtr<UObject> Parent;
private:
	///////////////////////////////////////////////////////////////////////////////
	// The vertices of the box are defined in the following order:
	// Vertices[0] = TVector<T>(Min);
	// Vertices[1] = TVector<T>(Min.X, Min.Y, Max.Z);
	// Vertices[2] = TVector<T>(Min.X, Max.Y, Min.Z);
	// Vertices[3] = TVector<T>(Max.X, Min.Y, Min.Z);
	// Vertices[4] = TVector<T>(Max.X, Max.Y, Min.Z);
	// Vertices[5] = TVector<T>(Max.X, Min.Y, Max.Z);
	// Vertices[6] = TVector<T>(Min.X, Max.Y, Max.Z);
	// Vertices[7] = TVector<T>(Max);
	////////////////////////////////////////////////////////////////
	// Right-handed coordinate system, anti-clockwise winding order
	// x
	// |   2 4   5 7
	// |   0 3   1 6
	// | BOTTOM  TOP
	// |--------------y
	// z
	// |  1 6   4 7
	// |  0 2   3 5
	// | BACK  FRONT
	// |/--------------y
	// z
	// |  3 5   6 7
	// |  0 1   2 4
	// | Left  Right
	// |/--------------x
	static constexpr int32 Faces[6][4] = {
		{0, 2, 6, 1}, // 0.Back
		{3, 5, 7, 4}, // 1.Front
		{0, 3, 4, 2}, // 2.Bottom
		{1, 6, 7, 5}, // 3.Top
		{2, 4, 7, 6}, // 4.Right
		{0, 1, 5, 3}  // 5.Left
	};
};


inline uint32 GetTypeHash(const FXkGeomBounds& GeomBounds)
{
	return GeomBounds.GetHash();
}