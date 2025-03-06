// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkController.h"
#include "XkCamera.h"
#include "XkCharacter.h"
#include "XkGameState.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/CanvasPanelSlot.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"



AXkParabolaCurve::AXkParabolaCurve(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	ParabolaSpline = CreateDefaultSubobject<USplineComponent>(TEXT("ParabolaSpline"));
	ParabolaSpline->SetupAttachment(RootComponent);
	ParabolaSpline->SetClosedLoop(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder(TEXT("/XkGamedevKit/Meshes/SM_SplineMeshTube.SM_SplineMeshTube"));
	ParabolaMesh = ObjectFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder2(TEXT("/XkGamedevKit/Materials/M_GuidelineParabolaMesh.M_GuidelineParabolaMesh"));
	ParabolaMeshMaterial = ObjectFinder2.Object;

	ParabolaStartScale = 0.075;
	ParabolaEndScale = 0.075;
	ParabolaPointsNum = 10;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}


void AXkParabolaCurve::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ParabolaMeshMaterial && IsValid(ParabolaMeshMaterial))
	{
		ParabolaMeshMaterialDyn = UMaterialInstanceDynamic::Create(ParabolaMeshMaterial, this);
	}
}


void AXkParabolaCurve::UpdateParabolaCurve(const FVector& Start, const FVector& End, const float ParaCurveArc)
{
	ParabolaSpline->ClearSplinePoints();
	TArray<FVector> Points = UXkTargetMovement::CalcParaCurvePoints(Start, End, ParaCurveArc, ParabolaPointsNum);
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		ParabolaSpline->AddSplinePoint(Points[i], ESplineCoordinateSpace::World);
	}

	TArray<TObjectPtr<class USplineMeshComponent>> TempSplineMeshes;
	for (int32 Index = 0; Index < (ParabolaSpline->GetNumberOfSplinePoints() - 1); ++Index)
	{
		USplineMeshComponent* SplineMesh = ParabolaSplineMeshes.IsValidIndex(Index) ? ParabolaSplineMeshes[Index] : nullptr;
		if (!SplineMesh || !IsValid(SplineMesh))
		{
			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(ParabolaSpline, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(ParabolaMesh);
			SplineMesh->SetForwardAxis(ESplineMeshAxis::X);
			SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SplineMesh->RegisterComponent();
			SplineMesh->SetMaterial(0, ParabolaMeshMaterialDyn);
			SplineMesh->SetTranslucentSortPriority(ParabolaSortPriority);
			AddInstanceComponent(SplineMesh); // Component Would Display on Setup.
		}
		SplineMesh->SetStartAndEnd(ParabolaSpline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local),
			ParabolaSpline->GetTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local),
			ParabolaSpline->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local),
			ParabolaSpline->GetTangentAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local));
		SplineMesh->SetStartScale(FVector2D(ParabolaStartScale));
		SplineMesh->SetEndScale(FVector2D(ParabolaEndScale));
		TempSplineMeshes.Add(SplineMesh);
	}

	for (int32 Index = 0; Index < ParabolaSplineMeshes.Num(); ++Index)
	{
		USplineMeshComponent* SplineMesh = ParabolaSplineMeshes[Index];
		if (SplineMesh && IsValid(SplineMesh) && !TempSplineMeshes.Contains(SplineMesh))
		{
			SplineMesh->DestroyComponent();
		}
	}
	ParabolaSplineMeshes = TempSplineMeshes;
}


void AXkParabolaCurve::SetParabolaCurveColor(const FLinearColor& Color)
{
	if (ParabolaMeshMaterialDyn && IsValid(ParabolaMeshMaterialDyn))
	{
		ParabolaMeshMaterialDyn->SetVectorParameterValue(FName(TEXT("Color")), Color);
	}
}


AXkGamepadCursor::AXkGamepadCursor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder(TEXT("/XkGamedevKit/Materials/M_CursorFocus"));
	GetDecal()->SetDecalMaterial(ObjectFinder.Object);
	GetDecal()->DecalSize = FVector(512, 128, 128);
	Radius = 50.0;
}


void AXkGamepadCursor::AddMovement(const FVector2D& InputValue, const FRotator& ForwardRotator, const float Speed)
{
	FVector MovementVector = FVector(InputValue.Y, InputValue.X, 0.0);
	AddMovement(MovementVector, ForwardRotator, Speed);
}


void AXkGamepadCursor::AddMovement(const FVector& InputValue, const FRotator& ForwardRotator, const float Speed)
{
	if (!ensure(GetWorld()))
	{
		return;
	}
	if (!GetDecal()->IsVisible())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!IsValid(PlayerController))
	{
		return;
	}

	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	FVector2D ViewportSize, ScreenPosition;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
	FVector ActorLocation = GetActorLocation();
	if (UGameplayStatics::ProjectWorldToScreen(PlayerController, ActorLocation, ScreenPosition))
	{
		if (ScreenPosition.X < 0 || ScreenPosition.X > ViewportSize.X ||
			ScreenPosition.Y < 0 || ScreenPosition.Y > ViewportSize.Y)
		{
			FVector2D NewLocationOnScreen = ViewportSize / 2.0;
			FVector NewLocation, NewDirection;
			UGameplayStatics::DeprojectScreenToWorld(PlayerController, NewLocationOnScreen, NewLocation, NewDirection);
			FHitResult HitResult;
			if (GetWorld()->LineTraceSingleByChannel(HitResult, NewLocation, NewLocation + NewDirection * UE_FLOAT_HUGE_DISTANCE,
				ECollisionChannel::ECC_Visibility))
			{
				SetActorLocation(HitResult.ImpactPoint);
			}
			
		}
	}
		

	FVector MovementVectorRotated = ForwardRotator.RotateVector(InputValue);
	FVector NewWorldLocation = MovementVectorRotated * Speed * DeltaSeconds + GetActorLocation();
	FHitResult HitResult;
	if (GetHitResultUnderGamepadCursor(ECollisionChannel::ECC_Visibility, false, HitResult))
	{
		FVector ImpactPoint = HitResult.ImpactPoint;
		NewWorldLocation.Z = ImpactPoint.Z;
	}
	if (UGameplayStatics::ProjectWorldToScreen(PlayerController, NewWorldLocation, ScreenPosition))
	{
		// @DEBUG: AddMovement
		//FString Message = FString::Printf(TEXT("ScreenPosition (%0.01f, %0.01f) ViewportSize (%0.01f, %0.01f)"), ScreenPosition.X, ScreenPosition.Y,
		//	ViewportSize.X, ViewportSize.Y);
		//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Red, *Message);
		if (ScreenPosition.X > 0 && ScreenPosition.X < ViewportSize.X &&
			ScreenPosition.Y > 0 && ScreenPosition.Y < ViewportSize.Y)
		{
			SetActorLocation(NewWorldLocation);
		}
	}
}


bool AXkGamepadCursor::GetHitResultUnderGamepadCursor(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const
{
	if (!ensure(GetWorld()))
	{
		return false;
	}
	FVector Start = GetActorLocation() + FVector(0.0, 0.0, UE_FLOAT_HUGE_DISTANCE);
	FVector End = GetActorLocation() + FVector(0.0, 0.0, -UE_FLOAT_HUGE_DISTANCE);
	return GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(Radius));
}


void AXkGamepadCursor::SetVisibility(const bool Input)
{
	GetDecal()->SetVisibility(Input);
}


AXkController::AXkController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GamepadCursorClass = AXkGamepadCursor::StaticClass();
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	ShortPressThreshold = 0.1;
	MaxHoveringThreshold = 0.25;
	CameraScrollingSpeed = 1000.0;
	CameraDraggingSpeed = 1500;
	CameraRotatingSpeed = 120.0;
	CameraZoomingSpeed = 2000.0;
	MouseDraggingSensibility = 1.5;
	MouseRotatingSensibility = 1.0;
	GamepadCursorMovingSpeed = 1000.0;

	FollowTime = 0.f;
	bIsCameraDraggingButtonPressing = false;
	bIsCameraRotatingButtonPressing = false;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> ObjectFinder(TEXT("/XkGamedevKit/Inputs/IMC_XkController"));
	DefaultMappingContext = ObjectFinder.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder2(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetSelectionClick"));
	SetSelectionClickAction = ObjectFinder2.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder3(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetSelectionTouch"));
	SetSelectionTouchAction = ObjectFinder3.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder4(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetDeselectionClick"));
	SetDeselectionClickAction = ObjectFinder4.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder5(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetDeselectionTouch"));
	SetDeselectionTouchAction = ObjectFinder5.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder6(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetCameraDragging"));
	SetCameraDraggingAction = ObjectFinder6.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder7(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetCameraDraggingPress"));
	SetCameraDraggingPressAction = ObjectFinder7.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder8(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetCameraRotating"));
	SetCameraRotatingAction = ObjectFinder8.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder9(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetCameraRotatingPress"));
	SetCameraRotatingPressAction = ObjectFinder9.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder10(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetCameraZooming"));
	SetCameraZoomingAction = ObjectFinder10.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder11(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetGamepadCursorMovement"));
	SetGamepadCursorMovementAction = ObjectFinder11.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> ObjectFinder12(TEXT("/XkGamedevKit/Inputs/Actions/IA_SetGamepadSwitch"));
	SetGamepadSwitchAction = ObjectFinder12.Object;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


void AXkController::SetControlsFlavor(const EXkControlsFlavor NewControlsFlavor)
{
	if (ControlsFlavor != NewControlsFlavor)
	{
		ControlsFlavor = NewControlsFlavor;
		if (NewControlsFlavor == EXkControlsFlavor::Gamepad)
		{
			CurrentMouseCursor = EMouseCursor::None;
			FHitResult HitResult;
			// we spawn game pad cursor at zero, when game pad cursor move, it would automatically
			// jump to the center of screen if it's not on screen
			FVector HitPoint = FVector::ZeroVector;
			if (ControllerSelect(HitResult, ECollisionChannel::ECC_WorldStatic))
			{
				HitPoint = HitResult.ImpactPoint;
			}
			if (ensure(GetWorld()) && GamepadCursorClass)
			{
				AXkGamepadCursor* SpawnedActor = GetWorld()->SpawnActor<AXkGamepadCursor>(GamepadCursorClass, HitPoint, FRotator::ZeroRotator);
				GamepadCursor = MakeWeakObjectPtr(SpawnedActor);
			}
			check(GamepadCursor.IsValid());
		}
		else if (NewControlsFlavor == EXkControlsFlavor::Keyboard)
		{
			CurrentMouseCursor = EMouseCursor::Default;
			FollowTime = 0.f;
			bIsCameraDraggingButtonPressing = false;
			bIsCameraRotatingButtonPressing = false;
			if (GamepadCursor.IsValid())
			{
				GamepadCursor->Destroy();
				GamepadCursor.Reset();
			}
		}
		// Fix Mouse Cursor not changing until moved
		// @see https://forums.unrealengine.com/t/mouse-cursor-not-changing-until-moved/290523/13
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
		FSlateApplication::Get().QueryCursor();
		ControlsFlavorChangedEvent.Broadcast(NewControlsFlavor);
	}
}


EMouseCursor::Type AXkController::GetMouseCursorType() const
{
	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		return CurrentMouseCursor;
	}
	return EMouseCursor::None;
}


void AXkController::SetMouseCursorType(const EMouseCursor::Type MouseCursor)
{
	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		CurrentMouseCursor = MouseCursor;
	}
	SetShowMouseCursor(MouseCursor == EMouseCursor::Type::None ? false : true);
}


bool AXkController::IsShortPress()
{
	if (FollowTime > ShortPressThreshold)
	{
		FollowTime = 0.0f;
	}
	if (FollowTime == 0.0f)
	{
		return true;
	}
	return false;
}


void AXkController::SetupInputComponent()
{
	SCOPED_NAMED_EVENT(AXkController_SetupInputComponent, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SetupInputComponent);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_SetupInputComponent);

	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events of selection
		EnhancedInputComponent->BindAction(SetSelectionClickAction, ETriggerEvent::Started, this, &AXkController::OnSetSelectionStarted);
		EnhancedInputComponent->BindAction(SetSelectionClickAction, ETriggerEvent::Triggered, this, &AXkController::OnSetSelectionTriggered);
		EnhancedInputComponent->BindAction(SetSelectionClickAction, ETriggerEvent::Triggered, this, &AXkController::OnSetSelectionPressing);
		EnhancedInputComponent->BindAction(SetSelectionClickAction, ETriggerEvent::Completed, this, &AXkController::OnSetSelectionReleased);
		EnhancedInputComponent->BindAction(SetSelectionClickAction, ETriggerEvent::Canceled, this, &AXkController::OnSetSelectionReleased);

		// Setup mouse input events of deselection
		EnhancedInputComponent->BindAction(SetDeselectionClickAction, ETriggerEvent::Started, this, &AXkController::OnSetDeselectionStarted);
		EnhancedInputComponent->BindAction(SetDeselectionClickAction, ETriggerEvent::Triggered, this, &AXkController::OnSetDeselectionTriggered);
		EnhancedInputComponent->BindAction(SetDeselectionClickAction, ETriggerEvent::Triggered, this, &AXkController::OnSetDeselectionPressing);
		EnhancedInputComponent->BindAction(SetDeselectionClickAction, ETriggerEvent::Completed, this, &AXkController::OnSetDeselectionReleased);
		EnhancedInputComponent->BindAction(SetDeselectionClickAction, ETriggerEvent::Canceled, this, &AXkController::OnSetDeselectionReleased);

		EnhancedInputComponent->BindAction(SetCameraDraggingAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetCameraDraggingAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraDraggingTriggered);
		EnhancedInputComponent->BindAction(SetCameraDraggingPressAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraDraggingPressing);
		EnhancedInputComponent->BindAction(SetCameraDraggingPressAction, ETriggerEvent::Completed, this, &AXkController::OnSetCameraDraggingReleased);
		EnhancedInputComponent->BindAction(SetCameraDraggingPressAction, ETriggerEvent::Canceled, this, &AXkController::OnSetCameraDraggingReleased);

		EnhancedInputComponent->BindAction(SetCameraRotatingAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetCameraRotatingAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraRotatingTriggered);
		EnhancedInputComponent->BindAction(SetCameraRotatingPressAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraRotatingPressing);
		EnhancedInputComponent->BindAction(SetCameraRotatingPressAction, ETriggerEvent::Completed, this, &AXkController::OnSetCameraRotatingReleased);
		EnhancedInputComponent->BindAction(SetCameraRotatingPressAction, ETriggerEvent::Canceled, this, &AXkController::OnSetCameraRotatingReleased);

		EnhancedInputComponent->BindAction(SetCameraZoomingAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetCameraZoomingAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraZoomingTriggered);
		EnhancedInputComponent->BindAction(SetCameraZoomingAction, ETriggerEvent::Triggered, this, &AXkController::OnSetCameraZoomingPressing);
		EnhancedInputComponent->BindAction(SetCameraZoomingAction, ETriggerEvent::Completed, this, &AXkController::OnSetCameraZoomingReleased);
		EnhancedInputComponent->BindAction(SetCameraZoomingAction, ETriggerEvent::Canceled, this, &AXkController::OnSetCameraZoomingReleased);


		EnhancedInputComponent->BindAction(SetGamepadCursorMovementAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetGamepadCursorMovementAction, ETriggerEvent::Triggered, this, &AXkController::OnSetGamepadCursorMovementTriggered);
		EnhancedInputComponent->BindAction(SetGamepadCursorMovementAction, ETriggerEvent::Triggered, this, &AXkController::OnSetGamepadCursorMovementPressing);
		EnhancedInputComponent->BindAction(SetGamepadCursorMovementAction, ETriggerEvent::Completed, this, &AXkController::OnSetGamepadCursorMovementReleased);
		EnhancedInputComponent->BindAction(SetGamepadCursorMovementAction, ETriggerEvent::Canceled, this, &AXkController::OnSetGamepadCursorMovementReleased);

		EnhancedInputComponent->BindAction(SetGamepadSwitchAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetGamepadSwitchAction, ETriggerEvent::Triggered, this, &AXkController::OnSetGamepadSwitchTriggered);
		EnhancedInputComponent->BindAction(SetGamepadSwitchAction, ETriggerEvent::Triggered, this, &AXkController::OnSetGamepadSwitchPressing);
		EnhancedInputComponent->BindAction(SetGamepadSwitchAction, ETriggerEvent::Completed, this, &AXkController::OnSetGamepadSwitchReleased);
		EnhancedInputComponent->BindAction(SetGamepadSwitchAction, ETriggerEvent::Canceled, this, &AXkController::OnSetGamepadSwitchReleased);

		// Setup touch input events
		EnhancedInputComponent->BindAction(SetSelectionTouchAction, ETriggerEvent::Started, this, &AXkController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetSelectionTouchAction, ETriggerEvent::Triggered, this, &AXkController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetSelectionTouchAction, ETriggerEvent::Completed, this, &AXkController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetSelectionTouchAction, ETriggerEvent::Canceled, this, &AXkController::OnTouchReleased);
	}
}


void AXkController::BeginPlay()
{
	SCOPED_NAMED_EVENT(AXkController_BeginPlay, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_BeginPlay);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_BeginPlay);

	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	ControlsFlavor = EXkControlsFlavor::None;

	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer)
	{
		LocalPlayer->ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
		LocalPlayer->ViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
		// when SetHideCursorDuringCapture, mouse capture UseHighPrecisionMouseMovement(ViewportWidgetRef), and it got a smooth movement.
		LocalPlayer->ViewportClient->SetHideCursorDuringCapture(true);
	}
}


void AXkController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	SCOPED_NAMED_EVENT(AXkController_TickActor, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_TickActor);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_TickActor);

	HoveringTime = (IsMouseCursorMoving() || IsGamepadCursorMoving()) ? 0.0f : (HoveringTime + DeltaTime);

	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		FVector2D MouseLocation;
		////////////////////////////////////////////////////////
		//  (1, 1)---------X---------(X, 1)
		//		 |                   |
		//		 Y                   Y
		//		 |                   |
		//  (1, Y)---------X---------(X, Y)
		if (GetMousePosition(MouseLocation.X, MouseLocation.Y)
			&& !bIsCameraDraggingButtonPressing && !bIsCameraRotatingButtonPressing)
		{
			// Mouse move screen
			int32 ViewportSizeX, ViewportSizeY;
			GetViewportSize(ViewportSizeX, ViewportSizeY);

			float PercentX = MouseLocation.X / (float)ViewportSizeX;
			float PercentY = MouseLocation.Y / (float)ViewportSizeY;
			if (!IsOnUI() && (PercentX < 0.01 || PercentX > 0.99 || PercentY < 0.01 || PercentY > 0.99))
			{
				FVector2D MovementVector = FVector2D::ZeroVector;
				if (PercentX < 0.01)
				{
					ControlsCursorArea = EXkControlsCursorArea::LeftArea;
					MovementVector += FVector2D(0, -1);
				}
				if (PercentX > 0.99)
				{
					ControlsCursorArea = EXkControlsCursorArea::RightArea;
					MovementVector += FVector2D(0, 1);
				}
				if (PercentY < 0.01)
				{
					ControlsCursorArea = EXkControlsCursorArea::TopArea;
					MovementVector += FVector2D(1, 0);
				}
				if (PercentY > 0.99)
				{
					ControlsCursorArea = EXkControlsCursorArea::DownArea;
					MovementVector += FVector2D(-1, 0);
				}
				// Move towards mouse pointer
				if (AXkTopDownCamera* TopDownCamera = GetTopDownCamera())
				{
					TopDownCamera->AddMovement(FVector(MovementVector.X, MovementVector.Y, 0.0), CameraScrollingSpeed);
				}
			}
			else
			{
				ControlsCursorArea = EXkControlsCursorArea::SafeArea;
			}

			CachedMouseCursorLocation = MouseLocation;
		}
	}
	else if (ControlsFlavor == EXkControlsFlavor::Gamepad)
	{
		if (GamepadCursor.IsValid())
		{
			CachedGamepadCursorLocation = GamepadCursor->GetActorLocation();
		}
	}

	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}


FVector2D AXkController::GetControlsCursorPositionOnScreen() const
{
	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		FVector2D MousePosition;
		if (GetMousePosition(MousePosition.X, MousePosition.Y))
		{
			return MousePosition;
		}
	}
	else if (ControlsFlavor == EXkControlsFlavor::Gamepad)
	{
		if (GamepadCursor.IsValid())
		{
			FVector2D ScreenSpaceLocation;
			FVector Location = GamepadCursor->GetActorLocation();
			if (ProjectWorldLocationToScreen(Location, ScreenSpaceLocation))
			{
				return ScreenSpaceLocation;
			}
		}
	}
	return FVector2D(1, 1);
}


void AXkController::OnInputStarted()
{
	StopMovement();
	FollowTime = 0.f;
}


void AXkController::OnInputPressing()
{
	FollowTime += GetWorld()->GetDeltaSeconds();
}


void AXkController::OnSetCameraDraggingTriggered(const FInputActionValue& Value)
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraDraggingTriggered, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraDraggingTriggered);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraDraggingTriggered);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	if (bIsCameraDraggingButtonPressing)
	{
		// input is a Vector2D
		FVector2D MovementVector = Value.Get<FVector2D>();
		MovementVector.Normalize();
		if (ControlsFlavor == EXkControlsFlavor::Keyboard)
		{
			MovementVector *= MouseDraggingSensibility;
		}
		// Move towards mouse pointer
		if (AXkTopDownCamera* TopDownCamera = GetTopDownCamera())
		{
			// @DEBUG: OnSetCameraDraggingTriggered
			//FString Message = FString::Printf(TEXT("OnSetCameraDraggingTriggered[%0.02f,%0.02f]"), MovementValue.X, MovementValue.Y);
			//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Red, *Message);

			TopDownCamera->AddMovement(MovementVector, CameraDraggingSpeed);
		}
	}
}


void AXkController::OnSetCameraDraggingPressing(const FInputActionValue& Value)
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraDraggingPressing, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraDraggingPressing);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraDraggingPressing);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();
	MovementVector.Normalize();
	// dead zoom
	if (FMath::Abs(MovementVector.X) < 0.5 && FMath::Abs(MovementVector.Y) < 0.5)
	{
		return;
	}

	// @DEBUG: Dead Zoom
	//FString Message = FString::Printf(TEXT("OnSetCameraDraggingPressing[%0.02f, %0.02f]"), MovementValue.X, MovementValue.Y);
	//GEngine->AddOnScreenDebugMessage(-1, 0.1, FColor::Green, *Message);

	if (!bIsCameraRotatingButtonPressing)
	{
		bIsCameraDraggingButtonPressing = true;

		// Move towards mouse pointer
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn != nullptr)
		{
			SetMouseCursorType(EMouseCursor::GrabHand);
		}
	}
}


void AXkController::OnSetCameraDraggingReleased()
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraDraggingReleased, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraDraggingReleased);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraDraggingReleased);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	bIsCameraDraggingButtonPressing = false;
	SetMouseCursorType(EMouseCursor::Default);
}


void AXkController::OnSetCameraRotatingTriggered(const FInputActionValue& Value)
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraRotatingTriggered, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraRotatingTriggered);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraRotatingTriggered);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	MovementVector.Normalize();
	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		MovementVector *= MouseRotatingSensibility;
	}
	if (bIsCameraRotatingButtonPressing)
	{
		// Move towards mouse pointer
		if (AXkTopDownCamera* TopDownCamera = GetTopDownCamera())
		{
			TopDownCamera->AddRotation(MovementVector, CameraRotatingSpeed);
		}
	}
}


void AXkController::OnSetCameraRotatingPressing(const FInputActionValue& Value)
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraRotatingPressing, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraRotatingPressing);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraRotatingPressing);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	bIsCameraRotatingButtonPressing = true;
	bIsCameraDraggingButtonPressing = false;
	// Move towards mouse pointer
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		SetMouseCursorType(EMouseCursor::None);
	}
}


void AXkController::OnSetCameraRotatingReleased()
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraRotatingReleased, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraRotatingReleased);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraRotatingReleased);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	bIsCameraRotatingButtonPressing = false;

	if (GetMouseCursor() == EMouseCursor::None)
	{
		SetMouseLocation(CachedMouseCursorLocation.X, CachedMouseCursorLocation.Y);
	}
	SetMouseCursorType(EMouseCursor::Default);
}


void AXkController::OnSetCameraZoomingTriggered(const FInputActionValue& Value)
{
	SCOPED_NAMED_EVENT(AXkController_OnSetCameraZoomingTriggered, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_OnSetCameraZoomingTriggered);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_OnSetCameraZoomingTriggered);

	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	float ZoomingValue = Value.Get<float>();
	if (AXkTopDownCamera * TopDownCamera = GetTopDownCamera())
	{
		TopDownCamera->AddCameraZoom(ZoomingValue, CameraZoomingSpeed);
	}
}


void AXkController::OnSetGamepadCursorMovementTriggered(const FInputActionValue& Value)
{
	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();
	// Move towards mouse pointer
	if (AXkTopDownCamera* TopDownCamera = GetTopDownCamera())
	{
		if (GamepadCursor.IsValid())
		{
			FRotator ForwardRotator = TopDownCamera->GetForwardRotator();
			GamepadCursor->AddMovement(MovementVector, ForwardRotator, GamepadCursorMovingSpeed);
		}
	}
}


void AXkController::OnSetGamepadCursorMovementPressing(const FInputActionValue& Value)
{
	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}
}


void AXkController::OnSetGamepadCursorMovementReleased()
{
	if (IsOnUI())
	{
		// Don't do anything if we are on UI mode.
		return;
	}
}


void AXkController::OnSetGamepadSwitchTriggered(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	// @DEBUG: OnSetGamepadSwitchTriggered
	//if (FollowTime == 0.0f)
	//{
	//	FString Message = FString::Printf(TEXT("OnSetGamepadSwitchTriggered (%0.2f,%0.2f)"), MovementVector.X, MovementVector.Y);
	//	GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Green, *Message);
	//}
}


void AXkController::OnSetGamepadSwitchPressing(const FInputActionValue& Value)
{
	OnInputPressing();
}


void AXkController::OnSetGamepadSwitchReleased()
{
}


// Triggered every frame when the input is held down
void AXkController::OnTouchTriggered()
{
	OnSetSelectionTriggered();
}


void AXkController::OnTouchReleased()
{
	OnSetSelectionReleased();
}


bool AXkController::ControllerSelect() const
{
	// We look for the location in the world where the player has pressed the input
	FHitResult HitResult;
	return ControllerSelect(HitResult);
}


bool AXkController::ControllerSelect(FHitResult& Hit) const
{
	return ControllerSelect(Hit, ECollisionChannel::ECC_Visibility);
}


bool AXkController::ControllerSelect(FHitResult& Hit, const ECollisionChannel Channel) const
{
	SCOPED_NAMED_EVENT(AXkController_ControllerSelect, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_AXkController_ControllerSelect);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_AXkController_ControllerSelect);

	// We look for the location in the world where the player has pressed the input
	bool bHitSuccessful = false;
	if (ControlsFlavor == EXkControlsFlavor::Touchpad)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, Channel, true, Hit);
	}
	else if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		bHitSuccessful = GetHitResultUnderCursor(Channel, true, Hit);
	}
	else if (ControlsFlavor == EXkControlsFlavor::Gamepad)
	{
		if (GamepadCursor.IsValid())
		{
			bHitSuccessful = GamepadCursor->GetHitResultUnderGamepadCursor(Channel, true, Hit);
		}
		else
		{
			bHitSuccessful = GetHitResultUnderCursor(Channel, true, Hit);
		}
	}
	return bHitSuccessful;
}


bool AXkController::ControllerSelect(FVector& HitLocation, FVector& WorldPosition, FVector& WorldDirection) const
{
	return ControllerSelect(HitLocation, WorldPosition, WorldDirection, ECollisionChannel::ECC_Visibility);
}


bool AXkController::ControllerSelect(FVector& HitLocation, FVector& WorldPosition, FVector& WorldDirection, const ECollisionChannel Channel) const
{
	FHitResult HitResult;
	if (ControllerSelect(HitResult, Channel))
	{
		HitLocation = HitResult.Location;
		if (DeprojectMousePositionToWorld(WorldPosition, WorldDirection))
		{
			return true;
		}
	}
	return false;
}


bool AXkController::IsMouseCursorMoving() const
{
	if (ControlsFlavor == EXkControlsFlavor::Keyboard)
	{
		// if camera dragging move or rotating, stop Hovered detected
		if (!bIsCameraDraggingButtonPressing && !bIsCameraRotatingButtonPressing)
		{
			FVector2D MouseLocation;
			return (GetMousePosition(MouseLocation.X, MouseLocation.Y) && FVector2D::Distance(MouseLocation, CachedMouseCursorLocation) >= 3.0);
		}
	}
	return false;
}


bool AXkController::IsGamepadCursorMoving() const
{
	if (ControlsFlavor == EXkControlsFlavor::Gamepad)
	{
		if (GamepadCursor.IsValid())
		{
			FVector GamepadCursorLocation = GamepadCursor->GetActorLocation();
			return (FVector::Distance(GamepadCursorLocation, CachedGamepadCursorLocation) >= 3.0);
		}
	}
	return false;
}


void AXkController::SetGamepadCursorVisibility(const bool Input)
{
	if (GamepadCursor.IsValid())
	{
		GamepadCursor->SetVisibility(Input);
	}
}


class AXkGameState* AXkController::GetGameState() const
{
	if (GetWorld() != NULL)
	{
		return Cast<AXkGameState>(UGameplayStatics::GetGameState(GetWorld()));
	}
	return nullptr;
}


class AXkTopDownCamera* AXkController::GetTopDownCamera() const
{
	APawn* ControlledPawn = GetPawn();
	AXkTopDownCamera* TopDownCamera = Cast<AXkTopDownCamera>(ControlledPawn);
	if (TopDownCamera && IsValid(TopDownCamera))
	{
		return TopDownCamera;
	}
	return nullptr;
}
