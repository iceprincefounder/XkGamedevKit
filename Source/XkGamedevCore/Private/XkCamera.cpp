// Copyright Epic Games, Inc. All Rights Reserved.

#include "XkCamera.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

AXkTopDownCamera::AXkTopDownCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for player capsule
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->InitCapsuleSize(42.f, 42.f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = TEXT("ID_Camera");
		ArrowComponent->SpriteInfo.DisplayName = FText::FromString(TEXT("ID_Camera"));
		ArrowComponent->SetupAttachment(CapsuleComponent);
		ArrowComponent->bIsScreenSizeScaled = true;
		ArrowComponent->SetSimulatePhysics(false);
	}
#endif // WITH_EDITORONLY_DATA

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->SetRelativeRotation(FRotator(-75.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraInvisibleWall = FBox2D(FVector2D(-1000.0, -1000.0), FVector2D(1000.0, 1000.0));
	CameraRotationLock = FVector2D(-75.0, -55.0);
	CameraZoomArmLength = 1200.0f;
	MaxVelocity = 500.0;
	MaxAcceleration = 2048.0;
	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AXkTopDownCamera::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (bMoveToTarget)
	{
		FVector Location = GetActorLocation();
		if (FVector::Dist2D(Location, MovementTarget) > 0.1f)
		{
			FVector StartVector = Location;
			FVector TargetVector = FVector(MovementTarget.X, MovementTarget.Y, Location.Z);
			FVector MovingDir = (TargetVector - StartVector);
			MovingDir.Normalize();
			float CurrentVelocity = Velocity.Size();
			float CurrentAcceleration = MaxAcceleration * DeltaSeconds;
			CurrentVelocity += CurrentAcceleration;
			CurrentVelocity = FMath::Clamp(CurrentVelocity, 0.0, MaxVelocity);
			FVector NewLocation = FMath::VInterpTo(StartVector, TargetVector, DeltaSeconds, CurrentVelocity);
			Velocity = (NewLocation - Location) / DeltaSeconds;
			Acceleration = (NewLocation - Location) * MaxAcceleration;
			SetActorLocation(NewLocation);
		}
		else
		{
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			bMoveToTarget = false;
		}
	}
}


void AXkTopDownCamera::AddMovement(const FVector2D& InputValue, const float Speed)
{
	FVector MovementVector = FVector(InputValue.Y, InputValue.X, 0.0);
	AddMovement(MovementVector, Speed);
}


void AXkTopDownCamera::AddMovement(const FVector& InputValue, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddMovement, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddMovement);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddMovement);

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// if camera is moving to target, ignore controller input
	if (bMoveToTarget)
	{
		return;
	}

	FRotator Rotator = GetForwardRotator();
	FVector MovementVectorRotated = Rotator.RotateVector(InputValue);
	AddActorWorldOffset(MovementVectorRotated * Speed * DeltaSeconds);

	if (CameraInvisibleWall.bIsValid)
	{
		FVector Location = GetActorLocation();
		Location.X = FMath::Clamp(Location.X, CameraInvisibleWall.Min.X, CameraInvisibleWall.Max.X);
		Location.Y = FMath::Clamp(Location.Y, CameraInvisibleWall.Min.Y, CameraInvisibleWall.Max.Y);
		SetActorLocation(Location);
	}
}


void AXkTopDownCamera::AddRotation(const FVector2D& InputValue, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddRotation, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddRotation);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddRotation);

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// if camera is moving to target, ignore controller input
	if (bMoveToTarget)
	{
		return;
	}

	FRotator Rotator = CameraBoom->GetRelativeRotation();
	float Patch = Rotator.Pitch;
	float Yaw = Rotator.Yaw;
	float Roll = 0.0f;
	if ((Rotator.Pitch < CameraRotationLock.Y && InputValue.Y > 0.0) || (Rotator.Pitch > CameraRotationLock.X && InputValue.Y < 0.0))
	{
		Patch += InputValue.Y * Speed * DeltaSeconds;
		Patch = FMath::Clamp(Patch, CameraRotationLock.X, CameraRotationLock.Y);
	}
	Yaw += InputValue.X * Speed * DeltaSeconds;
	FRotator NewRotator = FRotator(Patch, Yaw, Roll);
	CameraBoom->SetRelativeRotation(NewRotator);
}


void AXkTopDownCamera::ResetRotation()
{
	FRotator NewRotator = FRotator(CameraRotationLock.Y, 0.0, 0.0);
	CameraBoom->SetRelativeRotation(NewRotator);
}


void AXkTopDownCamera::AddCameraZoom(const float InputValue, const FVector2D& Range, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddCameraZoom, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddCameraZoom);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddCameraZoom);

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// if camera is moving to target, ignore controller input
	if (bMoveToTarget)
	{
		return;
	}

	float TargetArmLength = CameraBoom->TargetArmLength;
	TargetArmLength += (InputValue * Speed * DeltaSeconds);
	TargetArmLength = FMath::Clamp(TargetArmLength, Range.X, Range.Y);
	CameraBoom->TargetArmLength = TargetArmLength;
}


void AXkTopDownCamera::ResetCameraZoom()
{
	CameraBoom->TargetArmLength = CameraZoomArmLength;
}


void AXkTopDownCamera::AddMovementTarget(const FVector& InTarget)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddMovementTarget, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddMovementTarget);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddMovementTarget);

	MovementTarget = InTarget;
	bMoveToTarget = true;
}


FRotator AXkTopDownCamera::GetForwardRotator() const
{
	FTransform ComponentToWorld = CameraBoom->GetComponentToWorld();
	FRotator Rotator = ComponentToWorld.Rotator();
	Rotator.Pitch = 0.0; Rotator.Roll = 0.0;
	return Rotator;
}
