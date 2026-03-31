// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "XkCharacter.generated.h"

#define THRESH_TARGET_ARE_NEAR 0.91f

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
	FORCEINLINE virtual bool ShouldDoAction() const { return bShouldDoAction; }
	FORCEINLINE virtual bool IsOnAction() const { return bIsMoving || bIsRotating || bIsJumping || bIsFlying || bIsSliding || bIsFalling || PendingTargets.Num() > 0; }
	FORCEINLINE virtual bool IsActionFinished() const { return PendingTargets.IsEmpty() && !IsOnAction() && bShouldDoAction; }
	FORCEINLINE virtual void OnAction() { bShouldDoAction = true; bIsMoving = bIsRotating = bIsJumping = bIsFlying = bIsSliding = false; };
	FORCEINLINE virtual void DoActionTick(const float DeltaTime) { bShouldDoAction = false; };
	FORCEINLINE virtual bool IsMoving() const { return bIsMoving; };
	FORCEINLINE virtual bool IsRotating() const { return bIsRotating; };
	FORCEINLINE virtual bool IsJumping() const { return bIsJumping; };
	FORCEINLINE virtual bool IsFlying() const { return bIsFlying; };
	FORCEINLINE virtual bool IsSliding() const { return bIsSliding; };
	FORCEINLINE virtual bool IsFalling() const { return bIsJumping ||bIsFalling; };
	FORCEINLINE virtual AActor* GetMovementActor() const;

	static bool CheckRotationSafely(const FRotator& A, const FRotator& B, const float Tolerance = THRESH_TARGET_ARE_NEAR)
	{
		float YawA = FMath::Frac((A.Yaw + 360.0f) / 360.0f) * 360.0f;
		float YawB = FMath::Frac((B.Yaw + 360.0f) / 360.0f) * 360.0f;
		return (FMath::Abs(YawA - YawB) < Tolerance);
	};
	static bool CheckDistance2DSafely(const FVector& A, const FVector& B, const float Tolerance = THRESH_TARGET_ARE_NEAR)
	{
		return (FVector::Dist2D(A, B) < Tolerance);
	};
	static bool CheckDistanceSafely(const FVector& A, const FVector& B, const float Tolerance = THRESH_TARGET_ARE_NEAR)
	{
		return (FVector::Dist(A, B) < Tolerance);
	};
	static bool CheckHeightSafely(const FVector& A, const FVector& B, const float Tolerance = THRESH_TARGET_ARE_NEAR)
	{
		return (FMath::Abs(A.Z - B.Z) < Tolerance);
	};
	static bool CheckDirectionSafely(const FVector& A, const FVector& B, const FVector& O, const float Tolerance = KINDA_SMALL_NUMBER)
	{
		FVector A2O = O - A;
		FVector B2O = O - B;
		return (FVector::DotProduct(A2O, B2O) < KINDA_SMALL_NUMBER);
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
	/** Is on flying, work during tick.*/
	bool bIsFlying;
	/** Is on sliding, work during tick.*/
	bool bIsSliding;
	/** Is on slide falling, work during tick.*/
	bool bIsSlideFalling;
	/** Is on falling, work during tick.*/
	bool bIsFalling;

	enum EActionType
	{
		None = 0,
		Move,
		Rotate,
		Jump,
		Fly,
		Slide,
		Fall
	};

	TArray<TPair<EActionType, FVector>> PendingTargets;
};


/**
 * Movement base on multiple targets input. 
 */
UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkTargetMovementComponent : public UXkMovement
{
	GENERATED_BODY()

	/** The max move point of hexagon in one turn.*/
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 ActionPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 MoveCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 RotateCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 JumpCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 FlyCostPoint;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 SlideCostPoint;

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
	float FlyAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float SlideAcceler;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float JumpArc;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float FlyArc;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float CapsuleRadius;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float CapsuleHalfHeight;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float MaxStepHeight;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementReachTargetEvent OnMovementReachTargetEvent;

	/** Default UObject constructor. */
	UXkTargetMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin ActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End ActorComponent Interface

	//~ Begin UXkMovement Interface
	virtual void OnAction() override;
	virtual void DoActionTick(const float DeltaTime) override;
	//~ End UXkMovement Interface
	
	//~ Begin UXkTargetMovementComponent Interface
	FORCEINLINE virtual void AddActiontPoint(const uint8 Input) { ActionPoint += Input; };
	FORCEINLINE virtual void ClearActionPoint() { ActionPoint = 0; };
	FORCEINLINE virtual uint8 GetActionPoint() const { return ActionPoint; };
	FORCEINLINE virtual void SetActionPoint(const uint8 Input) { ActionPoint = Input; };
	FORCEINLINE virtual void ClearActionTargets() { PendingTargets.Empty(); };

	//~ Moving
	FORCEINLINE virtual void AddMoveTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Move, Target), 0); };
	FORCEINLINE virtual void SetMoveCost(const int32 Cost) { MoveCostPoint = Cost; };
	FORCEINLINE virtual void SetMoveAcceler(const float Acceler) { MoveAcceler = Acceler; };
	//~ Rotating
	FORCEINLINE virtual void AddRotateTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Rotate, Target), 0); };
	FORCEINLINE virtual void SetRotateCost(const int32 Cost) { RotateCostPoint = Cost; };
	//~ Jumping
	FORCEINLINE virtual void AddJumpTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Jump, Target), 0); };
	FORCEINLINE virtual void SetJumpCost(const int32 Cost) { JumpCostPoint = Cost; };
	FORCEINLINE virtual void SetJumpArc(const float Arc) { JumpArc = Arc; };
	FORCEINLINE virtual void SetJumpAcceler(const float Acceler) { JumpAcceler = Acceler; };
	//~ Flying
	FORCEINLINE virtual void AddFlyTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Fly, Target), 0); };
	FORCEINLINE virtual void SetFlyCost(const int32 Cost) { FlyCostPoint = Cost; };
	FORCEINLINE virtual void SetFlyArc(const float Arc) { FlyArc = Arc; };
	FORCEINLINE virtual void SetFlyAcceler(const float Acceler) { FlyAcceler = Acceler; };
	//~ Sliding
	FORCEINLINE virtual void AddSlideTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Slide, Target), 0); };
	FORCEINLINE virtual void SetSlideCost(const int32 Cost) { SlideCostPoint = Cost; };
	FORCEINLINE virtual void SetSlideAcceler(const float Acceler) { SlideAcceler = Acceler; };

	virtual FVector GetMovementActorCenter() const;
	virtual float GetMovementActorHeight() const;
	virtual FVector GetLineTraceLocation(const FVector& Input, const ECollisionChannel Channel = ECC_Pawn, const bool bTraceComplex = false, const bool bTraceUnderFoots = true);
	virtual AActor* GetLineTraceActor(const FVector& Input, const ECollisionChannel Channel = ECC_Pawn, const bool bTraceComplex = false, const bool bTraceUnderFoots = true);
	virtual FVector GetSphereTraceLocation(const FVector& Input, const ECollisionChannel Channel = ECC_Pawn, const bool bTraceComplex = false);
	virtual void ValidateOnGround();
	//~ End UXkTargetMovementComponent Interface

	static FVector CalcParaCurve(const FVector& Start, const FVector& End, const float CurveArc, const float CurveDist);
	static TArray<FVector> CalcParaCurvePoints(const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum);

private:
	TOptional<FVector> LastTarget;
	TOptional<FVector> LastLocation;
};


/**
 * Movement base on spline input
 */
UCLASS(Blueprintable)
class XKGAMEDEVCORE_API UXkSplineMovementComponent : public UXkMovement
{
	GENERATED_BODY()

public:
	/** Default UObject constructor. */
	UXkSplineMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(Category= "Character [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UXkTargetMovementComponent> TargetMovement;

	UPROPERTY(Category= "Character [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UXkBuoyancyComponent> BuoyancyComponent;
public:
	/** Default UObject constructor. */
	AXkCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame.
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UFUNCTION(Category = "Character [KEVINTSUIXUGAMEDEV]", BlueprintCallable, meta = (BlueprintThreadSafe))
	virtual bool IsCharacterFalling() const;

	UFUNCTION(Category = "Character [KEVINTSUIXUGAMEDEV]", BlueprintCallable, meta = (BlueprintThreadSafe))
	virtual bool IsCharacterMoving() const;

	UFUNCTION(Category = "Character [KEVINTSUIXUGAMEDEV]", BlueprintCallable, meta = (BlueprintThreadSafe))
	virtual FVector GetCharacterVelocity() const;

	UFUNCTION(Category = "Character [KEVINTSUIXUGAMEDEV]", BlueprintCallable, meta = (BlueprintThreadSafe))
	virtual FVector GetCharacterAcceleration() const;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {};
	
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit) {};

	FORCEINLINE UXkTargetMovementComponent* GetXkTargetMovement() const { return TargetMovement; }
	FORCEINLINE UXkBuoyancyComponent* GetXkBuoyancyComponent() const { return BuoyancyComponent; }
};