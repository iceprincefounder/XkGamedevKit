// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkCamera.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

AXkCamera::AXkCamera(const FObjectInitializer& ObjectInitializer)
{
	CVarSetEnableStylizedRendering->SetOnChangedCallback(
		FConsoleVariableDelegate::CreateUObject(this, &AXkCamera::SetEnableStylizedPostProcess));
	bEnableStylizePostProcess = false;

}


void AXkCamera::OnConstruction(const FTransform& Transform)
{
	SetEnableStylizedPostProcess(CVarSetEnableStylizedRendering->AsVariable());
	Super::OnConstruction(Transform);
}


void AXkCamera::SetEnableStylizedPostProcess(IConsoleVariable* Var)
{
	bool bInEnable = Var->GetBool();
	GConfig->SetBool(
		TEXT("XkGamedevKit"),
		TEXT("r.EnableStylizedRendering"),
		bInEnable,
		GGameUserSettingsIni
	);
	GConfig->Flush(false, GGameUserSettingsIni);
}


AXkCharacterCamera::AXkCharacterCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for player capsule
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->InitCapsuleSize(15.f, 15.f);
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
	CameraBoom->SetUsingAbsoluteRotation(false); // Use arm to rotate when character does
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SetRelativeRotation(FRotator(0.0f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->CameraLagSpeed = 20.0;
	CameraBoom->CameraRotationLagSpeed = 20.0;

	// Create a camera...
	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CharacterCamera"));
	SceneCaptureComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}


void AXkCharacterCamera::SetEnableStylizedPostProcess(IConsoleVariable* Var)
{
	bool bInEnable = Var->GetBool();
	if (bEnableStylizePostProcess != bInEnable)
	{
		bEnableStylizePostProcess = bInEnable;
		if (bEnableStylizePostProcess)
		{
			for (UMaterialInterface* MaterialInterface : StylizedPostProcessMaterials)
			{
				if (MaterialInterface && IsValid(MaterialInterface))
				{
					UMaterialInstanceDynamic* MaterialDyn = UMaterialInstanceDynamic::Create(MaterialInterface, this);
					StylizedPostProcessMaterialDyns.Add(MaterialDyn);
					SceneCaptureComponent->PostProcessSettings.AddBlendable(MaterialDyn, 1.0f);
				}
			}
		}
		else
		{
			for (UMaterialInstanceDynamic* MaterialDyn : StylizedPostProcessMaterialDyns)
			{
				if (MaterialDyn && IsValid(MaterialDyn))
				{
					SceneCaptureComponent->PostProcessSettings.RemoveBlendable(MaterialDyn);
				}
			}
			StylizedPostProcessMaterialDyns.Empty();
		}
		AXkCamera::SetEnableStylizedPostProcess(Var);
	}
}


void AXkCharacterCamera::AddRotation(const FVector2D& InputValue, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkCharacterCamera_AddRotation, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkCharacterCamera_AddRotation);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkCharacterCamera_AddRotation);

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	float InputValueX = InputValue.X;

	float InputValueY = InputValue.Y;

	FRotator Rotator = CameraBoom->GetRelativeRotation();
	float Pitch = Rotator.Pitch;
	float Yaw = Rotator.Yaw;
	float Roll = 0.0f;

	Yaw += InputValueX * Speed * DeltaSeconds;
	Yaw = FRotator::ClampAxis(Yaw);
	FRotator NewRotator = FRotator(Pitch, Yaw, Roll);
	CameraBoom->SetRelativeRotation(NewRotator);
}


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
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 20.0;
	CameraBoom->CameraRotationLagSpeed = 20.0;

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	TopDownCameraComponent->FieldOfView = 55.0f;

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder(TEXT("/XkGamedevKit/Materials/M_CameraPostProcess"));
	PostProcessMaterial = ObjectFinder.Object;
	bUseCameraRotationLock = false;
	CameraRotationLock = FVector2D(-75.0, -55.0);
	CameraZoomArmLength = 1200.0;
	CameraZoomArmRange = FVector2D(800.0, 2000.0);
	MaxVelocity = 10.0;
	MaxAcceleration = 100.0;
	bTravelingMode = false;
	TravelingView = FRotator(-25.f, 0.f, 0.f);
	TravelingZoom = 2500.0f;
	TravelingSpeed = 500.0;
	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


void AXkTopDownCamera::OnConstruction(const FTransform& Transform)
{
	if (PostProcessMaterial)
	{
		PostProcessMaterialDyn = UMaterialInstanceDynamic::Create(PostProcessMaterial, this);
		TopDownCameraComponent->PostProcessSettings.AddBlendable(PostProcessMaterialDyn, 1.0f);
	}
	Super::OnConstruction(Transform);
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
			CurrentVelocity = FMath::Clamp(CurrentVelocity + CurrentAcceleration, 0.0, MaxVelocity);
			FVector NewLocation = FMath::VInterpTo(StartVector, TargetVector, DeltaSeconds, CurrentVelocity);
			Velocity = (NewLocation - Location) / DeltaSeconds;
			Acceleration = MovingDir * CurrentAcceleration;
			SetActorLocation(NewLocation);
		}
		else
		{
			Velocity = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
			bMoveToTarget = false;
		}
	}
	else if (bTravelingMode)
	{
		AddMovement(GetForwardRotator().Vector(), TravelingSpeed);
	}
}


void AXkTopDownCamera::SetEnableStylizedPostProcess(IConsoleVariable* Var)
{
	bool bInEnable = Var->GetBool();
	if (bEnableStylizePostProcess != bInEnable)
	{
		bEnableStylizePostProcess = bInEnable;
		if (bEnableStylizePostProcess)
		{
			for (UMaterialInterface* MaterialInterface : StylizedPostProcessMaterials)
			{
				if (MaterialInterface && IsValid(MaterialInterface))
				{
					UMaterialInstanceDynamic* MaterialDyn = UMaterialInstanceDynamic::Create(MaterialInterface, this);
					StylizedPostProcessMaterialDyns.Add(MaterialDyn);
					TopDownCameraComponent->PostProcessSettings.AddBlendable(MaterialDyn, 1.0f);
				}
			}
		}
		else
		{
			for (UMaterialInstanceDynamic* MaterialDyn : StylizedPostProcessMaterialDyns)
			{
				if (MaterialDyn && IsValid(MaterialDyn))
				{
					TopDownCameraComponent->PostProcessSettings.RemoveBlendable(MaterialDyn);
				}
			}
			StylizedPostProcessMaterialDyns.Empty();
		}
		AXkCamera::SetEnableStylizedPostProcess(Var);
	}
}


void AXkTopDownCamera::ResetCamera()
{
	SetTravelingMode(false);
	SetActorLocation(FVector::ZeroVector);
	ResetRotation();
	ResetCameraZoom();
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

	FRotator Rotator = GetForwardRotator();
	FVector MovementVectorRotated = Rotator.RotateVector(InputValue);
	AddActorWorldOffset(MovementVectorRotated * Speed * DeltaSeconds);

	bMoveToTarget = false;
}


void AXkTopDownCamera::AddRotation(const FVector2D& InputValue, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddRotation, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddRotation);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddRotation);
	auto LimitViewPitch = [](FRotator& ViewRotation, float InViewPitchMin, float InViewPitchMax)
	{
		ViewRotation.Pitch = FMath::ClampAngle(ViewRotation.Pitch, InViewPitchMin, InViewPitchMax);
		ViewRotation.Pitch = FRotator::ClampAxis(ViewRotation.Pitch);
	};

	auto LimitViewRoll = [](FRotator& ViewRotation, float InViewRollMin, float InViewRollMax)
	{
		ViewRotation.Roll = FMath::ClampAngle(ViewRotation.Roll, InViewRollMin, InViewRollMax);
		ViewRotation.Roll = FRotator::ClampAxis(ViewRotation.Roll);
	};

	auto LimitViewYaw = [](FRotator& ViewRotation, float InViewYawMin, float InViewYawMax)
	{
		ViewRotation.Yaw = FMath::ClampAngle(ViewRotation.Yaw, InViewYawMin, InViewYawMax);
		ViewRotation.Yaw = FRotator::ClampAxis(ViewRotation.Yaw);
	};

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	float InputValueX = InputValue.X;
	//if (InputValueX != 0.0 && FMath::Abs(InputValueX) > 0.25)
	//{
	//	InputValueX = InputValueX; // (InputValueX > 0.0) ? 1.0 : -1.0;
	//}
	float InputValueY = InputValue.Y;
	//if (InputValueY != 0.0 && FMath::Abs(InputValueY) > 0.25)
	//{
	//	InputValueY = InputValueY; // (InputValueY > 0.0) ? 1.0 : -1.0;
	//}

	FRotator Rotator = CameraBoom->GetRelativeRotation();
	float Pitch = Rotator.Pitch;
	float Yaw = Rotator.Yaw;
	float Roll = 0.0f;

	if (bUseCameraRotationLock)
	{
		if ((Rotator.Pitch < CameraRotationLock.Y && InputValueY > 0.0) || (Rotator.Pitch > CameraRotationLock.X && InputValueY < 0.0))
		{
			Pitch += InputValueY * Speed * DeltaSeconds;
			Pitch = FMath::ClampAngle(Pitch, CameraRotationLock.X, CameraRotationLock.Y);
			Pitch = FRotator::ClampAxis(Pitch);
		}
	}
	else
	{
		Pitch += InputValueY * Speed * DeltaSeconds;
		Pitch = FRotator::ClampAxis(Pitch);
	}

	Yaw += InputValueX * Speed * DeltaSeconds;
	Yaw = FRotator::ClampAxis(Yaw);
	FRotator NewRotator = FRotator(Pitch, Yaw, Roll);
	CameraBoom->SetRelativeRotation(NewRotator);
}


void AXkTopDownCamera::ResetRotation()
{
	CameraBoom->SetRelativeRotation(FRotator(-75.f, 0.f, 0.f));
}


void AXkTopDownCamera::AddCameraZoom(const float InputValue, const float Speed)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddCameraZoom, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddCameraZoom);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddCameraZoom);

	if (!ensure(GetWorld()))
	{
		return;
	}
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	float TargetArmLength = CameraBoom->TargetArmLength;
	TargetArmLength += (InputValue * Speed * DeltaSeconds);
	TargetArmLength = FMath::Clamp(TargetArmLength, CameraZoomArmRange.X, CameraZoomArmRange.Y);
	CameraBoom->TargetArmLength = TargetArmLength;
}


void AXkTopDownCamera::ResetCameraZoom()
{
	CameraBoom->TargetArmLength = CameraZoomArmLength;
}


void AXkTopDownCamera::AddMoveTarget(const FVector& InTarget)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_AddMoveTarget, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_AddMoveTarget);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_AddMoveTarget);

	MovementTarget = InTarget;
	bMoveToTarget = true;
}


void AXkTopDownCamera::MoveToTarget(const FVector& InTarget, const bool bImmediately)
{
	SCOPED_NAMED_EVENT(AXkTopDownCamera_MoveToTarget, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkTopDownCamera_MoveToTarget);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkTopDownCamera_MoveToTarget);
	if (CameraBoom->bEnableCameraLag && bImmediately)
	{
		CameraBoom->bEnableCameraLag = false;
		SetActorLocation(InTarget);
		GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateLambda([this]()
				{
					CameraBoom->bEnableCameraLag = true;
				})
		);
	}
	else
	{
		SetActorLocation(InTarget);
	}
}


FRotator AXkTopDownCamera::GetForwardRotator() const
{
	FTransform ComponentToWorld = CameraBoom->GetComponentToWorld();
	FRotator Rotator = ComponentToWorld.Rotator();
	Rotator.Pitch = 0.0; Rotator.Roll = 0.0;
	return Rotator;
}


void AXkTopDownCamera::SetTravelingMode(const bool bInTravelingMode)
{
	if (bInTravelingMode)
	{
		CameraBoom->SetRelativeRotation(TravelingView);
		CameraBoom->TargetArmLength = TravelingZoom;
		Velocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
	}
	else
	{
		ResetRotation();
		Velocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
	}
	bTravelingMode = bInTravelingMode;
}