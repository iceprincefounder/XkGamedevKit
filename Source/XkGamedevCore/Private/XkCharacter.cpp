// Copyright ©XUKAI. All Rights Reserved.

#include "XkCharacter.h"
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


UXkTargetMovement::UXkTargetMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bBlinkMode = false;
	bFailToGround = false;
	MoveAcceler = 1.0f;
	JumpAcceler = 1.0f;
	ShotAcceler = 1.0f;
	SlideAcceler = 1.0f;
	JumpArc = -0.5;
	ShotArc = -0.1;
	CapsuleHalfHeight = 0.0;

	// Set default values
	ActionPoint = 0;
	MoveCostPoint = 1;
	RotateCostPoint = 0;
	JumpCostPoint = 2;
	SlideCostPoint = 0;
	ShotCostPoint = 0;

	// Activate ticking in order to update the cursor every frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}


void UXkTargetMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPED_NAMED_EVENT(UXkMovement_TickComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UXkMovement_TickComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_UXkMovement_TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	check(GetMovementActor());

	if (!bShouldDoAction && bFailToGround)
	{
		// Snap to ground
		FVector ActorLocation = GetMovementActor()->GetActorLocation();
		FVector TargetLocation = GetLineTraceLocation(ActorLocation);
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
		}
		else if (TargetLocation.Z < ActorLocation.Z)
		{
			// Falling to target if target is lower than current location
			const float Gravity = -980.0f; // Unreal Engine unit cm/s^2
			FVector GravityAcceleration(0.0f, 0.0f, Gravity);
			Acceleration = GravityAcceleration;
			Velocity += Acceleration * DeltaTime;
			
			NewLocation = ActorLocation + Velocity * DeltaTime + 0.5f * Acceleration * DeltaTime * DeltaTime;
			bIsFalling = true;
		}
		GetMovementActor()->SetActorLocation(NewLocation);
		return Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	FVector Location = GetMovementActor()->GetActorLocation();
	FRotator Rotation = GetMovementActor()->GetActorRotation();

	// Whether every action finished?
	if ((PendingMoveTargets.Num() == 0 || ActionPoint == 0)
		&& PendingRotateTargets.Num() == 0
		&& (PendingJumpTargets.Num() == 0 || ActionPoint == 0)
		&& PendingShotTargets.Num() == 0
		&& PendingSlideTargets.Num() == 0
		&& !bIsMoving && !bIsRotating && !bIsJumping && !bIsShotting && !bIsSliding && bShouldDoAction)
	{
		bShouldDoAction = false;
		MoveAcceler = JumpAcceler = ShotAcceler = SlideAcceler = 1.0f;
		OnMovementFinishEvent.Broadcast();
		OnMovementReachTargetEvent.Broadcast(ActionPoint);
		ClearActionPoint();
		ClearMoveTargets();
		ClearRotateTargets();
		ClearJumpTargets();
		ClearShotTargets();
		ClearSlideTargets();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Moving -> Rotating -> Jumping -> Shotting -> Sliding

	// 1. Moving
	if (bIsMoving)
	{
		/////////////////////////////////////////////////////////////
		// If into new Hexagon, change to new target
		// else if, stop moving
		// @TODO: check distance is not safe, pool FPS would case many problem
		if ((PendingMoveTargets.Num() == 0 || ActionPoint < MoveCostPoint) && CheckDistanceSafely(Location, CurrentMoveTarget))
		{
			// Closed enough, stop moving
			bIsMoving = false;
			bIsRotating = true;
			CurrentRotateTarget = GetMovementActor()->GetActorRotation();
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else if (PendingMoveTargets.Num() > 0 && ActionPoint >= MoveCostPoint && CheckDistanceSafely(Location, CurrentMoveTarget))
		{
			// decrease PendingMoveTargets count
			LastTarget = CurrentMoveTarget;
			CurrentMoveTarget = PendingMoveTargets.Pop(true);
			// broadcast current action point of last target reached, the action point was decreased when start to move to that target
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
			// decrease MovementPoint count
			ActionPoint -= MoveCostPoint;
		}
		/////////////////////////////////////////////////////////////
		// If not closed to target, move
		else
		{
			FVector TargetVector = FVector(CurrentMoveTarget.X, CurrentMoveTarget.Y, Location.Z);
			FVector StartVector = Location;
			FVector MovingDir = (TargetVector - StartVector);
			MovingDir.Normalize();
			float CurrentVelocity = Velocity.Size();
			float CurrentAcceleration = MaxAcceleration * MoveAcceler;
			CurrentVelocity += CurrentAcceleration * DeltaTime;
			CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * MoveAcceler);
			FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
				FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
			// Snap to ground
			NewLocation = GetLineTraceLocation(NewLocation);
			Velocity = CurrentVelocity * MovingDir;
			Acceleration = CurrentAcceleration * MovingDir;
			GetMovementActor()->SetActorLocation(NewLocation);

			const FVector X = TargetVector - StartVector;
			FRotator StartRotator = GetMovementActor()->GetActorRotation();
			FRotator TargetRotator = FRotationMatrix::MakeFromX(X).Rotator();
			FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
				FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
			GetMovementActor()->SetActorRotation(NewRotator);
		}
	} // Moving

	// 2. Rotating
	if (bIsRotating)
	{
		if ((PendingRotateTargets.Num() == 0 || ActionPoint < RotateCostPoint /* Useless*/) && CheckRotationSafely(Rotation, CurrentRotateTarget))
		{
			// Closed enough, stop moving
			bIsRotating = false;
			bIsJumping = true;
			CurrentJumpTarget = GetMovementActor()->GetActorLocation();
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else if ((PendingRotateTargets.Num() > 0 && ActionPoint >= RotateCostPoint) && CheckRotationSafely(Rotation, CurrentRotateTarget))
		{
			FVector RotationTarget = PendingRotateTargets.Pop(true);
			FVector TargetLocation = FVector(RotationTarget.X, RotationTarget.Y, Location.Z);
			const FVector X = TargetLocation - Location;
			CurrentRotateTarget = FRotationMatrix::MakeFromX(X).Rotator();
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else
		{
			FRotator StartRotator = GetMovementActor()->GetActorRotation();
			FRotator TargetRotator = CurrentRotateTarget;
			FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
				FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
			GetMovementActor()->SetActorRotation(NewRotator);

			Acceleration += TargetRotator.Vector() * MaxAcceleration * DeltaTime;
			Velocity += (TargetRotator.Vector() * MaxVelocity * DeltaTime);
		}
	}

	// 3. Jumping
	if (bIsJumping)
	{
		if ((PendingJumpTargets.Num() == 0 || ActionPoint < JumpCostPoint) && CheckDistanceSafely(Location, CurrentJumpTarget))
		{
			bIsJumping = false;
			bIsShotting = true;
			CurrentShotTarget = GetMovementActor()->GetActorLocation();
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else if (PendingJumpTargets.Num() > 0 && ActionPoint >= JumpCostPoint && CheckDistanceSafely(Location, CurrentJumpTarget))
		{
			// decrease PendingMoveTargets count
			LastTarget = CurrentJumpTarget;
			CurrentJumpTarget = PendingJumpTargets.Pop(true);
			CurrentJumpTarget = GetLineTraceLocation(CurrentJumpTarget);
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
			// decrease MovementPoint count
			ActionPoint -= JumpCostPoint;
		}
		else
		{
			FVector TargetVector = FVector(CurrentJumpTarget.X, CurrentJumpTarget.Y, Location.Z);
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
				FVector CurrLocation = CalcParaCurve(LastTarget.GetValue(), CurrentJumpTarget, JumpArc, CurrLength);
				NewLocation.Z = CurrLocation.Z;
			}
			Velocity = (NewLocation - Location) / DeltaTime;
			Acceleration = CurrentAcceleration * MovingDir;
			GetMovementActor()->SetActorLocation(NewLocation);

			const FVector X = TargetVector - StartVector;
			FRotator StartRotator = GetMovementActor()->GetActorRotation();
			FRotator TargetRotator = FRotationMatrix::MakeFromX(X).Rotator();
			FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
				FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
			GetMovementActor()->SetActorRotation(NewRotator);
		}
	}

	// 4. Shotting
	if (bIsShotting)
	{
		if ((PendingShotTargets.Num() == 0 || ActionPoint < ShotCostPoint) && CheckDistanceSafely(Location, CurrentShotTarget))
		{
			bIsShotting = false;
			bIsSliding = true;
			CurrentSlideTarget = GetMovementActor()->GetActorLocation();
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else if (PendingShotTargets.Num() > 0 && ActionPoint >= ShotCostPoint && CheckDistanceSafely(Location, CurrentShotTarget))
		{
			LastTarget = CurrentShotTarget;
			CurrentShotTarget = PendingShotTargets.Pop(true);
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
			// decrease MovementPoint count
			ActionPoint -= ShotCostPoint;
		}
		else
		{
			FVector TargetVector = FVector(CurrentShotTarget.X, CurrentShotTarget.Y, Location.Z);
			FVector StartVector = Location;
			FVector MovingDir = (TargetVector - StartVector);
			MovingDir.Normalize();
			float CurrentVelocity = Velocity.Size();
			float CurrentAcceleration = MaxAcceleration * ShotAcceler;
			CurrentVelocity += CurrentAcceleration * DeltaTime;
			CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity * ShotAcceler);
			FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
				FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
			// Calculate the height of shot
			if (LastTarget.IsSet())
			{
				float CurrLength = FVector::Dist2D(NewLocation, LastTarget.GetValue());
				FVector CurrLocation = CalcParaCurve(LastTarget.GetValue(), CurrentShotTarget, ShotArc, CurrLength);
				NewLocation.Z = CurrLocation.Z;
			}
			Velocity = (NewLocation - Location) / DeltaTime;
			Acceleration = CurrentAcceleration * MovingDir;
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

	// 5. Sliding
	if (bIsSliding)
	{
		if ((PendingSlideTargets.Num() == 0 || ActionPoint < SlideCostPoint) && CheckDistanceSafely(Location, CurrentSlideTarget))
		{
			// Closed enough, stop moving
			bIsSliding = false;
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		else if ((PendingSlideTargets.Num() > 0 && ActionPoint >= SlideCostPoint) && CheckDistanceSafely(Location, CurrentSlideTarget))
		{
			// decrease PendingMoveTargets count
			LastTarget = CurrentSlideTarget;
			CurrentSlideTarget = PendingSlideTargets.Pop(true);
			OnMovementReachTargetEvent.Broadcast(ActionPoint);
		}
		/////////////////////////////////////////////////////////////
		// If not closed to target, move
		else
		{
			FVector TargetVector = FVector(CurrentSlideTarget.X, CurrentSlideTarget.Y, Location.Z);
			FVector StartVector = Location;
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
			GetMovementActor()->SetActorLocation(NewLocation);
		}
	}
}


void UXkTargetMovement::OnAction()
{
	if (AActor* MovingActor = GetMovementActor())
	{
		bShouldDoAction = true;
		bIsMoving = true;
		CurrentMoveTarget = MovingActor->GetActorLocation();
		bIsRotating = bIsJumping = bIsSliding = false;
		OnMovementBeginEvent.Broadcast();
	}
}


TArray<FVector> UXkTargetMovement::GetValidMovementTargets() const
{
	TArray<FVector> Results;
	TArray<FVector> CheckTargets = PendingMoveTargets;
	for (uint8 Index = 0; ((Index < ActionPoint + 1) && (Index < PendingMoveTargets.Num())); Index++)
	{
		FVector Location = CheckTargets.Pop(true /* Shrink*/);
		Results.Insert(Location, 0);
	}
	return Results;
}


FVector UXkTargetMovement::GetFinalMovementTarget() const
{
	FVector Result = FVector::ZeroVector;
	if (AActor* MovingActor = GetMovementActor())
	{
		Result = MovingActor->GetActorLocation();
	}

	TArray<FVector> ValidMovementTargets;
	TArray<FVector> CheckTargets = PendingMoveTargets;
	for (uint8 Index = 0; ((Index < ActionPoint + 1) && (Index < PendingMoveTargets.Num())); Index++)
	{
		FVector Location = CheckTargets.Pop(true /* Shrink*/);
		ValidMovementTargets.Insert(Location, 0);
	}
	if (ValidMovementTargets.Num() > 0)
	{
		Result = ValidMovementTargets.Last();
	}
	return Result;
}


FVector UXkTargetMovement::GetLineTraceLocation(const FVector& Input, const ECollisionChannel Channel)
{
	FHitResult HitResult;
	FVector Start = Input + FVector(0.0, 0.0, UE_FLOAT_HUGE_DISTANCE);
	FVector End = Input + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetMovementActor());
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Channel, CollisionParams))
	{
		return HitResult.Location + FVector(0.0, 0.0, CapsuleHalfHeight);
	}
	return Input;
}


FVector UXkTargetMovement::CalcParaCurve(const FVector& Start, const FVector& End, const float CurveArc, const float CurveDist)
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


TArray<FVector> UXkTargetMovement::CalcParaCurvePoints(const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum)
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


bool UXkTargetMovement::CalcParaIntersection(const UObject* WorldContextObject, const FVector& Start, const FVector& End, const float CurveArc, const int32 SegmentNum, TArray<AActor*> IgnoreActors, const ECollisionChannel TraceChannel)
{
	TArray<FVector> Points = CalcParaCurvePoints(Start, End, CurveArc, SegmentNum);

	FHitResult HitResult;
	for (int32 i = 0; i < (Points.Num() - 1); ++i)
	{
		FVector A = Points[i];
		FVector B = Points[i + 1];

#if ENABLE_DRAW_DEBUG
		//DrawDebugLine(GetWorld(), A, B, FColor::Red, false, -1.0f, 0, 10.0);
#endif
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors(IgnoreActors);
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			if (World->LineTraceSingleByChannel(HitResult, A, B, TraceChannel, CollisionQueryParams))
			{
				return true;
			}
		}
	}
	return false;
}


UXkSplineMovement::UXkSplineMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Activate ticking in order to update the cursor every frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	CurrentLength = 0.0;
}


void UXkSplineMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPED_NAMED_EVENT(UXkSplineMovement_TickComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UXkSplineMovement_TickComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_UXkSplineMovement_TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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


bool UXkSplineMovement::IsOnSpline() const
{
	if (TargetSpline.IsValid() && CurrentLength <= TargetSpline->GetSplineLength())
	{
		return true;
	}
	return false;
}


void UXkSplineMovement::SetTargetSpline(USplineComponent* Input)
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
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	// disable receives decals by default
	GetMesh()->SetReceivesDecals(false);
	// Spawn and enable AI auto search path
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


void AXkCharacter::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}