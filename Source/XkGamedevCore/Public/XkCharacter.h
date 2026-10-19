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

	/** Max deceleration used when braking to a target (e.g. Slide).
	 * Stop time T = sqrt(2*D / MaxDeceleration). Smaller value means longer brake. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxDeceleration;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator RotationRate;

	/** Yaw tolerance in degrees for bHeavyMove. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ClampMax = "45", UIMax = "45"))
	float OrientTolerance;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementBeginEvent OnMovementBeginEvent;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementFinishEvent OnMovementFinishEvent;

	/** One-shot C++-only finish callback. Supplied via OnAction's optional
	 *  parameter and fires exactly once when IsActionFinished() turns true
	 *  (same tick OnMovementFinishEvent broadcasts), then cleared.
	 *  Not UPROPERTY on purpose: TFunction cannot be reflected and this
	 *  slot is intentionally C++-only. */
	TFunction<void()> PendingFinishCallback;
public:
	FORCEINLINE virtual bool ShouldDoAction() const { return bShouldDoAction; }
	FORCEINLINE virtual bool IsOnAction() const { return bIsMoving || bIsRotating || bIsJumping || bIsFlying || bIsSliding || bIsFalling || PendingTargets.Num() > 0; }
	FORCEINLINE virtual bool IsActionFinished() const { return PendingTargets.IsEmpty() && !IsOnAction() && bShouldDoAction; }
	/** Start executing queued targets. The optional InFinishCallback (default empty)
	 *  is invoked exactly once when all pending targets complete, right after
	 *  OnMovementFinishEvent broadcasts, then cleared. Supports lambdas. */
	FORCEINLINE virtual void OnAction(TFunction<void()> InFinishCallback = TFunction<void()>())
	{
		bShouldDoAction = true;
		bIsMoving = bIsRotating = bIsJumping = bIsFlying = bIsSliding = false;
		PendingFinishCallback = MoveTemp(InFinishCallback);
	};
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

	/** Finished action names since OnAction, used by xk.Movement.DebugInfo. */
	TArray<FString> DebugFinishedActions;
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

	/** Finish orienting toward move direction before translating. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bHeavyMove;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bFailToGround;

	/** If true, GetSnapLocation / GetSnapActor ignore every APawn in
	 *  the world during the downward sweep. Useful for non-pawn actors
	 *  (e.g. props, resources, fortresses) that must snap to the real ground
	 *  under them, not to a character capsule standing on the same hex. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIgnoreOnlyPawn;

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

	/** Z distance from the Pivot (actor origin) DOWN to the foot (bottom
	 * contact point) of the owning actor. Used directly by GetLineTraceLocation
	 * to convert ground impact Z into actor-origin Z:
	 *     ReturnZ = GroundImpact.Z + CapsuleHalfHeight
	 * so ground snapping works for ANY actor, not just characters whose
	 * capsule center == actor origin. Despite the historical name, this field
	 * is semantically "pivot-to-foot distance", not strictly a capsule
	 * dimension. It also controls the sweep sphere context used by Snap.
	 * Typical values:
	 *   - ACharacter-style capsule (pivot at capsule center): scaled capsule half height (e.g. 96).
	 *   - Mesh with pivot at the bottom (foot): 0.
	 *   - Mesh with pivot at the model center of height H: H/2.
	 *   - Pivot above the model top: negative value.
	 * AXkCharacter auto-syncs this to GetScaledCapsuleHalfHeight() in its
	 * constructor / OnConstruction. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float CapsuleHalfHeight;

	/** Minimum foot (capsule bottom) world Z used by GetLineTraceLocation. If the trace ground
	 * is lower than this value, the returned Actor Z is clamped so the capsule bottom stays
	 * at GroundMinHeight. Set to the water horizon height (e.g. HORIZION_HEIGHT) while the
	 * character floats on water. Default 0. */
	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float GroundMinHeight;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", BlueprintAssignable, meta = (AllowPrivateAccess = "true"))
	FOnMovementReachTargetEvent OnMovementReachTargetEvent;

	/** Default UObject constructor. */
	UXkTargetMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin ActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End ActorComponent Interface

	//~ Begin UXkMovement Interface
	virtual void OnAction(TFunction<void()> InFinishCallback = TFunction<void()>()) override;
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
	/** Target must be the actor-origin world position (same space as GetActorLocation()).
	 *  Use GetSnapLocation() to convert a ground/surface Z to actor-origin Z
	 *  before passing it here. */
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
	/** Target must be the actor-origin world position (same space as GetActorLocation()).
	 *  Use GetSnapLocation() to convert a ground/surface Z to actor-origin Z
	 *  before passing it here. */
	FORCEINLINE virtual void AddSlideTarget(const FVector& Target) { PendingTargets.Insert(TPair<EActionType, FVector>(EActionType::Slide, Target), 0); };
	FORCEINLINE virtual void SetSlideCost(const int32 Cost) { SlideCostPoint = Cost; };
	FORCEINLINE virtual void SetSlideAcceler(const float Acceler) { SlideAcceler = Acceler; };
	FORCEINLINE virtual void SetMaxDeceleration(const float Deceler) { MaxDeceleration = Deceler; };

	/**
	 * Sphere-sweeps straight down from Input and returns a position that
	 * places the ACTOR ORIGIN on the detected ground, i.e.
	 *     Return.Z == GroundImpact.Z + CapsuleHalfHeight
	 * where CapsuleHalfHeight is the vertical distance from the actor origin down
	 * to the foot (bottom contact point).
	 *
	 * SWEEP SHAPE (current behavior):
	 *  - The sweep ALWAYS starts at the FOOT position
	 *        FootStart.Z = Input.Z - CapsuleHalfHeight
	 *    and goes straight down. It never starts from a huge Z above the
	 *    world, so a ceiling / overhanging platform above the actor can
	 *    never be chosen as the ground.
	 *  - If the first sweep misses (e.g. the foot is embedded in geometry),
	 *    the implementation retries up to 10 times, each time lifting the
	 *    start by (EffectiveRescue / 10). EffectiveRescue equals
	 *    CapsuleHalfHeight when it is non-zero; otherwise (pivot already at
	 *    the foot, e.g. a Cube) it falls back to CapsuleRadius so bottom-pivot
	 *    actors still get a usable upward rescue range. The 10th retry starts
	 *    at FootStart + EffectiveRescue (actor origin for ACharacter,
	 *    foot + CapsuleRadius for a bottom-pivot Cube).
	 *  - This matches the "stand on whatever is directly below me" intent
	 *    used by Move / Jump / Slide snaps.
	 *
	 * COORDINATE SPACE (critical, has caused ragdoll jitter bugs):
	 *  - Input is treated as an ACTOR-ORIGIN world position (same space as
	 *    GetActorLocation()), NOT a foot/surface position.
	 *  - Return value is also in ACTOR-ORIGIN space, ready to pass directly to
	 *    SetActorLocation(). Do NOT subtract CapsuleHalfHeight again.
	 *  - If a caller stores "target points" as foot/surface Z (e.g. a hex-tile
	 *    center on the ground), you MUST normalize them through GetSnapLocation()
	 *    once before using them as a destination, otherwise vertical interpolation
	 *    will drag the actor origin down to foot height and the next ground-snap
	 *    tick will SnapUp by ~CapsuleHalfHeight, violently jolting any active
	 *    ragdoll. See the Slide/Jump branches for the normalization pattern.
	 *
	 * NON-CHARACTER ACTORS:
	 *  - CapsuleHalfHeight is the single knob that controls origin-vs-foot conversion.
	 *    Despite the name, treat it as "pivot-to-foot distance" and set it
	 *    according to the mesh pivot placement:
	 *      - Capsule-centered pivot (ACharacter default):  scaled capsule half height (e.g. 96).
	 *      - Pivot at the mesh bottom (foot):              0.
	 *      - Pivot at the model center of height H:        H/2.
	 *      - Pivot above the model top:                    negative value.
	 *
	 * PARAMETERS:
	 *  @param Input              Query position in ACTOR-ORIGIN space.
	 *  @param Channel            Collision channel to sweep against (default ECC_Pawn).
	 *  @param bTraceComplex      Use per-poly collision. Keep false for cheap/stable
	 *                            ground snapping; set true only when you really need
	 *                            precise contact with complex meshes.
	 *  @param bTraceUnderInput   Controls the sweep start point:
	 *                            - true  (default): sweep starts at the FOOT position
	 *                              (Input.Z - CapsuleHalfHeight) and retries upward.
	 *                              Use this for normal ground-snapping (Move/Jump/Slide).
	 *                            - false: sweep starts from MAX_Z straight down and
	 *                              returns the HIGHEST surface hit at this XY.
	 *                              Use this when you need the topmost geometry
	 *                              (e.g. floating a popup above stacked objects).
	 *
	 * EDGE CASES:
	 *  - If no hit is found, the function returns an Input-based fallback (see cpp).
	 *  - The return Z is clamped so capsule-bottom >= GroundMinHeight (e.g. water
	 *    surface), so sliding over gaps / water will "float" at GroundMinHeight
	 *    instead of falling through.
	 */
	/** @see full doc above. bTraceUnderInput=true: foot-down sweep (normal snap).
	 *  bTraceUnderInput=false: MAX_Z-down sweep, returns highest surface. */
	virtual FVector GetSnapLocation(const FVector& Input, const ECollisionChannel Channel = ECC_Pawn, const bool bTraceComplex = true, const bool bTraceUnderInput = true);
	virtual AActor* GetSnapActor(const FVector& Input, const ECollisionChannel Channel = ECC_Pawn, const bool bTraceComplex = true, const bool bTraceUnderInput = true);
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
	virtual void OnConstruction(const FTransform& Transform) override;
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