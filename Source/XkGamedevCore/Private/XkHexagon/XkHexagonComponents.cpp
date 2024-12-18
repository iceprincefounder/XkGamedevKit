// Copyright ©XUKAI. All Rights Reserved.


#include "XkHexagon/XkHexagonComponents.h"
#include "Components/ArrowComponent.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/CollisionProfile.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "UObject/UObjectIterator.h"
#include "StaticMeshResources.h"
#include "XkHexagon/XkHexagonSceneProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(XkHexagonComponents)

#define DEFAULT_SCREEN_SIZE	(0.0025f)
#define ARROW_SCALE			(80.0f)
#define ARROW_RADIUS_FACTOR	(0.03f)
#define ARROW_HEAD_FACTOR	(0.2f)
#define ARROW_HEAD_ANGLE	(20.f)

UInterface_HexagonalWorld::UInterface_HexagonalWorld(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

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
				for (int32 i = 0; i < 3; i++)
				{
					FTransform Transform = FTransform(FRotator(0, 60 * i, 0), FVector(0, 0, 10), FVector(1));
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

	virtual void OnTransformChanged() override
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
	MaxManhattanDistance = 64;

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