// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "XkCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementBeginEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementFinishEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementReachTargetEvent, int32, ActionPoint);

UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkMovement : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ForceUnits = "cm/s"))
	FVector Velocity;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ForceUnits = "cm/s"))
	FVector Acceleration;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxVelocity;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxAcceleration;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator RotationRate;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementBeginEvent OnMovementBeginEvent;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementFinishEvent OnMovementFinishEvent;
public:
	UFUNCTION(BlueprintCallable, Category = "Movement [KEVINTSUIXUGAMEDEV]")
	FORCEINLINE FVector GetVelocity() const { return Velocity; };

	UFUNCTION(BlueprintCallable, Category = "Movement [KEVINTSUIXUGAMEDEV]")
	FORCEINLINE FVector GetAcceleration() const { return Acceleration; };

	FORCEINLINE virtual bool ShouldDoAction() const { return bShouldDoAction; }
	FORCEINLINE virtual bool IsOnAction() const { return bIsMoving || bIsRotating || bIsJumping || bIsShotting || bIsSliding; }
	FORCEINLINE virtual void OnAction() { bShouldDoAction = true; bIsMoving = bIsRotating = bIsJumping = bIsShotting = bIsSliding = false; };
	FORCEINLINE virtual bool IsMoving() const { return bIsMoving; };
	FORCEINLINE virtual bool IsRotating() const { return bIsRotating; };
	FORCEINLINE virtual bool IsJumping() const { return bIsJumping; };
	FORCEINLINE virtual bool IsShotting() const { return bIsShotting; };
	FORCEINLINE virtual bool IsSliding() const { return bIsSliding; };
	FORCEINLINE virtual AActor* GetMovementActor() const;

	static bool CheckRotationSafely(const FRotator& A, const FRotator& B, const float Tolerance = 0.9999f)
	{
		float YawA = FMath::Frac((A.Yaw + 360.0f) / 360.0f) * 360.0f;
		float YawB = FMath::Frac((B.Yaw + 360.0f) / 360.0f) * 360.0f;
		return (FMath::Abs(YawA - YawB) < Tolerance);
	};
	static bool CheckDistanceSafely(const FVector& A, const FVector& B, const float Tolerance = 0.9999f)
	{
		return (FVector::Dist2D(A, B) < Tolerance);
	};
	static bool CheckHeightSafely(const FVector& A, const FVector& B, const float Tolerance = 0.9999f)
	{
		return (FMath::Abs(A.Z - B.Z) < Tolerance);
	};
	static bool CheckDirectionSafely(const FVector& A, const FVector& B, const FVector& O)
	{
		FVector A2O = O - A;
		FVector B2O = O - B;
		return (FVector::DotProduct(A2O, B2O) < 0);
	};
protected:
	/** Should move but might not be moving currently*/
	bool bShouldDoAction;
	/** Is on moving, work during tick.*/
	bool bIsMoving;
	/** Is on rotating, work during tick.*/
	bool bIsRotating;
	/** Is on jumping, work during tick.*/
	bool bIsJumping;
	/** Is on shotting, work during tick.*/
	bool bIsShotting;
	/** Is on sliding, work during tick.*/
	bool bIsSliding;
	/** Is on falling, work during tick.*/
	bool bIsFalling;
};


/**
 * Movement base on multiple targets input. 
 */
UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkTargetMovement : public UXkMovement
{
	GENERATED_BODY()

	/** The max move point of hexagon in one turn.*/
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 ActionPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 MoveCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector CurrentMoveTarget;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingMoveTargets;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 RotateCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator CurrentRotateTarget;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingRotateTargets;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 JumpCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector CurrentJumpTarget;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingJumpTargets;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 ShotCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector CurrentShotTarget;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingShotTargets;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 SlideCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector CurrentSlideTarget;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PendingSlideTargets;
public:
	/** Move to target very fast mode.*/
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bBlinkMode;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bFailToGround;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float MoveAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float JumpAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ShotAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float SlideAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float JumpArc;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ShotArc;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float CapsuleHalfHeight;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementReachTargetEvent OnMovementReachTargetEvent;

	UFUNCTION(BlueprintCallable, Category = "Movement [KEVINTSUIXUGAMEDEV]")
	bool IsFailing() const { return bIsJumping || bIsFalling; };

	/** Default UObject constructor. */
	UXkTargetMovement(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin ActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End ActorComponent Interface

	//~ Begin UXkMovement Interface
	FORCEINLINE virtual void OnAction() override;
	//~ End UXkMovement Interface
	
	//~ Begin UXkTargetMovement Interface
	FORCEINLINE virtual void AddActiontPoint(const uint8 Input) { ActionPoint += Input; };
	FORCEINLINE virtual void ClearActionPoint() { ActionPoint = 0; };
	FORCEINLINE virtual uint8 GetActionPoint() const { return ActionPoint; };
	FORCEINLINE virtual void SetActionPoint(const uint8 Input) { ActionPoint = Input; };

	//~ Moving
	FORCEINLINE virtual void AddMoveTarget(const FVector& Target) { PendingMoveTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearMoveTargets() { PendingMoveTargets.Empty(); };
	FORCEINLINE virtual void SetMoveCost(const int32 Cost) { MoveCostPoint = Cost; };
	FORCEINLINE virtual void SetMoveAcceler(const float Acceler) { MoveAcceler = Acceler; };
	//~ Rotating
	FORCEINLINE virtual void AddRotateTarget(const FVector& Target) { PendingRotateTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearRotateTargets() { PendingRotateTargets.Empty(); };
	FORCEINLINE virtual void SetRotateCost(const int32 Cost) { RotateCostPoint = Cost; };
	//~ Jumping
	FORCEINLINE virtual void AddJumpTarget(const FVector& Target) { PendingJumpTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearJumpTargets() { PendingJumpTargets.Empty(); };
	FORCEINLINE virtual void SetJumpCost(const int32 Cost) { JumpCostPoint = Cost; };
	FORCEINLINE virtual void SetJumpArc(const float Arc) { JumpArc = Arc; };
	FORCEINLINE virtual void SetJumpAcceler(const float Acceler) { JumpAcceler = Acceler; };
	//~ Shotting
	FORCEINLINE virtual void AddShotTarget(const FVector& Target) { PendingShotTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearShotTargets() { PendingShotTargets.Empty(); };
	FORCEINLINE virtual void SetShotCost(const int32 Cost) { ShotCostPoint = Cost; };
	FORCEINLINE virtual void SetShotArc(const float Arc) { ShotArc = Arc; };
	FORCEINLINE virtual void SetShotAcceler(const float Acceler) { ShotAcceler = Acceler; };
	//~ Sliding
	FORCEINLINE virtual void AddSlideTarget(const FVector& Target) { PendingSlideTargets.Insert(Target, 0); };
	FORCEINLINE virtual void ClearSlideTargets() { PendingSlideTargets.Empty(); };
	FORCEINLINE virtual void SetSlideCost(const int32 Cost) { SlideCostPoint = Cost; };
	FORCEINLINE virtual void SetSlideAcceler(const float Acceler) { SlideAcceler = Acceler; };

	/** Valid movement targets base on current movement point.*/
	FORCEINLINE virtual TArray<FVector> GetValidMovementTargets() const;
	/** Final movement target base on current movement point.*/
	FORCEINLINE virtual FVector GetFinalMovementTarget() const;
	FORCEINLINE virtual FVector GetLineTraceLocation(const FVector& Input);
	//~ End UXkTargetMovement Interface

	static FVector CalcParaCurve(const FVector& Start, const FVector& End, const float CurveArc, const float CurveDist);
	static TArray<FVector> CalcParaCurvePoints(const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum);
	static bool CalcParaIntersection(const UObject* WorldContextObject, const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum, TArray<AActor*> IgnoreActors = TArray<AActor*>(), const ECollisionChannel TraceChannel = ECC_Visibility);

private:
	TOptional<FVector> LastTarget;
	TOptional<FVector> LastLocation;
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