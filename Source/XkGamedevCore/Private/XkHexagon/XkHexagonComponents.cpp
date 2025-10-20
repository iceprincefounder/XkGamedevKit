// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkHexagon/XkHexagonComponents.h"
#include "XkHexagon/XkHexagonSceneProxy.h"
#include "XkGeometry/XkGeometry.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/Engine.h"
#include "Components/ArrowComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/CollisionProfile.h"
#include "SceneInterface.h"
#include "SceneManagement.h"

#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "Model.h"
#include "IndexTypes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshTransforms.h"
#include "DynamicMeshBuilder.h"
#include "DynamicMeshToMeshDescription.h"
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "Operations/MeshBoolean.h"
#include "Operations/MinimalHoleFiller.h"
#include "Remesher.h"
#include "QueueRemesher.h"
#include "MeshBoundaryLoops.h"
#include "MeshConstraints.h"
#include "MeshConstraintsUtil.h"
#include "Polygon2.h"
#include "Curve/GeneralPolygon2.h"
#include "ConstrainedDelaunay2.h"
#include "Generators/MinimalBoxMeshGenerator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(XkHexagonComponents)

#define DEFAULT_SCREEN_SIZE	(0.0025f)
#define ARROW_SCALE			(80.0f)
#define ARROW_RADIUS_FACTOR	(0.03f)
#define ARROW_HEAD_FACTOR	(0.2f)
#define ARROW_HEAD_ANGLE	(20.f)


void BuildXkHexagonConeVerts(float Angle1, float Angle2, float Scale, float Length, float ZOffset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	TArray<FVector> ConeVerts;
	ConeVerts.AddUninitialized(NumSides);

	for (uint32 i = 0; i < NumSides; i++)
	{
		float Fraction = (float)i / (float)(NumSides);
		float Azi = 2.f * UE_PI * Fraction;
		ConeVerts[i] = (CalcConeVert(Angle1, Angle2, Azi) * Scale) + FVector(Length, 0, 0);
	}

	for (uint32 i = 0; i < NumSides; i++)
	{
		// Normal of the current face 
		FVector TriTangentZ = ConeVerts[(i + 1) % NumSides] ^ ConeVerts[i]; // aka triangle normal
		FVector TriTangentY = ConeVerts[i];
		FVector TriTangentX = TriTangentZ ^ TriTangentY;


		FDynamicMeshVertex V0, V1, V2;

		V0.Position = FVector3f(0) + FVector3f(Length, 0, 0);
		V0.TextureCoordinate[0].X = 0.0f;
		V0.TextureCoordinate[0].Y = (float)i / NumSides;
		V0.SetTangents((FVector3f)TriTangentX, (FVector3f)TriTangentY, (FVector3f)FVector(-1, 0, 0));
		V0.Position.Z += ZOffset;
		int32 I0 = OutVerts.Add(V0);

		V1.Position = (FVector3f)ConeVerts[i];
		V1.TextureCoordinate[0].X = 1.0f;
		V1.TextureCoordinate[0].Y = (float)i / NumSides;
		FVector TriTangentZPrev = ConeVerts[i] ^ ConeVerts[i == 0 ? NumSides - 1 : i - 1]; // Normal of the previous face connected to this face
		V1.SetTangents((FVector3f)TriTangentX, (FVector3f)TriTangentY, (FVector3f)(TriTangentZPrev + TriTangentZ).GetSafeNormal());
		V1.Position.Z += ZOffset;
		int32 I1 = OutVerts.Add(V1);

		V2.Position = (FVector3f)ConeVerts[(i + 1) % NumSides];
		V2.TextureCoordinate[0].X = 1.0f;
		V2.TextureCoordinate[0].Y = (float)((i + 1) % NumSides) / NumSides;
		FVector TriTangentZNext = ConeVerts[(i + 2) % NumSides] ^ ConeVerts[(i + 1) % NumSides]; // Normal of the next face connected to this face
		V2.SetTangents((FVector3f)TriTangentX, (FVector3f)TriTangentY, (FVector3f)(TriTangentZNext + TriTangentZ).GetSafeNormal());
		V2.Position.Z += ZOffset;
		int32 I2 = OutVerts.Add(V2);

		// Flip winding for negative scale
		if (Scale >= 0.f)
		{
			OutIndices.Add(I0);
			OutIndices.Add(I1);
			OutIndices.Add(I2);
		}
		else
		{
			OutIndices.Add(I0);
			OutIndices.Add(I2);
			OutIndices.Add(I1);
		}
	}
}


void BuildXkHexagonCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, double Radius, double HalfHeight, float ZOffset, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	const float	AngleDelta = 2.0f * UE_PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	int32 BaseVertIndex = OutVerts.Num();

	//Compute vertices for base circle.
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = FVector3f(Vertex - TopOffset);
		MeshVertex.TextureCoordinate[0] = FVector2f(TC);

		MeshVertex.SetTangents(
			(FVector3f)-ZAxis,
			FVector3f((-ZAxis) ^ Normal),
			(FVector3f)Normal
		);

		MeshVertex.Position.Z += ZOffset;
		OutVerts.Add(MeshVertex); //Add bottom vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = FVector3f(Vertex + TopOffset);
		MeshVertex.TextureCoordinate[0] = FVector2f(TC);

		MeshVertex.SetTangents(
			(FVector3f)-ZAxis,
			FVector3f((-ZAxis) ^ Normal),
			(FVector3f)Normal
		);

		MeshVertex.Position.Z += ZOffset;
		OutVerts.Add(MeshVertex); //Add top vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for (uint32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex;
		int32 V1 = BaseVertIndex + SideIndex;
		int32 V2 = BaseVertIndex + ((SideIndex + 1) % Sides);

		//bottom
		OutIndices.Add(V0);
		OutIndices.Add(V1);
		OutIndices.Add(V2);

		// top
		OutIndices.Add(Sides + V2);
		OutIndices.Add(Sides + V1);
		OutIndices.Add(Sides + V0);
	}

	//Add sides.

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex + SideIndex;
		int32 V1 = BaseVertIndex + ((SideIndex + 1) % Sides);
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		OutIndices.Add(V0);
		OutIndices.Add(V2);
		OutIndices.Add(V1);

		OutIndices.Add(V2);
		OutIndices.Add(V3);
		OutIndices.Add(V1);
	}

}


/** Represents a UXkHexagonArrowComponent to the scene manager. */
class FXkHexagonArrowSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FXkHexagonArrowSceneProxy(UXkHexagonArrowComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, VertexFactory(GetScene().GetFeatureLevel(), "FArrowSceneProxy")
		, ArrowHeight(Component->ArrowHeight)
		, ArrowZOffset(Component->ArrowZOffset)
		, ArrowUnitStep(Component->ArrowUnitStep)
		, ArrowMarkWidth(Component->ArrowMarkWidth)
		, ArrowColor(Component->ArrowColor)
		, ArrowXColor(Component->ArrowXColor)
		, ArrowYColor(Component->ArrowYColor)
		, ArrowZColor(Component->ArrowZColor)
		, ArrowSize(Component->ArrowSize)
		, ArrowLength(Component->ArrowLength)
		, bIsScreenSizeScaled(Component->bIsScreenSizeScaled)
		, ScreenSize(Component->ScreenSize)
#if WITH_EDITORONLY_DATA
		, bLightAttachment(Component->bLightAttachment)
		, bTreatAsASprite(Component->bTreatAsASprite)
		, bUseInEditorScaling(Component->bUseInEditorScaling)
		, EditorScale(Component->EditorScale)
#endif
	{
		bWillEverBeLit = false;
#if WITH_EDITOR
		// If in the editor, extract the sprite category from the component
		if (GIsEditor)
		{
			SpriteCategoryIndex = GEngine->GetSpriteCategoryIndex(Component->SpriteInfo.Category);
		}
#endif	//WITH_EDITOR

		const float HeadAngle = FMath::DegreesToRadians(ARROW_HEAD_ANGLE);
		const float DefaultLength = ArrowSize * ARROW_SCALE;
		const float TotalLength = ArrowSize * ArrowLength;
		const float HeadLength = DefaultLength * ARROW_HEAD_FACTOR;
		const float ShaftRadius = DefaultLength * ARROW_RADIUS_FACTOR;
		const float ShaftLength = (TotalLength - HeadLength * 0.5); // 10% overlap between shaft and head
		const FVector ShaftCenter = FVector(0, 0, 0);

		TArray<FDynamicMeshVertex> OutVerts;
		BuildXkHexagonConeVerts(HeadAngle, HeadAngle, -HeadLength, TotalLength, ArrowHeight + ArrowZOffset, 32, OutVerts, IndexBuffer.Indices);
		// build axis mark verts.
		for (int32 i = 1; i < floor(TotalLength / (ArrowUnitStep * 1.5)); i++)
		{
			BuildXkHexagonCylinderVerts(-FVector(ArrowUnitStep * i * 1.5, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0), FVector(1, 0, 0), ShaftRadius * 2.0, ArrowMarkWidth, ArrowHeight + ArrowZOffset, 16, OutVerts, IndexBuffer.Indices);
			BuildXkHexagonCylinderVerts(FVector(ArrowUnitStep * i * 1.5, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0), FVector(1, 0, 0), ShaftRadius * 2.0, ArrowMarkWidth, ArrowHeight + ArrowZOffset, 16, OutVerts, IndexBuffer.Indices);
		}
		BuildXkHexagonCylinderVerts(ShaftCenter, FVector(0, 0, 1), FVector(0, 1, 0), FVector(1, 0, 0), ShaftRadius, ShaftLength, ArrowHeight + ArrowZOffset, 16, OutVerts, IndexBuffer.Indices);

		VertexBuffers.InitFromDynamicVertex(&VertexFactory, OutVerts);

		// Enqueue initialization of render resource
		BeginInitResource(&IndexBuffer);
	}

	virtual ~FXkHexagonArrowSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	// FPrimitiveSceneProxy interface.

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_XkHexagonArrowComponent_DrawDynamicElements);

		FMatrix EffectiveLocalToWorld;
#if WITH_EDITOR
		if (bLightAttachment)
		{
			EffectiveLocalToWorld = GetLocalToWorld().GetMatrixWithoutScale();
		}
		else
#endif	//WITH_EDITOR
		{
			EffectiveLocalToWorld = GetLocalToWorld();
		}

		auto ArrowMaterialRenderProxy = new FColoredMaterialRenderProxy(
			GEngine->ArrowMaterial->GetRenderProxy(),
			ArrowColor,
			"GizmoColor"
		);
		auto ArrowXMaterialRenderProxy = new FColoredMaterialRenderProxy(
			GEngine->ArrowMaterial->GetRenderProxy(),
			ArrowXColor,
			"GizmoColor"
		);
		auto ArrowYMaterialRenderProxy = new FColoredMaterialRenderProxy(
			GEngine->ArrowMaterial->GetRenderProxy(),
			ArrowYColor,
			"GizmoColor"
		);
		auto ArrowZMaterialRenderProxy = new FColoredMaterialRenderProxy(
			GEngine->ArrowMaterial->GetRenderProxy(),
			ArrowZColor,
			"GizmoColor"
		);

		Collector.RegisterOneFrameMaterialProxy(ArrowMaterialRenderProxy);
		Collector.RegisterOneFrameMaterialProxy(ArrowXMaterialRenderProxy);
		Collector.RegisterOneFrameMaterialProxy(ArrowYMaterialRenderProxy);
		Collector.RegisterOneFrameMaterialProxy(ArrowZMaterialRenderProxy);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				// Calculate the view-dependent scaling factor.
				float ViewScale = 1.0f;
				if (bIsScreenSizeScaled && (View->ViewMatrices.GetProjectionMatrix().M[3][3] != 1.0f))
				{
					const float ZoomFactor = FMath::Min<float>(View->ViewMatrices.GetProjectionMatrix().M[0][0], View->ViewMatrices.GetProjectionMatrix().M[1][1]);
					if (ZoomFactor != 0.0f)
					{
						// Note: we can't just ignore the perspective scaling here if the object's origin is behind the camera, so preserve the scale minus its sign.
						const float Radius = FMath::Abs(View->WorldToScreen(Origin).W * (ScreenSize / ZoomFactor));
						if (Radius < 1.0f)
						{
							ViewScale *= Radius;
						}
					}
				}

#if WITH_EDITORONLY_DATA
				ViewScale *= EditorScale;
#endif
				TArray<float> ArrowRotator = { 0, -120, 120 };
				for (int32 i = 0; i < 3; i++)
				{
					FTransform Transform = FTransform(FRotator(0, ArrowRotator[i], 0), FVector(0, 0, 10), FVector(1));
					EffectiveLocalToWorld = Transform.ToMatrixWithScale() * GetLocalToWorld();

					// Draw the mesh.
					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = &IndexBuffer;
					Mesh.bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;
					Mesh.VertexFactory = &VertexFactory;
					Mesh.MaterialRenderProxy = (i == 0) ? ArrowXMaterialRenderProxy : ((i == 1) ? ArrowYMaterialRenderProxy : ArrowZMaterialRenderProxy);

					FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
					DynamicPrimitiveUniformBuffer.Set(FScaleMatrix(ViewScale) * EffectiveLocalToWorld, FScaleMatrix(ViewScale) * EffectiveLocalToWorld, GetBounds(), GetLocalBounds(), true, false, AlwaysHasVelocity());
					BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

					BatchElement.FirstIndex = 0;
					BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
					Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = SDPG_World;
					Mesh.bCanApplyViewModeOverrides = false;
					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && (View->Family->EngineShowFlags.BillboardSprites);
		Result.bDynamicRelevance = true;
#if WITH_EDITOR
		if (bTreatAsASprite)
		{
			if (GIsEditor && SpriteCategoryIndex != INDEX_NONE && SpriteCategoryIndex < View->SpriteCategoryVisibility.Num() && !View->SpriteCategoryVisibility[SpriteCategoryIndex])
			{
				Result.bDrawRelevance = false;
			}
		}
#endif
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override
#else
	virtual void OnTransformChanged() override
#endif
	{
		Origin = GetLocalToWorld().GetOrigin();
	}

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }
	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

private:
	FStaticMeshVertexBuffers VertexBuffers;
	FDynamicMeshIndexBuffer32 IndexBuffer;
	FLocalVertexFactory VertexFactory;

	FVector Origin;
	float ArrowHeight;
	float ArrowZOffset;
	float ArrowUnitStep;
	float ArrowMarkWidth;
	FColor ArrowColor;
	FColor ArrowXColor;
	FColor ArrowYColor;
	FColor ArrowZColor;
	float ArrowSize;
	float ArrowLength;
	bool bIsScreenSizeScaled;
	float ScreenSize;
#if WITH_EDITORONLY_DATA
	bool bLightAttachment;
	bool bTreatAsASprite;
	int32 SpriteCategoryIndex;
	bool bUseInEditorScaling;
	float EditorScale;
#endif // #if WITH_EDITORONLY_DATA
};


UXkHexagonArrowComponent::UXkHexagonArrowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ArrowHeight = 10.0;
	ArrowZOffset = 0.0;
	ArrowUnitStep = 100.0;
	ArrowMarkWidth = 1.0;
	ArrowXColor = FColor::Red;
	ArrowYColor = FColor::Green;
	ArrowZColor = FColor::Blue;
};


FPrimitiveSceneProxy* UXkHexagonArrowComponent::CreateSceneProxy()
{
	return new FXkHexagonArrowSceneProxy(this);
}

FBoxSphereBounds UXkHexagonArrowComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FBox(FVector(-ArrowSize * ArrowLength, -ArrowSize * ArrowLength, 0),
		FVector(ArrowSize * ArrowLength, ArrowSize * ArrowLength, ARROW_SCALE))).TransformBy(LocalToWorld);
}


UXkHexagonalWorldComponent::UXkHexagonalWorldComponent(const FObjectInitializer& ObjectInitializer) :
	UPrimitiveComponent(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bTickInEditor = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder(TEXT("/XkGamedevKit/Materials/M_HexagonBaseSpherical"));
	BaseMaterial = ObjectFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder2(TEXT("/XkGamedevKit/Materials/M_HexagonEdgeSpherical"));
	EdgeMaterial = ObjectFinder2.Object;

	Radius = 100.0;
	Height = 10.0;
	GapWidth = 0.0;
	BaseInnerGap = 0.0;
	BaseOuterGap = 0.0;
	EdgeInnerGap = 9.0;
	EdgeOuterGap = 1.0;
	MaxManhattanDistance = 32;

	bShowBaseMesh = false;
	bShowEdgeMesh = true;
}


void UXkHexagonalWorldComponent::PostLoad()
{
	Super::PostLoad();
}


FPrimitiveSceneProxy* UXkHexagonalWorldComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* HexagonalWorldceneProxy = NULL;
	if (BaseMaterial && EdgeMaterial)
	{
		FPrimitiveSceneProxy* Proxy = new FXkHexagonalWorldSceneProxy(
			this, NAME_None, BaseMaterial->GetRenderProxy(), EdgeMaterial->GetRenderProxy());
		HexagonalWorldceneProxy = Proxy;
	}
	return HexagonalWorldceneProxy;
}


FBoxSphereBounds UXkHexagonalWorldComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FVector2D Extent = GetHexagonalWorldExtent();
	float BoxRadius = FMath::Max(Extent.X, Extent.Y);
	FBoxSphereBounds BoxSphereBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(BoxRadius, BoxRadius, BoxRadius), BoxRadius);
	return FBoxSphereBounds(BoxSphereBounds).TransformBy(LocalToWorld);
}


void UXkHexagonalWorldComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


FMaterialRelevance UXkHexagonalWorldComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result;
	Result |= BaseMaterial->GetRelevance_Concurrent(InFeatureLevel);
	Result |= EdgeMaterial->GetRelevance_Concurrent(InFeatureLevel);
	return Result;
}


void UXkHexagonalWorldComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (BaseMaterial && EdgeMaterial)
	{
		OutMaterials.Add(BaseMaterial);
		OutMaterials.Add(EdgeMaterial);
	}
}


void UXkHexagonalWorldComponent::BuildHexagonData(TArray<FVector4f>& OutVertices, TArray<uint32>& OutIndices)
{
	if (FXkHexagonalWorldSceneProxy* HexagonalWorldSceneProxy = static_cast<FXkHexagonalWorldSceneProxy*>(SceneProxy))
	{
		OutVertices = HexagonalWorldSceneProxy->HexagonData.BaseVertices;
		OutIndices = HexagonalWorldSceneProxy->HexagonData.BaseIndices;
	}
}


FVector2D UXkHexagonalWorldComponent::GetHexagonalWorldExtent() const
{
	float Distance = Radius + GapWidth;
	float X = MaxManhattanDistance * Distance * 1.5 + Distance;
	float Y = MaxManhattanDistance * Distance * 2.0 * XkCos30 + Distance * XkCos30;
	return FVector2D(X, Y);
}


FVector2D UXkHexagonalWorldComponent::GetFullUnscaledWorldSize(const FVector2D& UnscaledPatchCoverage, const FVector2D& Resolution) const
{
	// UnscaledPatchCoverage is meant to represent the distance between the centers of the extremal pixels.
	// That distance in pixels is Resolution-1.
	FVector2D TargetPixelSize(UnscaledPatchCoverage / FVector2D::Max(Resolution - 1, FVector2D(1, 1)));
	return TargetPixelSize * Resolution;
}


UXkInstancedHexagonComponent::UXkInstancedHexagonComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/XkGamedevKit/Meshes/SM_StandardHexagonWithUV"));
	SetStaticMesh(ObjectFinder.Object);
}


UXkHexagonBasedFortressComponent::UXkHexagonBasedFortressComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TrapezoidWallMaterial = UMaterial::GetDefaultMaterial(MD_Surface);

	bEnableFlatShading = true;
	bEnableComplexCollision = true;
	bDeferCollisionUpdates = true;
	CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
    TrapezoidWallMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	CastShadow = true;
	bCastDynamicShadow = true;
	bCastStaticShadow = true;
}


void MakeTrapezoidBaseHexagon(
	FDynamicMesh3& Mesh,
	TArray<TPair<FVector, FVector>>& Edges,
	TArray<TArray<FVector>>& Contours,
	const TArray<TPair<FVector, FVector>>& EdgesToBlend,
	const FVector& Center,
	const FVector& PointTo,
	const float BtmRadius,
	const float TopRadius,
	const float Height,
	const bool bBlendingWithMesh,
	const bool bHalfRadialSlices,
	const int32 GroupId)
{
	using namespace UE::Geometry;
	const int32 RadialSlices = 6;
	FVector Target = (PointTo - Center).GetSafeNormal();
	FVector RightY = FVector::RightVector;
	FQuat Quat = FQuat::FindBetweenNormals(RightY, Target);
	FTransform Transform = FTransform(Quat, Center);

	TArray<FVector> StartVertices;
	TArray<FVector> EndVertices;
	for (const TPair<FVector, FVector>& Edge : EdgesToBlend)
	{
		StartVertices.Add(Edge.Key);
		EndVertices.Add(Edge.Value);
	}

	TArray<FVector> SideVertices;
	TMap<int32, int32> SidVerticesMap;
	TArray<FVector> TopVertices;
	TMap<int32, int32> TopVerticesMap;
	TArray<FVector> BtmVertices;
	TMap<int32, int32> BtmVerticesMap;
	float BtmWidth = BtmRadius / 2.0f;
	float TopWidth = TopRadius / 2.0f;

	int32 StartIndex = bHalfRadialSlices ? RadialSlices / 2 : 0;
	// Create vertices for the top and bottom circles
	for (int32 i = 0; i < RadialSlices; i++)
	{
		float Angle = (float)i / RadialSlices * UE_PI * 2.0f;
		FVector TopVert = FVector(TopWidth * FMath::Cos(Angle), TopWidth * FMath::Sin(Angle), Height);
		FVector BtmVert = FVector(BtmWidth * FMath::Cos(Angle), BtmWidth * FMath::Sin(Angle), 0.0f);

		TopVert = Transform.TransformPosition(TopVert);
		BtmVert = Transform.TransformPosition(BtmVert);
		if (bBlendingWithMesh)
		{
			TArray<FVector> MeshVertices;
			if (i == 4)
			{
				MeshVertices = StartVertices;
			}
			if (i == 5)
			{
				MeshVertices = EndVertices;
			}
			if (i == 4 || i == 5)
			{
				float MinBtmDist = TNumericLimits<float>::Max();
				float MinTopDist = TNumericLimits<float>::Max();
				for (const FVector& MeshVertex : MeshVertices)
				{
					float CurrBtmDist = FVector::Dist(MeshVertex, BtmVert);
					if (CurrBtmDist < BtmWidth && CurrBtmDist < MinBtmDist)
					{
						MinBtmDist = CurrBtmDist;
						BtmVert = MeshVertex;
					}
					float CurrTopDist = FVector::Dist(MeshVertex, TopVert);
					if (CurrTopDist < TopWidth && CurrTopDist < MinTopDist)
					{
						MinTopDist = CurrTopDist;
						TopVert = MeshVertex;
					}
				}
			}
		}

		SideVertices.Add(TopVert);
		SidVerticesMap.Add(SideVertices.Num() - 1, Mesh.AppendVertex((FVector3d)TopVert));
		TopVertices.Add(TopVert);
		TopVerticesMap.Add(TopVertices.Num() - 1, Mesh.AppendVertex((FVector3d)TopVert));
		SideVertices.Add(BtmVert);
		SidVerticesMap.Add(SideVertices.Num() - 1, Mesh.AppendVertex((FVector3d)BtmVert));
		BtmVertices.Add(BtmVert);
		BtmVerticesMap.Add(BtmVertices.Num() - 1, Mesh.AppendVertex((FVector3d)BtmVert));
	}
	FVector CenterTop = FVector(0.0f, 0.0f, Height);
	CenterTop = Transform.TransformPosition(CenterTop);
	FVector CenterBtm = FVector(0.0f, 0.0f, 0.0f);
	CenterBtm = Transform.TransformPosition(CenterBtm);
	TopVertices.Add(CenterTop); // Add center top vertex
	int32 c0 = Mesh.AppendVertex((FVector3d)CenterTop);
	TopVerticesMap.Add(TopVertices.Num() - 1, c0);
	BtmVertices.Add(CenterBtm); // Add center bottom vertex
	int32 c1 = Mesh.AppendVertex((FVector3d)CenterBtm);
	BtmVerticesMap.Add(BtmVertices.Num() - 1, c1);

	// Create indices for the trapezoid sides
	for (int32 i = StartIndex; i < RadialSlices; i++)
	{
		int32 NextIndex = (i + 1) % RadialSlices;
		int32 TopA = i * 2;
		int32 TopB = NextIndex * 2;
		int32 BtmA = i * 2 + 1;
		int32 BtmB = NextIndex * 2 + 1;
		// Create two triangles for each trapezoid side
		int32 i0 = SidVerticesMap[TopA];
		int32 i1 = SidVerticesMap[TopB];
		int32 i2 = SidVerticesMap[BtmA];
		Mesh.AppendTriangle(i0, i1, i2, GroupId);
		int32 i3 = SidVerticesMap[BtmA];
		int32 i4 = SidVerticesMap[TopB];
		int32 i5 = SidVerticesMap[BtmB];
		Mesh.AppendTriangle(i3, i4, i5, GroupId);
	}
	TArray<FVector> Contour;
	// Create indices for the top circle
	for (int32 i = StartIndex; i < RadialSlices; i++)
	{
		int32 NextIndex = (i + 1) % RadialSlices;
		int32 TopA = TopVerticesMap[i];
		int32 TopB = TopVerticesMap[NextIndex];
		int32 CenterTopIndex = TopVerticesMap[TopVertices.Num() - 1];
		Mesh.AppendTriangle(TopA, CenterTopIndex, TopB, GroupId);
		Contour.AddUnique(TopVertices[i]);
		Contour.AddUnique(TopVertices[NextIndex]);
		Edges.Add(TPair<FVector, FVector>(TopVertices[NextIndex], TopVertices[i]));
	}
	Contours.Add(Contour);
	// Create indices for the bottom circle
	for (int32 i = StartIndex; i < RadialSlices; i++)
	{
		int32 NextIndex = (i + 1) % RadialSlices;
		int32 BtmA = BtmVerticesMap[i];
		int32 BtmB = BtmVerticesMap[NextIndex];
		int32 CenterBtmIndex = BtmVerticesMap[BtmVertices.Num() - 1];
		Mesh.AppendTriangle(BtmA, BtmB, CenterBtmIndex, GroupId);
	}
}


void MakeTrapezoidBaseAlongLine(
	FDynamicMesh3& Mesh,
	TArray<TPair<FVector, FVector>>& TopEdges,
	TArray<TPair<FVector, FVector>>& BtmEdges,
	TArray<TArray<FVector>>& Contours,
	const FVector& Start,
	const FVector& End,
	bool bMoveEndToMid,
	float BottomWidth,
	float TopWidth,
	float Height,
	int32 GroupId)
{
	using namespace UE::Geometry;

	FVector Forward = (End - Start).GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	FVector Up = FVector::UpVector;

	float HalfBottom = BottomWidth * 0.5f;
	float HalfTop = TopWidth * 0.5f;

	// Start section plane points
	FVector v0 = Start - Right * HalfBottom;
	FVector v1 = Start + Right * HalfBottom;
	FVector v2 = Start + Right * HalfTop + Up * Height;
	FVector v3 = Start - Right * HalfTop + Up * Height;

	// End section plane points
	FVector RealEnd = End;
	if (bMoveEndToMid)
	{
		RealEnd = Start + Forward * (End - Start).Length() / 2.0;
	}
	FVector v4 = RealEnd - Right * HalfBottom;
	FVector v5 = RealEnd + Right * HalfBottom;
	FVector v6 = RealEnd + Right * HalfTop + Up * Height;
	FVector v7 = RealEnd - Right * HalfTop + Up * Height;

	// Start plane
	{
		int i0 = Mesh.AppendVertex((FVector3d)v0);
		int i1 = Mesh.AppendVertex((FVector3d)v1);
		int i2 = Mesh.AppendVertex((FVector3d)v2);
		int i3 = Mesh.AppendVertex((FVector3d)v3);
		Mesh.AppendTriangle(i0, i1, i3, GroupId);
		Mesh.AppendTriangle(i1, i2, i3, GroupId);
	}

	// End plane
	{
		int i4 = Mesh.AppendVertex((FVector3d)v4);
		int i5 = Mesh.AppendVertex((FVector3d)v5);
		int i6 = Mesh.AppendVertex((FVector3d)v6);
		int i7 = Mesh.AppendVertex((FVector3d)v7);
		Mesh.AppendTriangle(i4, i7, i5, GroupId);
		Mesh.AppendTriangle(i5, i7, i6, GroupId);
	}

	// Right plane
	{
		int i1 = Mesh.AppendVertex((FVector3d)v1);
		int i2 = Mesh.AppendVertex((FVector3d)v2);
		int i5 = Mesh.AppendVertex((FVector3d)v5);
		int i6 = Mesh.AppendVertex((FVector3d)v6);
		Mesh.AppendTriangle(i1, i5, i2, GroupId);
		Mesh.AppendTriangle(i2, i5, i6, GroupId);
	}

	// Left plane
	{
		int i0 = Mesh.AppendVertex((FVector3d)v0);
		int i3 = Mesh.AppendVertex((FVector3d)v3);
		int i4 = Mesh.AppendVertex((FVector3d)v4);
		int i7 = Mesh.AppendVertex((FVector3d)v7);
		Mesh.AppendTriangle(i3, i7, i0, GroupId);
		Mesh.AppendTriangle(i0, i7, i4, GroupId);
	}

	// Bottom plane
	{
		int i0 = Mesh.AppendVertex((FVector3d)v0);
		int i1 = Mesh.AppendVertex((FVector3d)v1);
		int i4 = Mesh.AppendVertex((FVector3d)v4);
		int i5 = Mesh.AppendVertex((FVector3d)v5);
		Mesh.AppendTriangle(i0, i4, i1, GroupId);
		Mesh.AppendTriangle(i1, i4, i5, GroupId);

		BtmEdges.Add(TPair<FVector, FVector>(v4, v0));
		BtmEdges.Add(TPair<FVector, FVector>(v1, v5));
	}

	// Top plane
	{
		int i2 = Mesh.AppendVertex((FVector3d)v2);
		int i3 = Mesh.AppendVertex((FVector3d)v3);
		int i6 = Mesh.AppendVertex((FVector3d)v6);
		int i7 = Mesh.AppendVertex((FVector3d)v7);
		Mesh.AppendTriangle(i2, i6, i3, GroupId);
		Mesh.AppendTriangle(i3, i6, i7, GroupId);

		TopEdges.Add(TPair<FVector, FVector>(v2, v6));
		TopEdges.Add(TPair<FVector, FVector>(v7, v3));

		Contours.Add(TArray<FVector>{ v3, v2, v6, v7 });
	}
}


void MakeTrapezoidWallAlongLine(
	FDynamicMesh3& Mesh,
	TArray<TPair<FVector, FVector>>& TopEdges,
	TArray<TPair<FVector, FVector>>& BtmEdges,
	const FVector& Start,
	const FVector& End,
	float BottomWidth,
	float TopWidth,
	float Height,
	int32 GroupId)
{
	TArray<TArray<FVector>> TmpContours;
	MakeTrapezoidBaseAlongLine(
		Mesh,
		TopEdges,
		BtmEdges,
		TmpContours,
		Start,
		End,
		false,
		BottomWidth,
		TopWidth,
		Height,
		GroupId);
}


void MakeTrapezoidWallHexagon(
	FDynamicMesh3& Mesh,
	const TArray<TPair<FVector, FVector>>& EdgesToBlend,
	const FVector& Center,
	const FVector& PointTo,
	const float BtmRadius,
	const float TopRadius,
	const float Height,
	const bool bBlendingWithMesh,
	const bool bHalfRadialSlices,
	const int32 GroupId)
{
	TArray<TPair<FVector, FVector>> TmpEdges;
	TArray<TArray<FVector>> TmpContours;
	MakeTrapezoidBaseHexagon(
		Mesh,
		TmpEdges,
		TmpContours,
		EdgesToBlend,
		Center,
		PointTo,
		BtmRadius,
		TopRadius,
		Height,
		bBlendingWithMesh,
		bHalfRadialSlices,
		GroupId);
}


void MakeTrapezoidTowerHexagon(
	FDynamicMesh3& Mesh,
	TArray<TPair<FVector, FVector>>& Edges,
	const FVector& Center,
	const FVector& PointTo,
	const float BtmRadius,
	const float TopRadius,
	const float Height,
	const int32 GroupId)
{
	TArray<TArray<FVector>> TmpContours;
	TArray<TPair<FVector, FVector>> TmpEdgesToBlend;
	MakeTrapezoidBaseHexagon(
		Mesh,
		Edges,
		TmpContours,
		TmpEdgesToBlend,
		Center,
		PointTo,
		BtmRadius,
		TopRadius,
		Height,
		false,
		false,
		GroupId);
}


void MakeTrapezoidGateAlongLine(
	FDynamicMesh3& Mesh,
	const FVector& Start,
	const FVector& End,
	float BottomWidth,
	float TopWidth,
	float Height,
	int32 GroupId)
{
	TArray<TPair<FVector, FVector>> TmpTopEdges;
	TArray<TPair<FVector, FVector>> TmpBtmEdges;
	TArray<TArray<FVector>> TmpContours;
	MakeTrapezoidBaseAlongLine(
		Mesh,
		TmpTopEdges,
		TmpBtmEdges,
		TmpContours,
		Start,
		End,
		false,
		BottomWidth,
		TopWidth,
		Height,
		GroupId);
}


void UXkHexagonBasedFortressComponent::UpdateHexagonBasedFortressBase()
{
	using namespace UE::Geometry;
	FDynamicMesh3 ShapeMesh = FDynamicMesh3();
	FVector Origin = GetComponentLocation();
	FVector TargetAmount = FVector::ZeroVector;
	TrapezoidBaseEdges.Empty();
	TrapezoidBaseContours.Empty();
	TArray<TPair<FVector, FVector>> EdgesToBlend;
	for (int32 Index = 0; Index < TrapezoidBaseAnchors.Num(); Index++)
	{
		FVector Target = TrapezoidBaseAnchors[Index];
		TargetAmount += Target;
		TArray<TPair<FVector, FVector>> TopEdges;
		TArray<TPair<FVector, FVector>> BtmEdges;
		TArray<TArray<FVector>> Contours;
		MakeTrapezoidBaseAlongLine(
			ShapeMesh,
			TopEdges,
			BtmEdges,
			Contours,
			Origin,
			Target,
			true,
			100.0f,
			65.0f,
			200.0f,
			0
		);
		TrapezoidBaseEdges.Append(TopEdges);
		TrapezoidBaseContours.Append(Contours);
		EdgesToBlend.Append(TopEdges);
		EdgesToBlend.Append(BtmEdges);
	}

	{
		FVector Target = TrapezoidBaseAnchors.Num() > 0 ? TargetAmount / TrapezoidBaseAnchors.Num() : Origin;
		Target.Z = Origin.Z;
		MakeTrapezoidBaseHexagon(
			ShapeMesh,
			TrapezoidBaseEdges,
			TrapezoidBaseContours,
			EdgesToBlend,
			Origin,
			Target,
			100.0f,
			65.0f,
			200.0f,
			true,
			TrapezoidBaseAnchors.Num() > 0,
			0);
	}
	UpdateTrapezoidDynamicMeshInternal(ShapeMesh, true);
	SetMaterial(0, TrapezoidWallMaterial);
	
#if WITH_EDITOR
	if (bExplicitShowWireframe)
	{
		TArray<FXkGeomEdge> Edges = GetTrapezoidBaseBoundaryEdges();
		for (FXkGeomEdge& Edge : Edges)
		{
			int32 Index = &Edge - Edges.GetData();
			FLinearColor Color = FLinearColor::MakeFromHSV8((Index * 37) % 255, 255, 255);
			Edge.DrawDebugEdge(GetWorld(), Color.ToFColor(false), false, -1.0f, SDPG_World, 3.0f);
		}
	}
#endif
}


void UXkHexagonBasedFortressComponent::UpdateHexagonBasedFortressWall()
{
	TArray<FXkGeomEdge> BoundaryEdges = GetTrapezoidBaseBoundaryEdges();
	using namespace UE::Geometry;
	FDynamicMesh3 ShapeMesh = BuildTrapezoidWallByBoundaryEdges(BoundaryEdges);
	UpdateTrapezoidDynamicMeshInternal(ShapeMesh);
	SetMaterial(0, TrapezoidWallMaterial);
}


void UXkHexagonBasedFortressComponent::UpdateHexagonBasedFortressTower()
{
	using namespace UE::Geometry;
	FDynamicMesh3 ShapeMesh = FDynamicMesh3();
	FVector Origin = GetComponentLocation();
	FVector TargetAmount = FVector::ZeroVector;
	TArray<TPair<FVector, FVector>> Edges;
	for (int32 Index = 0; Index < TrapezoidBaseAnchors.Num(); Index++)
	{
		FVector Target = TrapezoidBaseAnchors[Index];
		TargetAmount += Target;
	}
	{
		FVector Target = TrapezoidBaseAnchors.Num() > 0 ? TargetAmount / TrapezoidBaseAnchors.Num() : Origin;
		Target.Z = Origin.Z;
		MakeTrapezoidTowerHexagon(
			ShapeMesh,
			Edges,
			Origin,
			Target,
			100.0f,
			95.0f,
			300.0f,
			0);
	}
	TArray<FXkGeomEdge> BoundaryEdges;
	for (const TPair<FVector, FVector>& Edge : Edges)
	{
		BoundaryEdges.Add(FXkGeomEdge(Edge));
	}
	UpdateTrapezoidDynamicMeshInternal(ShapeMesh);
	FDynamicMesh3 WallShapeMesh = BuildTrapezoidWallByBoundaryEdges(BoundaryEdges);
	UpdateTrapezoidDynamicMeshInternal(WallShapeMesh);
	SetMaterial(0, TrapezoidWallMaterial);
}


void UXkHexagonBasedFortressComponent::UpdateHexagonBasedFortressGate()
{
	using namespace UE::Geometry;
	FDynamicMesh3 ShapeMesh = FDynamicMesh3();
	FVector Origin = GetComponentLocation();
	FVector Direction = FVector::ZeroVector;
	for (int32 Index = 0; Index < NeighborHexagonCenters.Num() / 2; Index++)
	{
		FVector A = NeighborHexagonCenters[Index];
		if (TrapezoidBaseAnchors.Contains(A))
		{
			continue;
		}
		FVector B = NeighborHexagonCenters[(Index + 3) % NeighborHexagonCenters.Num()];
		if (TrapezoidBaseAnchors.Contains(B))
		{
			continue;
		}
		Direction += (B - A).GetSafeNormal();

		MakeTrapezoidGateAlongLine(
			ShapeMesh,
			Origin - Direction.GetSafeNormal() * 100.0f + FVector(0.0f, 0.0f, -5.0f),
			Origin + Direction.GetSafeNormal() * 100.0f + FVector(0.0f, 0.0f, -5.0f),
			75.0f,
			55.0f,
			185.0f,
			0);
	}
	UDynamicMesh* DynamicMesh = GetDynamicMesh();
	FDynamicMesh3 OriginMesh(DynamicMesh->GetMeshRef());
	FDynamicMesh3 MergedMesh = BooleanOperationInternal(FMeshBoolean::EBooleanOp::Difference, OriginMesh, ShapeMesh, GetComponentTransform());
	UpdateTrapezoidDynamicMeshInternal(MergedMesh, true);

	TArray<TPair<FVector, FVector>> Edges;
	FDynamicMesh3 GateTowerMesh = FDynamicMesh3();
	MakeTrapezoidTowerHexagon(
		GateTowerMesh,
		Edges,
		Origin + FVector(0.0f, 0.0f, 200.0f),
		Origin + Direction.GetSafeNormal() * 100.0f + FVector(0.0f, 0.0f, 200.0f),
		125.0f,
		125.0f,
		35.0f,
		0);
	MakeTrapezoidTowerHexagon(
		GateTowerMesh,
		Edges,
		Origin + FVector(0.0f, 0.0f, 235.0f),
		Origin + Direction.GetSafeNormal() * 100.0f + FVector(0.0f, 0.0f, 235.0f),
		150.0f,
		10.0f,
		100.0f,
		0);
	TArray<FXkGeomEdge> BoundaryEdges;
	for (const TPair<FVector, FVector>& Edge : Edges)
	{
		BoundaryEdges.Add(FXkGeomEdge(Edge));
	}
	UpdateTrapezoidDynamicMeshInternal(GateTowerMesh);
	//FDynamicMesh3 WallShapeMesh = BuildTrapezoidWallByBoundaryEdges(BoundaryEdges);
	//UpdateTrapezoidDynamicMeshInternal(WallShapeMesh);

	SetMaterial(0, TrapezoidWallMaterial);
}


void UXkHexagonBasedFortressComponent::UpdateHexagonBasedFortressPhysics()
{
	UBodySetup* BodySetup = GetBodySetup();
	if (BodySetup)
	{
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		BodySetup->bMeshCollideAll = true;
	}
	UpdateCollision();
}


TArray<FXkGeomEdge> UXkHexagonBasedFortressComponent::GetTrapezoidBaseBoundaryEdges() const
{
	TArray<FXkGeomEdge> Edges;
	for (TPair<FVector, FVector> Edge : TrapezoidBaseEdges)
	{
		Edges.Add(FXkGeomEdge(Edge));
	}
	TArray<FVector2d> IntersectionPoints;
	for (FXkGeomEdge& Edge : Edges)
	{
		for (FXkGeomEdge& _Edge : Edges)
		{
			if (&Edge == &_Edge)
			{
				continue;
			}
			if (FVector2d Point; Edge.FindIntersection(Point, _Edge))
			{
				if (Edge.IsPointAtStartOrEnd(Point) || _Edge.IsPointAtStartOrEnd(Point))
				{
					continue;
				}
				IntersectionPoints.AddUnique(Point);
			}
		}
	}
	TArray<FXkGeomEdge> SplitedEdges;
	for (FXkGeomEdge& Edge : Edges)
	{
		bool bHasSplit = false;
		for (FVector2d Point : IntersectionPoints)
		{
			if (Edge.IsPointOnEdge(Point) && !Edge.IsPointAtStartOrEnd(Point))
			{
				SplitedEdges.Append(Edge.Split(Point));
				bHasSplit = true;
			}
		}
		if (!bHasSplit)
		{
			SplitedEdges.Add(Edge);
		}
	}
	TArray<FXkGeomEdge> BoundaryEdges;
	for (FXkGeomEdge& Edge : SplitedEdges)
	{
		FVector Center = Edge.GetCenter();
		FVector Right = Edge.GetRight();
		FVector Offset_L = Center - Right;
		FVector Offset_R = Center + Right;
		bool bIsLInside = false, bIsRInside = false;
		for (TArray<FVector> Contour : TrapezoidBaseContours)
		{
			if (FXkGeomEdge::CheckIsPointInsideEdgeLoops2D(Offset_L, Contour))
			{
				bIsLInside = true;
				break;
			}
		}
		for (TArray<FVector> Contour : TrapezoidBaseContours)
		{
			if (FXkGeomEdge::CheckIsPointInsideEdgeLoops2D(Offset_R, Contour))
			{
				bIsRInside = true;
				break;
			}
		}
		bool bIsBoundary = false;
		if (bIsLInside != bIsRInside)
		{
			BoundaryEdges.Add(Edge);
		}
	}
	return BoundaryEdges;
}


void UXkHexagonBasedFortressComponent::UpdateTrapezoidDynamicMeshInternal(const FDynamicMesh3& InDynamicMesh, const bool bForceUpdate)
{
	UDynamicMesh* DynamicMesh = GetDynamicMesh();
	if (!DynamicMesh || !IsValid(DynamicMesh))
	{
		DynamicMesh = NewObject<UDynamicMesh>(this);
	}
	FTransform Transform = GetComponentTransform();
	DynamicMesh->EditMesh([&](FDynamicMesh3& EditMesh)
		{
			if (bForceUpdate)
			{
				EditMesh = InDynamicMesh;
				for (int32 vid : EditMesh.VertexIndicesItr())
				{
					FVector3d Vertex = EditMesh.GetVertex(vid);
					EditMesh.SetVertex(vid, Transform.InverseTransformPosition(Vertex));
				}
			}
			else
			{
				TMap<int32, int32> VerticesMap;
				for (int32 vid : InDynamicMesh.VertexIndicesItr())
				{
					FVector3d Vertex = InDynamicMesh.GetVertex(vid);
					Vertex = (FVector3d)Transform.InverseTransformPosition((FVector)Vertex);
					int32 NewVid = EditMesh.AppendVertex(Vertex);
					VerticesMap.Add(vid, NewVid);
				}
				for (int32 tid : InDynamicMesh.TriangleIndicesItr())
				{
					FIndex3i Triangle = InDynamicMesh.GetTriangle(tid);
					EditMesh.AppendTriangle(
						VerticesMap[Triangle.A],
						VerticesMap[Triangle.B],
						VerticesMap[Triangle.C],
						0);
				}
			}
			// Skip Normals, use flat shading
		});
	SetDynamicMesh(DynamicMesh);
}


FDynamicMesh3 UXkHexagonBasedFortressComponent::BuildTrapezoidWallByBoundaryEdges(const TArray<FXkGeomEdge>& InBoundaryEdges)
{
	using namespace UE::Geometry;
	FDynamicMesh3 ResultMesh = FDynamicMesh3();
	TArray<TPair<FVector, FVector>> TopEdges;
	TArray<TPair<FVector, FVector>> BtmEdges;
	for (const FXkGeomEdge& Edge : InBoundaryEdges)
	{
		MakeTrapezoidWallAlongLine(
			ResultMesh,
			TopEdges,
			BtmEdges,
			Edge.GetStart(),
			Edge.GetEnd(),
			10.0f,
			10.0f,
			20.0f,
			0
		);
	}
	TArray<TPair<FVector, FVector>> EdgesToBlend;
	EdgesToBlend.Append(TopEdges);
	EdgesToBlend.Append(BtmEdges);
	TArray<const FXkGeomEdge*> ProcessedEdges;
	for (const FXkGeomEdge& Edge : InBoundaryEdges)
	{
		for (const FXkGeomEdge& _Edge : InBoundaryEdges)
		{
			if (&Edge == &_Edge || ProcessedEdges.Contains(&_Edge))
			{
				continue;
			}
			FVector TargetAmount = FVector::ZeroVector;
			if (FVector Point; Edge.FindIntersection(Point, _Edge))
			{
				FVector A1 = Edge.GetStart();
				FVector B1 = Edge.GetEnd();
				FVector A2 = _Edge.GetStart();
				FVector B2 = _Edge.GetEnd();
				FVector V1 = (B1 - A1).GetSafeNormal();
				if (((FVector2d)Point).Equals((FVector2d)B1))
				{
					V1 = (A1 - B1).GetSafeNormal();
				}
				FVector V2 = (B2 - A2).GetSafeNormal();
				if (((FVector2d)Point).Equals((FVector2d)B2))
				{
					V2 = (A2 - B2).GetSafeNormal();
				}
				FVector Target = Point + (V1 + V2).GetSafeNormal() * 25.0f;
				Target.Z = Point.Z;
				MakeTrapezoidWallHexagon(
					ResultMesh,
					EdgesToBlend,
					Point,
					Target,
					10.0f,
					10.0f,
					20.0f,
					true,
					true,
					0
				);
			}
		}
		ProcessedEdges.Add(&Edge);
	}
	return ResultMesh;
}


FDynamicMesh3 UXkHexagonBasedFortressComponent::BooleanOperationInternal(const FMeshBoolean::EBooleanOp Operation, 
	const FDynamicMesh3& MeshA, const FDynamicMesh3& MeshB, const FTransformSRT3d& TransformA, const FTransformSRT3d& TransformB)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UXkHexagonBasedFortressComponent::BooleanOp);

	// Perform the actual boolean operation.
	FDynamicMesh3 BooleanResultMesh(EMeshComponents::None);
	FMeshBoolean BooleanOperation(&MeshA, TransformA, &MeshB, TransformB, &BooleanResultMesh, Operation);
	//BooleanOperation.
	bool bSuccess = BooleanOperation.Compute();
	if (!bSuccess)
	{
		if (BooleanResultMesh.IsClosed() == false)
		{
			// try to close any cracks
			FMergeCoincidentMeshEdges Merge(&BooleanResultMesh);
			Merge.MergeVertexTolerance = FMathf::ZeroTolerance * 10.0;
			Merge.Apply();

			// fill any holes
			FMeshBoundaryLoops BoundaryLoops(&BooleanResultMesh, true);
			for (FEdgeLoop& Loop : BoundaryLoops.Loops)
			{
				FMinimalHoleFiller Filler(&BooleanResultMesh, Loop);
				Filler.Fill();
			}
		}
	}
	return BooleanResultMesh;
}