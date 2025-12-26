// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XkBuoyancy.generated.h"

//Custom bone density/radius override struct.
USTRUCT(BlueprintType)
struct XKGAMEDEVCORE_API FXkSkeletalMeshBonesOverride
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyancy)
	FName BoneName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyancy)
	float Density;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyancy)
	float TestRadius;

	//Default struct values
	FXkSkeletalMeshBonesOverride()
	{
		Density = 600.f;
		TestRadius = 10.f;
	}
};

/**
 *	Applies buoyancy forces to physics objects.
 */
UCLASS(Blueprintable, showcategories = (Trigger), meta = (BlueprintSpawnableComponent))
class XKGAMEDEVCORE_API UXkBuoyancyComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/* Custom horizon height offset.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float HorizonHeight;
	
	/* Use simple wave height calculation (faster).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	bool bUseSimpleWaveHeight;

	/**
	* Height scale for shoreline waves, should as same as the wave height scale in water material.
	* IMPORTANT: set manually
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float WaveHeightScale;

	/**
	* Wave time scale, should as same as the water waves time scale in water material.
	* IMPORTANT: set manually
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float WaveTimeScale;

	/* Density of mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float MeshDensity;

	/* Density of water. Typically you don't need to change this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float FluidDensity;

	/* Linear damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float FluidLinearDamping;

	/* Angular damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float FluidAngularDamping;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	FVector VelocityDamper;

	/* Add Force to every pontoons, all forces equal to gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	uint32 bEnableForceToPontoons : 1;

	/* Radius of the points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float PontoonRadius;

	/* Test point array. At least one point is required for buoyancy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	TArray<FVector> Pontoons;

	/* Per-point mesh density override, can be used for half-sinking objects etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	TArray<float> PointDensityOverride;
	
	/* If skeletal mesh with physics asset, it will apply buoyancy force at the COM of each bone instead of using the test point array. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	uint32 bEnableForceToBones : 1;

	/* Density & radius overrides per skeletal bone (bEnableForceToBones needs to be true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	TArray<FXkSkeletalMeshBonesOverride> BoneOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	uint32 bDebugMode : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	bool ClampMaxVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float MaxUnderwaterVelocity;

	/** Waves will push objects towards the wave direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	uint16 bEnableWaveForces : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float WaveForceMultiplier;

	/** Stay upright physics constraint (inspired by UDK's StayUprightSpring) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	uint16 bEnableStayUprightConstraint : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float StayUprightStiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	float StayUprightDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	FRotator StayUprightDesiredRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy [KEVINTSUIXUGAMEDEV]")
	TEnumAsByte<enum ETickingGroup> TickGroup;

private: 
	UPROPERTY()
	TWeakObjectPtr<class UWaterWavesAsset> SpecifiedWaterWaves;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> SpecifiedSkeletalMesh;

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> SpecifiedStaticMesh;

public:
	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	//~ End UActorComponent interface

	UFUNCTION(BlueprintPure, Category = "Buoyancy Settings [KEVINTSUIXUGAMEDEV]")
	FVector GetBuoyancyWaterWavesLocation(const FVector& InLocation) const { return GetWaveLocation(InLocation); }
	UFUNCTION(BlueprintCallable, Category = "Buoyancy Settings [KEVINTSUIXUGAMEDEV]")
	void SetBuoyancyWaterWavesAsset(class UWaterWavesAsset* WaterWavesAsset);
	UFUNCTION(BlueprintCallable, Category = "Buoyancy Settings [KEVINTSUIXUGAMEDEV]")
	void SetBuoyancySkeletalMesh(USkeletalMeshComponent* Component);
	UFUNCTION(BlueprintCallable, Category = "Buoyancy Settings [KEVINTSUIXUGAMEDEV]")
	void SetBuoyancyStaticMesh(UStaticMeshComponent* Component);

private:
	float GetWaveHeight(const FVector& InLocation = FVector::ZeroVector) const;
	FVector GetWaveLocation(const FVector& InLocation = FVector::ZeroVector) const;
	FVector GetWaveDirection() const;
	FVector GetWorldPosition() const;

	static FVector GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName = NAME_None);
	void ApplyUprightConstraint(UPrimitiveComponent* BasePrimComp);

	float PrimtiveAngularDamping;
	float PrimtiveLinearDamping;

	UWorld* World;
};