// Copyright ©ICEPRINCE. All Rights Reserved.
#include "XkLandscape/XkBuoyancy.h"
#include "XkLandscape/XkGerstnerWave.h"
#include "WaterWaves.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/PhysicsVolume.h"
#include "PhysicsEngine/ConstraintInstance.h"

UXkBuoyancyComponent::UXkBuoyancyComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PrimaryComponentTick.TickGroup = TickGroup;
	SetComponentTickEnabled(true);
	bTickInEditor = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;

	// Default Water Waves
	HorizonHeight = 0.0f;
	bUseSimpleWaveHeight = true;
	WaveHeightScale = 1.0f;
	WaveTimeScale = 1.0f;

	//Defaults
	MeshDensity = 600.0f;
	FluidDensity = 1025.0f;
	PontoonRadius = 10.0f;
	FluidLinearDamping = 1.0f;
	FluidAngularDamping = 1.0f;

	VelocityDamper = FVector(0.1, 0.1, 0.1);
	MaxUnderwaterVelocity = 1000.f;

	bDebugMode = false;
	bEnableForceToPontoons = true;
	bEnableForceToBones = false;
	bEnableWaveForces = false;
	bEnableStayUprightConstraint = false;

	StayUprightStiffness = 50.0f;
	StayUprightDamping = 5.0f;

	WaveForceMultiplier = 2.0f;
}

void UXkBuoyancyComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//Store the world ref.
	World = GetWorld();

	PontoonRadius = FMath::Abs(PontoonRadius);

	UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	if (BasePrimComp)
	{
		ApplyUprightConstraint(BasePrimComp);

		//Store the initial damping values.
		PrimtiveLinearDamping = BasePrimComp->GetLinearDamping();
		PrimtiveAngularDamping = BasePrimComp->GetAngularDamping();
	}
}

void UXkBuoyancyComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If disabled or we are not attached to a parent component, return.
	if (!IsActive() || !GetOwner()->GetRootComponent()) return;

	UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	bool bIsUnderWater = false;

	//--------------- If Skeletal ---------------
	USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(BasePrimComp);
	if (SpecifiedSkeletalMesh.IsValid())
	{
		SkeletalComp = SpecifiedSkeletalMesh.Get();
	}
	if (SkeletalComp && SkeletalComp->IsSimulatingPhysics() && bEnableForceToBones)
	{
		//Get gravity
		float Gravity = SkeletalComp->GetPhysicsVolume()->GetGravityZ();

		TArray<FName> BoneNames;
		SkeletalComp->GetBoneNames(BoneNames);
		for (int32 Itr = 0; Itr < BoneNames.Num(); Itr++)
		{
			FBodyInstance* BI = SkeletalComp->GetBodyInstance(BoneNames[Itr], false);
			if (BI && BI->IsValidBodyInstance()
				&& BI->bEnableGravity) //Buoyancy doesn't exist without gravity
			{
				bIsUnderWater = false;
				FVector WorldBoneLocation = BI->GetCOMPosition(); //Use center of mass of the bone's physics body instead of bone's location
				FVector WorldWaveLocation = GetWaveLocation(WorldBoneLocation);
				float BoneDensity = MeshDensity;
				float BoneTestRadius = FMath::Abs(PontoonRadius);
				float SignedBoneRadius = FMath::Sign(Gravity) * PontoonRadius;

				//Get density & radius from the override array, if available.
				for (int PointIndex = 0; PointIndex < BoneOverride.Num(); PointIndex++)
				{
					FXkSkeletalMeshBonesOverride Override = BoneOverride[PointIndex];

					if (Override.BoneName.IsEqual(BoneNames[Itr]))
					{
						BoneDensity = Override.Density;
						BoneTestRadius = FMath::Abs(Override.TestRadius);
						SignedBoneRadius = FMath::Sign(Gravity) * BoneTestRadius;
					}
				}

				//If test point radius is below water surface, add buoyancy force.
				if (WorldWaveLocation.Z > (WorldBoneLocation.Z + SignedBoneRadius))
				{
					bIsUnderWater = true;

					float DepthMultiplier = (WorldWaveLocation.Z - (WorldBoneLocation.Z + SignedBoneRadius)) / (BoneTestRadius * 2);
					DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

					float Mass = SkeletalComp->CalculateMass(BoneNames[Itr]); //Mass of this specific bone's physics body

					/**
				   * --------
				   * Buoyancy force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) / Total Points * Depth Multiplier
				   * --------
				   */
					float BuoyancyForceZ = Mass / BoneDensity * FluidDensity * -Gravity * DepthMultiplier;

					//Velocity damping.
					FVector DampingForce = -BI->GetUnrealWorldVelocity() * VelocityDamper * Mass * DepthMultiplier;

					//Experimental x,y wave force
					if (bEnableWaveForces)
					{
						float WaveVelocity = FMath::Clamp(BI->GetUnrealWorldVelocity().Z, -20.f, 150.f) * (1 - DepthMultiplier);
						DampingForce += GetWaveDirection() * Mass * WaveVelocity * WaveForceMultiplier;
					}

					//Add force to this bone
					BI->AddForce(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyancyForceZ));
				}

				//Apply fluid damping & clamp velocity
				if (bIsUnderWater)
				{
					BI->SetLinearVelocity(-BI->GetUnrealWorldVelocity() * (FluidLinearDamping / 10), true);
					BI->SetAngularVelocityInRadians(-BI->GetUnrealWorldAngularVelocityInRadians() * FMath::DegreesToRadians(FluidAngularDamping / 10), true);

					//Clamp the velocity to MaxUnderwaterVelocity
					if (ClampMaxVelocity && BI->GetUnrealWorldVelocity().Size() > MaxUnderwaterVelocity)
					{
						FVector	Velocity = BI->GetUnrealWorldVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
						BI->SetLinearVelocity(Velocity, false);
					}
				}
#if WITH_EDITOR
				if (bDebugMode)
				{
#if ENABLE_DRAW_DEBUG
					FColor DebugColor = FColor::Yellow;
					if (bIsUnderWater) { DebugColor = FColor::White; }
					::DrawDebugSphere(GetWorld(), WorldBoneLocation, BoneTestRadius, 12, DebugColor, false, -1.0f, SDPG_World, 3.0f);
#endif
				}
#endif
			}
		}
		return;
	}

	//--------------- If With Pontoons ---------------
	UStaticMeshComponent* GeometryComp = Cast<UStaticMeshComponent>(BasePrimComp);
	if (SpecifiedStaticMesh.IsValid())
	{
		GeometryComp = SpecifiedStaticMesh.Get();
	}
	if (GeometryComp && GeometryComp->IsSimulatingPhysics() && bEnableForceToPontoons)
	{
		//Get gravity
		float Gravity = GeometryComp->GetPhysicsVolume()->GetGravityZ();

		float TotalPoints = Pontoons.Num();
		if (TotalPoints < 1) return;

		int PointsUnderWater = 0;
		for (int PointIndex = 0; PointIndex < TotalPoints; PointIndex++)
		{
			if (!Pontoons.IsValidIndex(PointIndex)) return; //Array size changed during runtime

			bIsUnderWater = false;
			FVector TestPoint = Pontoons[PointIndex];
			FVector WorldTestPoint = GeometryComp->GetComponentTransform().TransformPosition(TestPoint);
			FVector WorldWaveLocation = GetWaveLocation(TestPoint);

			float SignedRadius = FMath::Sign(GeometryComp->GetPhysicsVolume()->GetGravityZ()) * PontoonRadius;

			//If test point radius is below water surface, add buoyancy force.
			if (WorldWaveLocation.Z > (WorldTestPoint.Z + SignedRadius)
				&& GeometryComp->IsGravityEnabled()) //Buoyancy doesn't exist without gravity
			{
				PointsUnderWater++;
				bIsUnderWater = true;

				float DepthMultiplier = (WorldWaveLocation.Z - (WorldTestPoint.Z + SignedRadius)) / (PontoonRadius * 2);
				DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

				//If we have a point density override, use the overridden value instead of MeshDensity
				float PointDensity = PointDensityOverride.IsValidIndex(PointIndex) ? PointDensityOverride[PointIndex] : MeshDensity;

				/**
				* --------
				* Buoyancy force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) / Total Points * Depth Multiplier
				* --------
				*/
				float BuoyancyForceZ = GeometryComp->GetMass() / PointDensity * FluidDensity * -Gravity / TotalPoints * DepthMultiplier;

				//Experimental velocity damping using GetUnrealWorldVelocityAtPoint!
				FVector DampingForce = -GetUnrealVelocityAtPoint(GeometryComp, WorldTestPoint) * VelocityDamper * GeometryComp->GetMass() * DepthMultiplier;

				//Experimental x,y wave force
				if (bEnableWaveForces)
				{
					DampingForce += GeometryComp->GetMass() * FVector2D(WorldWaveLocation.X, WorldWaveLocation.Y).Size() * GetWaveDirection() * WaveForceMultiplier / TotalPoints;
				}

				//Add force for this test point
				GeometryComp->AddForceAtLocation(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyancyForceZ), WorldTestPoint);
			}
			//Clamp the velocity to MaxUnderwaterVelocity if there is any point underwater
			if (ClampMaxVelocity && PointsUnderWater > 0
				&& GeometryComp->GetPhysicsLinearVelocity().Size() > MaxUnderwaterVelocity)
			{
				FVector	Velocity = GeometryComp->GetPhysicsLinearVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
				GeometryComp->SetPhysicsLinearVelocity(Velocity);
			}

			//Update damping based on number of underwater test points
			GeometryComp->SetLinearDamping(PrimtiveLinearDamping + FluidLinearDamping / TotalPoints * PointsUnderWater);
			GeometryComp->SetAngularDamping(PrimtiveAngularDamping + FluidAngularDamping / TotalPoints * PointsUnderWater);
#if WITH_EDITOR
			if (bDebugMode)
			{
#if ENABLE_DRAW_DEBUG
				FColor DebugColor = FColor::Yellow;
				if (bIsUnderWater) { DebugColor = FColor::White; }
				::DrawDebugSphere(GetWorld(), WorldTestPoint, PontoonRadius, 12, DebugColor, false, -1.0f, SDPG_World, 3.0f);
#endif
			}
#endif
		}
		return;
	}
}

void UXkBuoyancyComponent::SetBuoyancyWaterWavesAsset(UWaterWavesAsset* WaterWavesAsset)
{
	SpecifiedWaterWaves = MakeWeakObjectPtr(WaterWavesAsset);
}

void UXkBuoyancyComponent::SetBuoyancySkeletalMesh(USkeletalMeshComponent* Component)
{
	SpecifiedSkeletalMesh = MakeWeakObjectPtr(Component);
}

void UXkBuoyancyComponent::SetBuoyancyStaticMesh(UStaticMeshComponent* Component)
{
	SpecifiedStaticMesh = MakeWeakObjectPtr(Component);
}

float UXkBuoyancyComponent::GetWaveHeight(const FVector& InLocation) const
{
	return GetWaveLocation(InLocation).Z;
}

FVector UXkBuoyancyComponent::GetWaveLocation(const FVector& InLocation) const
{
	double Time = GetWorld()->GetTimeSeconds() * WaveTimeScale;
	if (SpecifiedWaterWaves.IsValid())
	{
		float Height = HorizonHeight;
		if (bUseSimpleWaveHeight)
		{
			Height += SpecifiedWaterWaves->GetWaterWaves()->GetSimpleWaveHeightAtPosition(InLocation, 1000.0f, Time);
		}
		else
		{
			FVector OutNormal;
			Height += SpecifiedWaterWaves->GetWaterWaves()->GetWaveHeightAtPosition(InLocation, 1000.0f, Time, OutNormal);
		}
		return FVector(InLocation.X, InLocation.Y, Height);
	}
	float Height = GetWorldPosition().Z;
	for (int32 Index = 1; Index < 4; Index++)
	{
		Height += FMath::Sin((InLocation.X + InLocation.Y) * 0.1f + Time / Index) * 2.5f * Index;
	}
	return FVector(InLocation.X, InLocation.Y, Height);
}

FVector UXkBuoyancyComponent::GetWaveDirection() const
{
	return FVector();
}

FVector UXkBuoyancyComponent::GetWorldPosition() const
{
	return GetOwner()->GetActorLocation();
}

FVector UXkBuoyancyComponent::GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName)
{
	if (!Target) return FVector::ZeroVector;

	FBodyInstance* BI = Target->GetBodyInstance(BoneName);
	if (BI->IsValidBodyInstance())
	{
		return BI->GetUnrealWorldVelocityAtPoint(Point);
	}

	return FVector::ZeroVector;
}

void UXkBuoyancyComponent::ApplyUprightConstraint(UPrimitiveComponent* BasePrimComp)
{
	//Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	if (bEnableStayUprightConstraint)
	{
		UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(BasePrimComp);

		//Settings
		FConstraintInstance ConstraintInstance;

		ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);

		//ConstraintInstance.LinearLimitSize = 0;

		//ConstraintInstance.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Limited);

		ConstraintInstance.SetOrientationDriveTwistAndSwing(true, true);

		//ConstraintInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

		ConstraintInstance.SetAngularDriveParams(StayUprightStiffness, StayUprightDamping, 0);

		ConstraintInstance.AngularRotationOffset = BasePrimComp->GetComponentRotation().GetInverse() + StayUprightDesiredRotation;

		//UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(BasePrimComp);
		if (ConstraintComp)
		{
			ConstraintComp->ConstraintInstance = ConstraintInstance; //Set instance parameters
			ConstraintComp->SetWorldLocation(BasePrimComp->GetComponentLocation());

			//Attach
			ConstraintComp->AttachToComponent(BasePrimComp, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			ConstraintComp->SetConstrainedComponents(BasePrimComp, NAME_None, NULL, NAME_None);
			ConstraintComp->RegisterComponent();
		}
	}
}