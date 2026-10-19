// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkCharacter.h"
#include "XkController.h"
#include "XkLandscape/XkBuoyancy.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

static TAutoConsoleVariable<bool> CVarXkMovementDebugInfo(
	TEXT("r.xk.Movement.DebugInfo"),
	false,
	TEXT("Print Velocity and pending action stack of UXkTargetMovementComponent every tick."),
	ECVF_Default);

UE_DISABLE_OPTIMIZATION

UXkMovement::UXkMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldDoAction = false;
	bIsMoving = false;
	bIsRotating = false;
	MaxVelocity = 500.0;
	MaxAcceleration = 2048.0;
	MaxDeceleration = 2048.0;
	RotationRate = FRotator(0.0, 500.0, 0.0);
}

AActor* UXkMovement::GetMovementActor() const
{
	AActor* Actor = GetOwner();
	if (Actor && IsValid(Actor))
	{
		return Actor;
	}
	return nullptr;
}

UXkTargetMovementComponent::UXkTargetMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bBlinkMode = false;
	bHeavyMove = false;
	OrientTolerance = 5.0f;
	bFailToGround = false;
	bIgnoreOnlyPawn = false;
	MoveAcceler = 1.0f;
	JumpAcceler = 1.0f;
	FlyAcceler = 1.0f;
	SlideAcceler = 1.0f;
	JumpArc = -0.5;
	FlyArc = -0.1;
	CapsuleRadius = 55.0f;
	// Z distance from the Pivot (actor origin) DOWN to the foot (bottom contact).
	// Default assumes an ACharacter-style capsule where the pivot sits at the
	// capsule center, so pivot-to-foot equals the historical "half height" (96).
	// Non-character actors with a bottom pivot should set this to 0 in BP / child class.
	CapsuleHalfHeight = 96.0f;
	GroundMinHeight = 0.0f;

	// Set default values
	ActionPoint = 0;
	MoveCostPoint = 1;
	RotateCostPoint = 0;
	JumpCostPoint = 2;
	SlideCostPoint = 0;
	FlyCostPoint = 0;

	// Activate ticking in order to update the cursor every frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXkTargetMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPED_NAMED_EVENT(UXkMovement_TickComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UXkMovement_TickComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_UXkMovement_TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If the movement is not active, do nothing
	if (!IsActive())
	{
		return;
	}

	check(GetMovementActor());

	DoActionTick(DeltaTime);

	// Debug info
	if (CVarXkMovementDebugInfo.GetValueOnGameThread())
	{
		static const TCHAR* ActionNames[] = { TEXT("none"), TEXT("move"), TEXT("rotate"), TEXT("jump"), TEXT("fly"), TEXT("slide"), TEXT("fall") };
		TArray<FString> Items;
		for (const FString& Item : DebugFinishedActions)
		{
			Items.Add(Item + TEXT("(finish)"));
		}
		for (int32 Index = PendingTargets.Num() - 1; Index >= 0; --Index)
		{
			FString Item = ActionNames[static_cast<int32>(PendingTargets[Index].Key)];
			if (Index == PendingTargets.Num() - 1)
			{
				Item += TEXT("(current)");
			}
			Items.Add(Item);
		}
		const FString Stack = Items.Num() > 0 ? FString::Join(Items, TEXT("->")) : FString(TEXT("empty"));
		UE_LOG(LogTemp, Display, TEXT("[XkMovement] %s Velocity=(%.1f, %.1f, %.1f) Speed=%.1f Stack: %s"),
			*GetNameSafe(GetMovementActor()), Velocity.X, Velocity.Y, Velocity.Z, Velocity.Size(), *Stack);
		if (GEngine)
		{
			FString DebugMessage = FString::Printf(TEXT("[XkMovement] %s Velocity=(%.1f, %.1f, %.1f) Speed=%.1f Stack: %s"),
				*GetNameSafe(GetMovementActor()), Velocity.X, Velocity.Y, Velocity.Z, Velocity.Size(), *Stack);
			GEngine->AddOnScreenDebugMessage(/*Key*/ static_cast<uint64>(GetUniqueID()), /*Time*/ 5.0f, FColor::Green, DebugMessage);
		}
	}

	// Whether every action finished?
	if (IsActionFinished())
	{
		bShouldDoAction = false;
		MoveAcceler = JumpAcceler = FlyAcceler = SlideAcceler = 1.0f;
		OnMovementFinishEvent.Broadcast();
		OnMovementReachTargetEvent.Broadcast(ActionPoint);
		// Fire and consume the one-shot C++-only finish callback supplied
		// through OnAction's optional parameter. Move it to a local first
		// so a re-entrant OnAction(nextCallback) call from inside the
		// callback (chaining the next movement) is preserved.
		if (PendingFinishCallback)
		{
			TFunction<void()> LocalCallback = MoveTemp(PendingFinishCallback);
			PendingFinishCallback = nullptr;
			LocalCallback();
		}
		ClearActionPoint();
		ClearActionTargets();
	}
}

void UXkTargetMovementComponent::OnAction(TFunction<void()> InFinishCallback)
{
	if (AActor* MovingActor = GetMovementActor())
	{
		bShouldDoAction = true;
		DebugFinishedActions.Reset();
		LastTarget = MovingActor->GetActorLocation();
		bIsMoving = bIsRotating = bIsJumping = bIsSliding = bIsFalling = bIsFlying = false;
		PendingFinishCallback = MoveTemp(InFinishCallback);
		OnMovementBeginEvent.Broadcast();
	}
}

void UXkTargetMovementComponent::DoActionTick(const float DeltaTime)
{
	if (PendingTargets.Num() > 0)
	{
		TPair<EActionType, FVector> CurrentTarget = PendingTargets.Last();
		FVector Location = GetMovementActor()->GetActorLocation();
		FRotator Rotation = GetMovementActor()->GetActorRotation();
		FVector TargetLocation = CurrentTarget.Value;
		FRotator TargetRotation = FRotationMatrix::MakeFromX(TargetLocation - Location).Rotator();
		if (CurrentTarget.Key == EActionType::Move)
		{
			/*- DEBUG CODE -*/
			//FString Message = FString::Printf(TEXT("Velocity (%0.01f)"), Velocity.Size());
			//GEngine->AddOnScreenDebugMessage(-1, 0.5, FColor::Red, *Message);

			if (CheckDistance2DSafely(Location, TargetLocation))
			{
				// Closed enough, stop moving
				bIsMoving = false;
				LastTarget = Location;
				DebugFinishedActions.Add(TEXT("move"));
				PendingTargets.Pop(true /* Shrink*/);
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (!bIsMoving && ActionPoint < MoveCostPoint)
			{
				// Skip this target if no enough AP
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsMoving)
			{
				bIsMoving = true;
				// decrease MovementPoint count
				ActionPoint -= MoveCostPoint;
				// broadcast current action point of last target reached, the action point was decreased when start to move to that target
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			/////////////////////////////////////////////////////////////
			// If not closed to target, move
			else if (bIsMoving)
			{
				FVector TargetVector = FVector(TargetLocation.X, TargetLocation.Y, Location.Z);
				FVector StartVector = Location;
				FVector MovingDir = (TargetVector - StartVector);
				MovingDir.Normalize();

				const FVector X = TargetVector - StartVector;
				FRotator StartRotator = GetMovementActor()->GetActorRotation();
				FRotator TargetRotator = FRotationMatrix::MakeFromX(X).Rotator();

				// Orient toward move direction before translating.
				if (bHeavyMove && !CheckRotationSafely(StartRotator, TargetRotator, OrientTolerance))
				{
					bIsRotating = true;
					FRotator OnlyYawTarget = TargetRotator;
					OnlyYawTarget.Pitch = StartRotator.Pitch;
					OnlyYawTarget.Roll = StartRotator.Roll;
					FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, OnlyYawTarget, DeltaTime, RotationRate.Yaw) :
						FMath::RInterpConstantTo(StartRotator, OnlyYawTarget, DeltaTime, RotationRate.Yaw);
					GetMovementActor()->SetActorRotation(NewRotator);
					float CurrentVelocity = Velocity.Size();
					CurrentVelocity += MaxAcceleration * DeltaTime;
					CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity);
					Velocity = OnlyYawTarget.Vector() * CurrentVelocity;
					Acceleration = OnlyYawTarget.Vector() * MaxAcceleration;
					LastLocation = GetMovementActor()->GetActorLocation();
					return;
				}
				bIsRotating = false;

				float CurrentVelocity = Velocity.Size();
				float CurrentAcceleration = MaxAcceleration * MoveAcceler;
				CurrentVelocity += CurrentAcceleration * DeltaTime;
				CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * MoveAcceler);
				FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
					FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);

				/*- DEBUG CODE -*/
				//float Dist = FVector::Dist2D(NewLocation, Location);
				//FString Message = FString::Printf(TEXT("Current Velocity (%0.01f)  Dist (%0.01f)"), CurrentVelocity, Dist);
				//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Red, *Message);

				// Snap to ground
				NewLocation = GetSnapLocation(NewLocation);
				Velocity = CurrentVelocity * MovingDir;
				Acceleration = CurrentAcceleration * MovingDir;
				LastLocation = GetMovementActor()->GetActorLocation();
				GetMovementActor()->SetActorLocation(NewLocation);

				FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
					FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
				GetMovementActor()->SetActorRotation(NewRotator);
			}
		}
		else if (CurrentTarget.Key == EActionType::Rotate)
		{
			if (CheckRotationSafely(Rotation, TargetRotation))
			{
				// Closed enough, stop moving
				bIsRotating = false;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
				DebugFinishedActions.Add(TEXT("rotate"));
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsRotating && ActionPoint < RotateCostPoint)
			{
				// Skip this target if no enough AP
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsRotating)
			{
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
				bIsRotating = true;
			}
			else if (bIsRotating)
			{
				FRotator StartRotator = GetMovementActor()->GetActorRotation();
				FRotator TargetRotator = TargetRotation;
				// Only interpolate Yaw, keep Pitch and Roll unchanged
				TargetRotator.Pitch = StartRotator.Pitch; TargetRotator.Roll = StartRotator.Roll;
				FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
					FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
				GetMovementActor()->SetActorRotation(NewRotator);

				float CurrentVelocity = Velocity.Size();
				CurrentVelocity += MaxAcceleration * DeltaTime;
				CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity);
				Velocity = TargetRotator.Vector() * CurrentVelocity;
				Acceleration = TargetRotator.Vector() * MaxAcceleration;
			}
		}
		else if (CurrentTarget.Key == EActionType::Jump)
		{
			// Target is already in actor-origin space (same as GetActorLocation()).
			// Callers must pass the actor-origin Z directly (e.g. via GetSnapLocation).
			if (CheckDistance2DSafely(Location, TargetLocation))
			{
				bIsJumping = false;
				LastTarget = TargetLocation;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
				DebugFinishedActions.Add(TEXT("jump"));
				PendingTargets.Pop(true /* Shrink*/);
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (!bIsJumping && ActionPoint < JumpCostPoint)
			{
				// Skip this target if no enough AP
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsJumping)
			{
				bIsJumping = true;
				// decrease ActionPoint count
				ActionPoint -= JumpCostPoint;
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (bIsJumping)
			{
				FVector TargetVector = FVector(TargetLocation.X, TargetLocation.Y, Location.Z);
				FVector StartVector = Location;
				FVector MovingDir = (TargetVector - StartVector);
				MovingDir.Normalize();
				float CurrentVelocity = Velocity.Size();
				float CurrentAcceleration = MaxAcceleration;
				CurrentVelocity += CurrentAcceleration * DeltaTime;
				CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * JumpAcceler);
				FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
					FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
				// Calculate the height of jump
				if (LastTarget.IsSet())
				{
					float CurrLength = FVector::Dist2D(NewLocation, LastTarget.GetValue());
					FVector CurrLocation = CalcParaCurve(LastTarget.GetValue(), TargetLocation, JumpArc, CurrLength);
					NewLocation.Z = CurrLocation.Z;
				}
				Velocity = (NewLocation - Location) / DeltaTime;
				Acceleration = CurrentAcceleration * MovingDir;
				LastLocation = GetMovementActor()->GetActorLocation();
				GetMovementActor()->SetActorLocation(NewLocation);

				const FVector X = TargetVector - StartVector;
				FRotator StartRotator = GetMovementActor()->GetActorRotation();
				FRotator TargetRotator = FRotationMatrix::MakeFromX(X).Rotator();
				FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
					FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
				GetMovementActor()->SetActorRotation(NewRotator);
			}
		}
		else if (CurrentTarget.Key == EActionType::Slide)
		{
			// Target is already in actor-origin space (same as GetActorLocation()).
			// Callers must pass the actor-origin Z directly (e.g. via GetSnapLocation).
			if (CheckDistanceSafely(Location, TargetLocation))
			{
				// Closed enough, stop moving
				bIsSliding = false;
				LastTarget = TargetLocation;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
				DebugFinishedActions.Add(TEXT("slide"));
				PendingTargets.Pop(true /* Shrink*/);
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (!bIsSliding && ActionPoint < SlideCostPoint)
			{
				// Skip this target if no enough AP
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsSliding)
			{
				bIsSliding = true;
				// decrease ActionPoint count
				ActionPoint -= SlideCostPoint;
				OnMovementReachTargetEvent.Broadcast(ActionPoint);

				// Uniform deceleration: initial speed v0 = sqrt(2 * a * D) so the
				// actor comes to a stop exactly at the target. Horizontal slide uses
				// MaxDeceleration; a pure vertical slide uses gravity (980 cm/s^2)
				// so the fall feels natural. Both are allowed to exceed MaxVelocity.
				// If the slide starts already aligned on XY (pure vertical slide),
				// seed the initial velocity along Z using the Z distance; otherwise
				// seed it along the horizontal direction using the 2D distance.
				if (CheckDistance2DSafely(Location, TargetLocation))
				{
					// Vertical slide: SlideAcceler scales the initial speed so a
					// high value produces a strong impact feel (e.g. blown upward).
					// The per-frame speed is still capped to sqrt(2*a*D_left) so
					// the actor always stops exactly at the target.
					FVector SlideDir(0.0f, 0.0f, FMath::Sign(TargetLocation.Z - Location.Z));
					const float DistLeft = FMath::Abs(TargetLocation.Z - Location.Z);
					const float InitialSpeed = FMath::Sqrt(2.0f * MaxDeceleration * SlideAcceler * DistLeft);
					Velocity = SlideDir * InitialSpeed;
				}
				else
				{
					FVector SlideDir = (TargetLocation - Location);
					SlideDir.Z = 0.0f;
					SlideDir.Normalize();
					const float DistLeft = FVector::Dist(FVector(Location.X, Location.Y, 0.0f), FVector(TargetLocation.X, TargetLocation.Y, 0.0f));
					const float InitialSpeed = FMath::Sqrt(2.0f * MaxDeceleration * DistLeft);
					Velocity = SlideDir * InitialSpeed;
				}
			}
			else if (bIsSliding)
			{
				FVector TargetVector = TargetLocation;
				FVector StartVector = Location;

				// Vertical slide to target: uses GRAVITY (980 cm/s^2) as the
				// deceleration so the vertical fall feels natural, independent
				// from the horizontal MaxDeceleration tuning. Each frame the
				// speed is capped to the exact stopping speed for the remaining
				// Z distance (v = sqrt(2*g*D_left)), so velocity reaches zero
				// right at the target without overshoot. After the slide pops,
				// the end-of-tick FallToGround fallback picks up and drives the
				// actor back to real ground via gravity if the target was airborne.
				if (CheckDistance2DSafely(Location, TargetVector))
				{
					const float DzSigned = TargetVector.Z - StartVector.Z;
					const float RemainingDist = FMath::Abs(DzSigned);
					FVector MovingDir(0.0f, 0.0f, FMath::Sign(DzSigned));
					const float CurrentVelocity = FMath::Sqrt(2.0f * MaxDeceleration * SlideAcceler * RemainingDist);

					FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
						FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
					// Clamp if we overshoot past the target along the move direction.
					if ((DzSigned > 0.0f && NewLocation.Z > TargetVector.Z) ||
						(DzSigned < 0.0f && NewLocation.Z < TargetVector.Z))
					{
						NewLocation = TargetVector;
					}
					Velocity = (NewLocation - Location) / DeltaTime;
					Acceleration = -MaxDeceleration * MovingDir;
					LastLocation = GetMovementActor()->GetActorLocation();
					GetMovementActor()->SetActorLocation(NewLocation);
				}
				// Horizontal slide to target: ramps down to zero speed exactly at
				// the target. Each frame the speed is capped to the exact stopping
				// speed for the remaining distance, so it never overshoots and the
				// velocity reaches zero right at the target (no snap, no jump).
				else
				{
					TargetVector.Z = Location.Z;
					FVector MovingDir = (TargetVector - StartVector).GetSafeNormal();
					const float RemainingDist = FVector::Dist(StartVector, TargetVector);
					// Exact speed that decelerates to zero over RemainingDist using
					// MaxDeceleration. v = sqrt(2*a*D_left) makes velocity reach
					// zero right at the target, so the slide eases in and stops smoothly.
					const float CurrentVelocity = FMath::Sqrt(2.0f * MaxDeceleration * RemainingDist);

					FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
						FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
					// Hug the ground every frame so the FallToGround post-step
					// below has nothing to snap, which avoids the ragdoll twitch
					// at the end of a slide. Trace from well above the actor
					// (bTraceUnderInput = false) so the sweep never starts inside
					// nearby geometry; GetSnapLocation already adds
					// OriginToFoot so the returned Z is the correct actor
					// origin that sits on the ground.
					if (bFailToGround)
					{
						NewLocation = GetSnapLocation(NewLocation, ECollisionChannel::ECC_Pawn, true, false);
					}
					Velocity = (NewLocation - Location) / DeltaTime;
					Acceleration = -MaxDeceleration * MovingDir;
					LastLocation = GetMovementActor()->GetActorLocation();
					GetMovementActor()->SetActorLocation(NewLocation);
				}
			}
		}
		else if (CurrentTarget.Key == EActionType::Fly)
		{
			if (CheckDistance2DSafely(Location, TargetLocation))
			{
				bIsFlying = false;
				LastTarget = TargetLocation;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
				DebugFinishedActions.Add(TEXT("fly"));
				PendingTargets.Pop(true /* Shrink*/);
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (!bIsFlying && ActionPoint < FlyCostPoint)
			{
				// Skip this target if no enough AP
				PendingTargets.Pop(true /* Shrink*/);
			}
			else if (!bIsFlying)
			{
				bIsFlying = true;
				// decrease ActionPoint count
				ActionPoint -= FlyCostPoint;
				Velocity = (TargetLocation - Location).GetSafeNormal() * MaxVelocity;
				Acceleration = FVector(MaxAcceleration);
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
			}
			else if (bIsFlying)
			{
				FVector TargetVector = FVector(TargetLocation.X, TargetLocation.Y, Location.Z);
				FVector StartVector = Location;
				FVector MovingDir = (TargetVector - StartVector);
				MovingDir.Normalize();
				float CurrentVelocity = Velocity.Size();
				float CurrentAcceleration = MaxAcceleration * FlyAcceler;
				CurrentVelocity += CurrentAcceleration * DeltaTime;
				CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * FlyAcceler);
				FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
					FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
				// Calculate the height of shot
				if (LastTarget.IsSet())
				{
					float CurrLength = FVector::Dist2D(NewLocation, LastTarget.GetValue());
					FVector CurrLocation = CalcParaCurve(LastTarget.GetValue(), TargetLocation, FlyArc, CurrLength);
					NewLocation.Z = CurrLocation.Z;
				}
				Velocity = (NewLocation - Location) / DeltaTime;
				Acceleration = CurrentAcceleration * MovingDir;
				LastLocation = GetMovementActor()->GetActorLocation();
				GetMovementActor()->SetActorLocation(NewLocation);

				if (LastLocation.IsSet())
				{
					FVector NewDir = NewLocation - LastLocation.GetValue();
					NewDir.Normalize();
					GetMovementActor()->SetActorRotation(NewDir.ToOrientationRotator());
				}
				LastLocation = NewLocation;
			}
		}
	}

	if (bFailToGround && PendingTargets.IsEmpty() && !bIsSliding && !bIsJumping && !bIsFlying)
	{
		// Snap to ground
		FVector ActorLocation = GetMovementActor()->GetActorLocation();
		FVector TargetLocation = GetSnapLocation(ActorLocation, ECollisionChannel::ECC_Pawn, true);
		FVector NewLocation = ActorLocation;
		if (CheckHeightSafely(ActorLocation, TargetLocation))
		{
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			bIsFalling = false;
		}
		else if (TargetLocation.Z > ActorLocation.Z)
		{
			// Snap to target if target is higher than current location
			NewLocation = TargetLocation;
			bIsFalling = true;
		}
		else
		{
			// Falling to target if target is lower than current location
			const float Gravity = -980.0f; // Unreal Engine unit cm/s^2
			FVector GravityAcceleration(0.0f, 0.0f, Gravity);
			Acceleration = GravityAcceleration;
			Velocity += Acceleration * DeltaTime;
			NewLocation = ActorLocation + Velocity * DeltaTime + 0.5f * Acceleration * DeltaTime * DeltaTime;
			bIsFalling = true;
		}
		LastLocation = GetMovementActor()->GetActorLocation();
		GetMovementActor()->SetActorLocation(NewLocation);
	}
}

FVector UXkTargetMovementComponent::GetSnapLocation(const FVector& Input, const ECollisionChannel Channel, const bool bTraceComplex, const bool bTraceUnderInput)
{
	// bTraceUnderInput=true (default): foot-down sweep, finds the ground directly
	// below the actor. Retries upward up to 10 times if the foot is embedded.
	// bTraceUnderInput=false: MAX_Z-down sweep, returns the HIGHEST surface at
	// this XY (e.g. for floating a popup above stacked geometry).
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	// Optionally ignore every pawn so prop-like actors do not snap onto a
	// character capsule that happens to stand on the same hex.
	if (bIgnoreOnlyPawn)
	{
		for (TActorIterator<APawn> It(GetWorld()); It; ++It)
		{
			CollisionParams.AddIgnoredActor(*It);
		}
	}
	CollisionParams.bTraceComplex = bTraceComplex;
	const float TraceRadius = CapsuleRadius * 0.25f;
	// GroundMinHeight is interpreted as the minimum FOOT (bottom contact) world Z.
	// Convert it to the equivalent actor-origin minimum by adding CapsuleHalfHeight.
	const float MinOriginZ = GroundMinHeight + CapsuleHalfHeight;

	if (!bTraceUnderInput)
	{
		// MAX_Z sweep: find the topmost surface at this XY position.
		const FVector Start = FVector(Input.X, Input.Y, UE_FLOAT_HUGE_DISTANCE);
		const FVector End   = FVector(Input.X, Input.Y, -UE_FLOAT_HUGE_DISTANCE);
		if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
		{
			float HeightZ = HitResult.ImpactPoint.Z + CapsuleHalfHeight;
			HeightZ = FMath::Max(HeightZ, MinOriginZ);
			return FVector(Input.X, Input.Y, HeightZ);
		}
		FVector Result = Input;
		Result.Z = FMath::Max(Result.Z, MinOriginZ);
		return Result;
	}

	// bTraceUnderInput=true: foot-down sweep.
	// CapsuleHalfHeight is the Z distance from the actor origin down to the foot:
	//   - ACharacter (pivot at capsule center): equal to the scaled capsule half height (e.g. 96).
	//   - Mesh with pivot at the bottom (foot): 0.
	//   - Mesh with pivot at model center of height H: H/2.
	// If the first sweep misses, retry up to 10 times, lifting the start by
	// (EffectiveRescue / 10) each iteration, where EffectiveRescue falls back
	// to CapsuleRadius when CapsuleHalfHeight is 0 (bottom-pivot meshes like a
	// Cube have zero pivot->foot distance and would otherwise get a zero-length
	// rescue range). The 10th retry starts exactly at FootStart + EffectiveRescue.
	const FVector FootStart = Input - FVector(0.0, 0.0, CapsuleHalfHeight);
	// Fallback to CapsuleRadius when pivot==foot (CapsuleHalfHeight == 0) so
	// bottom-pivot actors still get a usable upward rescue range.
	const float EffectiveRescue = FMath::IsNearlyZero(CapsuleHalfHeight) ? CapsuleRadius : CapsuleHalfHeight;
	const float StepSize = EffectiveRescue / 10.0f;
	FVector Start = FootStart;
	const FVector End = Input + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
	{
		float HeightZ = HitResult.ImpactPoint.Z + CapsuleHalfHeight;
		// Clamp so the foot stays at or above GroundMinHeight (e.g. water surface).
		HeightZ = FMath::Max(HeightZ, MinOriginZ);
		return FVector(Input.X, Input.Y, HeightZ);
	}
	// Rescue retries: lift the start by StepSize each iteration. After 10
	// retries the start reaches the actor origin exactly.
	int32 LoopIndex = 1;
	while (LoopIndex <= 10)
	{
		Start = FootStart + FVector(0.0, 0.0, StepSize * LoopIndex);
		if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
		{
			float HeightZ = HitResult.ImpactPoint.Z + CapsuleHalfHeight;
			// Clamp so the foot stays at or above GroundMinHeight (e.g. water surface).
			HeightZ = FMath::Max(HeightZ, MinOriginZ);
			return FVector(Input.X, Input.Y, HeightZ);
		}
		LoopIndex++;
	}
	// No hit at all: still honor GroundMinHeight so Input is not left below the configured floor.
	FVector Result = Input;
	Result.Z = FMath::Max(Result.Z, MinOriginZ);
	return Result;
}

AActor* UXkTargetMovementComponent::GetSnapActor(const FVector& Input, const ECollisionChannel Channel, const bool bTraceComplex, const bool bTraceUnderInput)
{
	// bTraceUnderInput=true (default): foot-down sweep, mirrors GetSnapLocation.
	// bTraceUnderInput=false: MAX_Z-down sweep, returns actor on the highest surface.
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	// Optionally ignore every pawn so prop-like actors do not snap onto a
	// character capsule that happens to stand on the same hex.
	if (bIgnoreOnlyPawn)
	{
		for (TActorIterator<APawn> It(GetWorld()); It; ++It)
		{
			CollisionParams.AddIgnoredActor(*It);
		}
	}
	CollisionParams.bTraceComplex = bTraceComplex;
	const float TraceRadius = CapsuleRadius * 0.25f;

	if (!bTraceUnderInput)
	{
		// MAX_Z sweep: find the actor on the topmost surface at this XY.
		const FVector Start = FVector(Input.X, Input.Y, UE_FLOAT_HUGE_DISTANCE);
		const FVector End   = FVector(Input.X, Input.Y, -UE_FLOAT_HUGE_DISTANCE);
		if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
		{
			return HitResult.GetActor();
		}
		return nullptr;
	}

	// bTraceUnderInput=true: foot-down sweep.
	// Mirror GetSnapLocation: sweep down starting from the foot
	// (Input.Z - CapsuleHalfHeight) and retry by lifting the start in
	// (EffectiveRescue / 10) steps, up to 10 times. EffectiveRescue falls back
	// to CapsuleRadius when CapsuleHalfHeight is 0 so bottom-pivot actors still
	// get a usable upward rescue range.
	const FVector FootStart = Input - FVector(0.0, 0.0, CapsuleHalfHeight);
	const float EffectiveRescue = FMath::IsNearlyZero(CapsuleHalfHeight) ? CapsuleRadius : CapsuleHalfHeight;
	const float StepSize = EffectiveRescue / 10.0f;
	FVector Start = FootStart;
	const FVector End = Input + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
	{
		return HitResult.GetActor();
	}
	int32 LoopIndex = 1;
	while (LoopIndex <= 10)
	{
		Start = FootStart + FVector(0.0, 0.0, StepSize * LoopIndex);
		if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
		{
			return HitResult.GetActor();
		}
		LoopIndex++;
	}
	return nullptr;
}

void UXkTargetMovementComponent::ValidateOnGround()
{
	if (bFailToGround)
	{
		FVector ActorLocation = GetMovementActor()->GetActorLocation();
		FVector TargetLocation = GetSnapLocation(ActorLocation, ECollisionChannel::ECC_Pawn, true);
		GetMovementActor()->SetActorLocation(TargetLocation);
	}
}

FVector UXkTargetMovementComponent::CalcParaCurve(const FVector& Start, const FVector& End, const float CurveArc, const float CurveDist)
{
	// y = a*x^2 + b*x + c
	// x = dist(start, end)
	// y = 0
	// b = ?
	const float a = CurveArc / 100.0 /* cm to m */;
	const float t = CurveDist;
	float b = 0.0f;
	const float c = Start.Z - End.Z;
	const float h = End.Z;
	const float x = FVector::Dist2D(End, Start);
	const float y = 0.0;
	b = (y - a * x * x - c) / x;

	const float s = t / x;
	float vx = FMath::Lerp(Start.X, End.X, s);
	float vy = FMath::Lerp(Start.Y, End.Y, s);
	float vz = a * t * t + b * t + c + h;
	return FVector(vx, vy, vz);
}

TArray<FVector> UXkTargetMovementComponent::CalcParaCurvePoints(const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum)
{
	TArray<FVector> Points;
	for (int32 i = 0; i <= SegmentNum; ++i)
	{
		float T = i / static_cast<float>(SegmentNum);
		float Dist = FVector::Dist2D(Start, End) * T;
		FVector Point = CalcParaCurve(Start, End, CurveArc, Dist);
		Points.Add(Point);
	}
	return Points;
}

UXkSplineMovementComponent::UXkSplineMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Activate ticking in order to update the cursor every frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	CurrentLength = 0.0;
}

void UXkSplineMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPED_NAMED_EVENT(UXkSplineMovement_TickComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UXkSplineMovement_TickComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_UXkSplineMovement_TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If the movement is not active, do nothing
	if (!IsActive())
	{
		return;
	}

	if (AActor* MovingActor = GetMovementActor())
	{
		if (!ShouldDoAction())
		{
			return;
		}

		// If TargetSpline is NULL which means TargetSpline might be destroyed
		// after the moving start, just keep the line and flight over through.
		if (!TargetSpline.IsValid())
		{
			FVector Location = GetMovementActor()->GetActorLocation();
			Location += Velocity;
			MovingActor->SetActorLocation(Location);
			return;
		}

		float DeltaDistance = DeltaTime * MaxVelocity;
		FVector DeltaDirection = TargetSpline->GetDirectionAtDistanceAlongSpline(CurrentLength, ESplineCoordinateSpace::World);
		DeltaDirection.Normalize();
		if (CurrentLength <= TargetSpline->GetSplineLength())
		{
			FVector Location = TargetSpline->GetLocationAtDistanceAlongSpline(CurrentLength, ESplineCoordinateSpace::World);
			FRotator Rotator = TargetSpline->GetRotationAtDistanceAlongSpline(CurrentLength, ESplineCoordinateSpace::World);
			MovingActor->SetActorLocation(Location);
			MovingActor->SetActorRotation(Rotator);
		}
		else
		{
			FVector Location = GetMovementActor()->GetActorLocation();
			Location += DeltaDirection * DeltaDistance;
			MovingActor->SetActorLocation(Location);
		}
		CurrentLength += DeltaDistance;
		Velocity = DeltaDirection * DeltaDistance;
	}
}

bool UXkSplineMovementComponent::IsOnSpline() const
{
	if (TargetSpline.IsValid() && CurrentLength <= TargetSpline->GetSplineLength())
	{
		return true;
	}
	return false;
}

void UXkSplineMovementComponent::SetTargetSpline(USplineComponent* Input)
{
	TargetSpline = MakeWeakObjectPtr<USplineComponent>(Input);
}

AXkCharacter::AXkCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AXkCharacter::OnHit);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AXkCharacter::OnBeginOverlap);

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->SetActive(false);
	GetCharacterMovement()->SetAutoActivate(false);

	// disable receives decals by default
	GetMesh()->SetReceivesDecals(false);
	// Spawn and enable AI auto search path
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Configure character movement
	TargetMovement = CreateDefaultSubobject<UXkTargetMovementComponent>(TEXT("Target Movement"));
	TargetMovement->bFailToGround = true;
	TargetMovement->CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	// ACharacter: pivot sits at capsule center, so pivot-to-foot == scaled capsule half height.
	TargetMovement->CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TargetMovement->SetActive(true);
	TargetMovement->SetAutoActivate(true);

	// Configure buoyancy component
	BuoyancyComponent = CreateDefaultSubobject<UXkBuoyancyComponent>(TEXT("Buoyancy Component"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AXkCharacter::OnConstruction(const FTransform& Transform)
{
	TargetMovement->CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	// Keep pivot-to-foot in sync for character-shaped actors: pivot at capsule center.
	TargetMovement->CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	Super::OnConstruction(Transform);
}

void AXkCharacter::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}

bool AXkCharacter::IsCharacterFalling() const
{
	if (GetCharacterMovement()->IsActive())
	{
		return GetCharacterMovement()->IsFalling();
	}
	if (GetXkTargetMovement()->IsActive())
	{
		return GetXkTargetMovement()->IsFalling();
	}
	return false;
}

bool AXkCharacter::IsCharacterMoving() const
{
	if (GetCharacterMovement()->IsActive())
	{
		FVector Velocity = GetCharacterMovement()->Velocity;
		return GetCharacterMovement()->IsWalking() && (Velocity.SizeSquared() > FMath::Square(GetCharacterMovement()->MinAnalogWalkSpeed));
	}
	else if (GetXkTargetMovement()->IsActive())
	{
		return GetXkTargetMovement()->IsMoving();
	}
	return false;
}

FVector AXkCharacter::GetCharacterVelocity() const
{
	if (GetCharacterMovement()->IsActive())
	{
		return GetCharacterMovement()->Velocity;
	}
	if (GetXkTargetMovement()->IsActive())
	{
		return GetXkTargetMovement()->Velocity;
	}
	return FVector::ZeroVector;
}

FVector AXkCharacter::GetCharacterAcceleration() const
{
	if (GetCharacterMovement()->IsActive())
	{
		return GetCharacterMovement()->GetCurrentAcceleration();
	}
	if (GetXkTargetMovement()->IsActive())
	{
		return GetXkTargetMovement()->Acceleration;
	}
	return FVector::ZeroVector;
}

UE_ENABLE_OPTIMIZATION