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

UE_DISABLE_OPTIMIZATION

UXkMovement::UXkMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldDoAction = false;
	bIsMoving = false;
	bIsRotating = false;
	MaxVelocity = 500.0;
	MaxAcceleration = 2048.0;
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
	bFailToGround = false;
	MoveAcceler = 1.0f;
	JumpAcceler = 1.0f;
	FlyAcceler = 1.0f;
	SlideAcceler = 1.0f;
	JumpArc = -0.5;
	FlyArc = -0.1;
	CapsuleRadius = 55.0f;
	CapsuleHalfHeight = 96.0f;
	MaxStepHeight = 45.0f;

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

	// Whether every action finished?
	if (IsActionFinished())
	{
		bShouldDoAction = false;
		MoveAcceler = JumpAcceler = FlyAcceler = SlideAcceler = 1.0f;
		OnMovementFinishEvent.Broadcast();
		OnMovementReachTargetEvent.Broadcast(ActionPoint);
		ClearActionPoint();
		ClearActionTargets();
	}
}

void UXkTargetMovementComponent::OnAction()
{
	if (AActor* MovingActor = GetMovementActor())
	{
		bShouldDoAction = true;
		LastTarget = MovingActor->GetActorLocation();
		bIsMoving = bIsRotating = bIsJumping = bIsSliding = bIsFalling = bIsFlying = false;
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
				NewLocation = GetLineTraceLocation(NewLocation);
				Velocity = CurrentVelocity * MovingDir;
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
		else if (CurrentTarget.Key == EActionType::Rotate)
		{
			if (CheckRotationSafely(Rotation, TargetRotation))
			{
				// Closed enough, stop moving
				bIsRotating = false;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
				OnMovementReachTargetEvent.Broadcast(ActionPoint);
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

				Acceleration += TargetRotator.Vector() * MaxAcceleration * DeltaTime;
				Velocity += (TargetRotator.Vector() * MaxVelocity * DeltaTime);
			}
		}
		else if (CurrentTarget.Key == EActionType::Jump)
		{
			TargetLocation = GetLineTraceLocation(TargetLocation, ECollisionChannel::ECC_Pawn, true, false);
			if (CheckDistance2DSafely(Location, TargetLocation))
			{
				bIsJumping = false;
				LastTarget = TargetLocation;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
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
			if (CheckDistanceSafely(Location, TargetLocation))
			{
				// Closed enough, stop moving
				bIsSliding = false;
				LastTarget = TargetLocation;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;
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
			}
			else if (bIsSliding)
			{
				FVector TargetVector = TargetLocation;
				FVector StartVector = Location;

				// Vertical slide to target
				if (CheckDistance2DSafely(Location, TargetVector))
				{
					FVector NewLocation = FMath::VInterpTo(StartVector, TargetVector, DeltaTime, 9.80f);
					if (NewLocation.Z > TargetVector.Z)
					{
						NewLocation = TargetVector;
					}
					Velocity = (NewLocation - Location) / DeltaTime;
					Acceleration = MaxAcceleration * (TargetVector - StartVector).GetSafeNormal();
					LastLocation = GetMovementActor()->GetActorLocation();
					GetMovementActor()->SetActorLocation(NewLocation);
					// Slide vertically is another type of falling
					bIsFalling = true;
				}
				// Horizontal slide to target
				else
				{
					TargetVector.Z = Location.Z;
					FVector MovingDir = (TargetVector - StartVector);
					MovingDir.Normalize();
					float CurrentVelocity = Velocity.Size();
					float CurrentAcceleration = MaxAcceleration;
					CurrentVelocity += CurrentAcceleration * DeltaTime;
					CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * SlideAcceler);
					FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
						FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
					Velocity = (NewLocation - Location) / DeltaTime;
					Acceleration = CurrentAcceleration * MovingDir;
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
		FVector TargetLocation = GetLineTraceLocation(ActorLocation, ECollisionChannel::ECC_Pawn, true);
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


FVector UXkTargetMovementComponent::GetMovementActorCenter() const
{
	if (AActor* Actor = GetMovementActor(); IsValid(Actor))
	{
		return Actor->GetComponentsBoundingBox().GetCenter();
	}
	return FVector::ZeroVector;
}


float UXkTargetMovementComponent::GetMovementActorHeight() const
{
	if (AActor* Actor = GetMovementActor(); IsValid(Actor))
	{
		return Actor->GetComponentsBoundingBox().GetSize().Z;
	}
	return 0.0f;
}


FVector UXkTargetMovementComponent::GetLineTraceLocation(const FVector& Input, const ECollisionChannel Channel, const bool bTraceComplex, const bool bTraceUnderInput)
{
	FHitResult HitResult;
	FVector Start = bTraceUnderInput ? Input + FVector(0.0, 0.0, MaxStepHeight) : Input + FVector(0.0, 0.0, UE_FLOAT_HUGE_DISTANCE);
	FVector End = Input + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	CollisionParams.bTraceComplex = bTraceComplex;
	float TraceRadius = CapsuleRadius * 0.25f;
	if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
	{
		float HeightZ = HitResult.ImpactPoint.Z + CapsuleHalfHeight;
		return FVector(Input.X, Input.Y, HeightZ);
	}
	return Input;
}


AActor* UXkTargetMovementComponent::GetLineTraceActor(const FVector& Input, const ECollisionChannel Channel, const bool bTraceComplex, const bool bTraceUnderInput)
{
	FHitResult HitResult;
	FVector Center = GetMovementActorCenter();
	FVector Start = bTraceUnderInput ? Center : Center + FVector(0.0, 0.0, UE_FLOAT_HUGE_DISTANCE);
	FVector End = Center + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	CollisionParams.bTraceComplex = bTraceComplex;
	float TraceRadius = CapsuleRadius * 0.25f;
	if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
	{
		return HitResult.GetActor();
	}
	return nullptr;
}


FVector UXkTargetMovementComponent::GetSphereTraceLocation(const FVector& Input, const ECollisionChannel Channel, const bool bTraceComplex)
{
	FHitResult HitResult;
	FVector Center = GetMovementActorCenter();
	FVector Start = Center + FVector(0.0, 0.0, UE_FLOAT_HUGE_DISTANCE);
	FVector End = Center + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	CollisionParams.bTraceComplex = bTraceComplex;
	float TraceRadius = CapsuleRadius * 0.25f;
    if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(TraceRadius), CollisionParams))
	{
		return HitResult.ImpactPoint + FVector(0.0, 0.0, CapsuleHalfHeight);
	}
	return Input;
}

void UXkTargetMovementComponent::ValidateOnGround()
{
	if (bFailToGround)
	{
		FVector ActorLocation = GetMovementActor()->GetActorLocation();
		FVector TargetLocation = GetLineTraceLocation(ActorLocation, ECollisionChannel::ECC_Pawn, false /*Not bTraceUnderInput*/);
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
