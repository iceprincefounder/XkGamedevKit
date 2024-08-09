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
	bShouldMove = false;
	bIsMoving = false;
	bIsRotating = false;
	MaxVelocity = 500.0;
	MaxAcceleration = 2048.0;
	RotationRate = FRotator(0.0, 640.0, 0.0);
}


AActor* UXkMovement::GetMovingActor() const
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

	// Activate ticking in order to update the cursor every frame.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}


void UXkTargetMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	auto IsMovementJobFinished = [this]() -> bool
	{
		bool bMovementJobFinished = PendingMovementTargets.Num() == 0 || MovementPoint == 0;
		return bMovementJobFinished;
	};

	auto IsRotationJobFinished = [this]() -> bool
	{
		bool bRotationJobFinished = PendingRotationTargets.Num() == 0;
		return bRotationJobFinished;
	};

	auto IsAllPendingJobFinished = [this, IsMovementJobFinished, IsRotationJobFinished]() -> bool
	{
		return IsRotationJobFinished() && IsMovementJobFinished();
	};

	SCOPED_NAMED_EVENT(UXkMovement_TickComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UXkMovement_TickComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_UXkMovement_TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bShouldMove)
	{
		return Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	if (AActor* MovingActor = GetMovingActor())
	{
		FVector Location = MovingActor->GetActorLocation();
		FRotator Rotation = MovingActor->GetActorRotation();

		////////////////////////////////////////////////////////////////////////////////
		// Sliding -> Moving -> Jumping -> Rotating

		// If no PendingMovementTargets, execute PendingRotationTargets
		if (!bIsMoving && IsMovementJobFinished())
		{
			if (PendingRotationTargets.Num() > 0)
			{
				FVector RotationTarget = PendingRotationTargets.Pop(true);
				FVector TargetLocation = FVector(RotationTarget.X, RotationTarget.Y, Location.Z);
				const FVector X = TargetLocation - Location;
				CurrentRotationTarget = FRotationMatrix::MakeFromX(X).Rotator();
				bIsRotating = true;
			}
			else if (bIsRotating && IsRotationJobFinished() && CheckRotationSafely(Rotation, CurrentRotationTarget))
			{
				// Closed enough, stop moving
				bIsRotating = false;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;

				if (IsAllPendingJobFinished())
				{
					bShouldMove = false;
					OnMovementBaseFinishEvent.Broadcast();
					OnMovementTargetFinishEvent.Broadcast(MovementPoint);
				}
			}

			if (bIsRotating && !CheckRotationSafely(Rotation, CurrentRotationTarget))
			{
				FRotator StartRotator = MovingActor->GetActorRotation();
				FRotator TargetRotator = CurrentRotationTarget;
				FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
					FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
				MovingActor->SetActorRotation(NewRotator);
				
				Acceleration += TargetRotator.Vector() * MaxAcceleration * DeltaTime;
				Velocity += (TargetRotator.Vector() * MaxAcceleration * DeltaTime);
			}
		}

		if (bIsRotating)
		{
			return;
		}

		if (!bShouldMove)
		{
			return;
		}

		if (!bIsMoving)
		{
			bIsMoving = true;
			OnMovementBaseBeginEvent.Broadcast();
		}

		/////////////////////////////////////////////////////////////
		// If into new Hexagon, change to new target
		// else if, stop moving
		// @TODO: check distance is not safe, pool FPS would case many problem
		if (CheckMovementSafely(Location, CurrentMovementTarget, 1.0f))
		{
			if (!IsMovementJobFinished())
			{
				// decrease MovementPoint count
				MovementPoint--;
				// decrease PendingMovementTargets count
				CurrentMovementTarget = PendingMovementTargets.Pop(true);
			}
			else if (CheckMovementSafely(Location, CurrentMovementTarget))
			{
				// Closed enough, stop moving
				bIsMoving = false;
				Velocity = FVector::ZeroVector;
				Acceleration = FVector::ZeroVector;

				if (IsAllPendingJobFinished())
				{
					bShouldMove = false;
					OnMovementBaseFinishEvent.Broadcast();
					OnMovementTargetFinishEvent.Broadcast(MovementPoint);
				}
			}
		}
		/////////////////////////////////////////////////////////////
		// If not closed to target, move
		if (!CheckMovementSafely(Location, CurrentMovementTarget))
		{
			FVector TargetVector = FVector(CurrentMovementTarget.X, CurrentMovementTarget.Y, Location.Z);
			FVector StartVector = Location;
			FVector MovingDir = (TargetVector - StartVector);
			MovingDir.Normalize();
			float CurrentVelocity = Velocity.Size();
			float CurrentAcceleration = MaxAcceleration * DeltaTime;
			// @TODO: Fix movement lagging
			//if (PendingMovementTargets.Num() > 0)
			//{
			//	FVector NextLocation = PendingMovementTargets[0];
			//	FVector NextTargetVector = FVector(NextLocation.X, NextLocation.Y, Location.Z);
			//	FVector NextMovingDir = (NextTargetVector - StartVector);
			//	NextMovingDir.Normalize();
			//	if (MovingDir == NextMovingDir)
			//	{
			//		TargetVector = NextTargetVector;
			//	}
			//}
			// Here is no deceleration currently.
			//CurrentVelocity -= CurrentAcceleration;
			CurrentVelocity += CurrentAcceleration;
			CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity);
			FVector NewLocation = bBlinkMode ? FMath::VInterpTo(StartVector, TargetVector, DeltaTime, CurrentVelocity) :
				FMath::VInterpConstantTo(StartVector, TargetVector, DeltaTime, CurrentVelocity);
			Velocity = (NewLocation - Location) / DeltaTime;
			Acceleration = (NewLocation - Location) * MaxAcceleration;
			MovingActor->SetActorLocation(NewLocation);

			// @DEBUG: WorldOffset
			//FString Message = FString::Printf(TEXT("WorldOffset %f"), MovingDir.Size());
			//GEngine->AddOnScreenDebugMessage(-1, 3.0, FColor::Orange, *Message);

			const FVector X = TargetVector - StartVector;
			FRotator StartRotator = MovingActor->GetActorRotation();
			FRotator TargetRotator = FRotationMatrix::MakeFromX(X).Rotator();
			FRotator NewRotator = bBlinkMode ? FMath::RInterpTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw) :
				FMath::RInterpConstantTo(StartRotator, TargetRotator, DeltaTime, RotationRate.Yaw);
			MovingActor->SetActorRotation(NewRotator);
		}
	}
}


void UXkTargetMovement::OnMoving()
{
	if (AActor* MovingActor = GetMovingActor())
	{
		if (PendingMovementTargets.Num() > 0)
		{
			// PendingMovementTargets contains the start hexagon which character step on currently,
			// So, begin every moving, add one more MovementPoint to make up for
			MovementPoint += 1;
			CurrentMovementTarget = MovingActor->GetActorLocation();
			bShouldMove = true;
		}
		if (PendingRotationTargets.Num() > 0)
		{
			CurrentRotationTarget = MovingActor->GetActorRotation();
			bShouldMove = true;
		}
		if (PendingMovementJumpTargets.Num() > 0)
		{
			CurrentMovementJumpTarget = MovingActor->GetActorLocation();
			bShouldMove = true;
		}
		if (PendingMovementSlideTargets.Num() > 0)
		{
			CurrentMovementSlideTarget = MovingActor->GetActorLocation();
			bShouldMove = true;
		}
	}
}


void UXkTargetMovement::AddMovementPoint(const uint8 Input)
{
	// Extra add one, for character currently standing hexagon target
	MovementPoint += Input;
}


TArray<FVector> UXkTargetMovement::GetValidMovementTargets() const
{
	TArray<FVector> Results;
	TArray<FVector> CheckTargets = PendingMovementTargets;
	for (uint8 Index = 0; ((Index < MovementPoint + 1) && (Index < PendingMovementTargets.Num())); Index++)
	{
		FVector Location = CheckTargets.Pop(true /* Shrink*/);
		Results.Insert(Location, 0);
	}
	return Results;
}


FVector UXkTargetMovement::GetFinalMovementTarget() const
{
	FVector Result = FVector::ZeroVector;
	if (AActor* MovingActor = GetMovingActor())
	{
		Result = MovingActor->GetActorLocation();
	}

	TArray<FVector> ValidMovementTargets;
	TArray<FVector> CheckTargets = PendingMovementTargets;
	for (uint8 Index = 0; ((Index < MovementPoint + 1) && (Index < PendingMovementTargets.Num())); Index++)
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
	if (AActor* MovingActor = GetMovingActor())
	{
		if (!ShouldMove())
		{
			return;
		}

		// If TargetSpline is NULL which means TargetSpline might be destroyed
		// after the moving start, just keep the line and flight over through.
		if (!TargetSpline.IsValid())
		{
			FVector Location = GetMovingActor()->GetActorLocation();
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
			FVector Location = GetMovingActor()->GetActorLocation();
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