// Copyright ©xukai. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "XkCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementBaseBeginEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementBaseFinishEvent);

UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkMovement : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxVelocity;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0"))
	float MaxAcceleration;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator RotationRate;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementBaseBeginEvent OnMovementBaseBeginEvent;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementBaseFinishEvent OnMovementBaseFinishEvent;
public:
	UFUNCTION(BlueprintCallable, Category = "Movement [KEVINTSUIXU GAMEDEV]")
	FORCEINLINE FVector GetVelocity() const { return Velocity; };
	UFUNCTION(BlueprintCallable, Category = "Movement [KEVINTSUIXU GAMEDEV]")
	FORCEINLINE FVector GetAcceleration() const { return Acceleration; };

	FORCEINLINE virtual bool ShouldMove() const { return bShouldMove; }
	FORCEINLINE virtual bool IsOnMoving() const { return bIsMoving || bIsRotating; }
	FORCEINLINE virtual void OnMoving() { bShouldMove = true; };
	FORCEINLINE virtual AActor* GetMovingActor() const;

	static bool CheckRotationSafely(const FRotator& A, const FRotator& B, const float Tolerance = 0.1f)
	{
		return (FMath::Abs(A.Yaw - B.Yaw) < Tolerance);
	};
	static bool CheckMovementSafely(const FVector& A, const FVector& B, const float Tolerance = 0.1f)
	{
		return (FVector::Dist2D(A, B) < Tolerance);
	};
protected:
	/** Should move but might not be moving currently*/
	UPROPERTY()
	bool bShouldMove;
	/** Is on moving, tick component is working*/
	UPROPERTY()
	bool bIsMoving;
	UPROPERTY()
	bool bIsRotating;
	UPROPERTY()
	FVector Velocity;
	UPROPERTY()
	FVector Acceleration;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementTargetFinishEvent, uint8, MovementPoint);

/**
 * Movement base on multiple targets input. 
 */
UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkTargetMovement : public UXkMovement
{
	GENERATED_BODY()

	/** Move to target very fast mode.*/
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bBlinkMode;

	/** The max move point of hexagon in one turn.*/
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	uint8 MovementPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector CurrentMovementTarget;
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingMovementTargets;
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator CurrentRotationTarget;
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingRotationTargets;

public:
	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementTargetFinishEvent OnMovementTargetFinishEvent;

	/** Default UObject constructor. */
	UXkTargetMovement(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin ActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End ActorComponent Interface

	//~ Begin UXkMovement Interface
	FORCEINLINE virtual void OnMoving() override;
	//~ End UXkMovement Interface
	
	//~ Begin UXkTargetMovement Interface
	FORCEINLINE virtual void AddMovementPoint(const uint8 Input);
	FORCEINLINE virtual void ClearMovementPoint() { MovementPoint = 0; };
	FORCEINLINE virtual uint8 GetMovementPoint() const { return MovementPoint; };
	FORCEINLINE virtual void SetMovementPoint(const uint8 Input) { MovementPoint = Input; };
	/**
	* Add movement targets should from start target to final target.
	* @param Target The location which should move to
	*/
	FORCEINLINE virtual void AddMovementTarget(const FVector& Target);
	FORCEINLINE virtual void ClearMovementTargets() { PendingMovementTargets.Empty(); };
	FORCEINLINE virtual TArray<FVector> GetMovementTargets() const { return PendingMovementTargets; };
	FORCEINLINE virtual void SetMovementTargets(const TArray<FVector>& Inputs) { PendingMovementTargets = Inputs; };
	FORCEINLINE virtual void AddRotationTarget(const FVector& Target) { PendingRotationTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearRotationTargets() { PendingRotationTargets.Empty(); };
	FORCEINLINE virtual TArray<FVector> GetRotationTargets() const { return PendingRotationTargets; };
	FORCEINLINE virtual void SetRotationTargets(const TArray<FVector>& Inputs) { PendingRotationTargets = Inputs; };
	/** Valid movement targets base on current movement point.*/
	FORCEINLINE virtual TArray<FVector> GetValidMovementTargets() const;
	/** Final movement target base on current movement point.*/
	FORCEINLINE virtual FVector GetFinalMovementTarget() const;
	//~ End UXkTargetMovement Interface
};


/**
 * Movement base on spline input
 */
UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkSplineMovement : public UXkMovement
{
	GENERATED_BODY()

public:
	/** Default UObject constructor. */
	UXkSplineMovement(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE virtual bool IsOnSpline() const;

	FORCEINLINE virtual void SetTargetSpline(class USplineComponent* Input);

private:
	// The spline which character move alone side.
	UPROPERTY()
	TWeakObjectPtr<class USplineComponent> TargetSpline;
	UPROPERTY()
	float CurrentLength;
};


UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Default UObject constructor. */
	AXkCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame.
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {};
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit) {};
};