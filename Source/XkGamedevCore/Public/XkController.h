// Copyright ©xukai. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DecalActor.h"
#include "InputActionValue.h"
#include "XkController.generated.h"


UENUM(BlueprintType)
enum class EXkControlsFlavor : uint8
{
	Keyboard,
	Gamepad,
	Touchpad,
	None
};


UENUM(BlueprintType)
enum class EXkControlsCursorArea: uint8
{
	SafeArea = 0,
	TopArea,
	DownArea,
	LeftArea,
	RightArea
};


UCLASS()
class XKGAMEDEVCORE_API AXkGamepadCursor : public ADecalActor
{
	GENERATED_BODY()

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Radius;

public:
	/** Default UObject constructor. */
	AXkGamepadCursor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	FORCEINLINE virtual void AddMovement(const FVector2D& InputValue, const FRotator& ForwardRotator, const float Speed);
	FORCEINLINE virtual void AddMovement(const FVector& InputValue, const FRotator& ForwardRotator, const float Speed);

	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (bTraceComplex = true))
	bool GetHitResultUnderGamepadCursor(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;
	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXU GAMEDEV]")
	void SetVisibility(const bool Input);

private:
	bool bMovementFreeze = false;
};


UCLASS()
class XKGAMEDEVCORE_API AXkController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Default UObject constructor. */
	AXkController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float ShortPressThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraScrollingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraDraggingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraRotatingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraZoomingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraMinimalHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float CameraMaximalHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float MouseDraggingSensibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float MouseRotatingSensibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]")
	float GamepadCursorMovingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategic Chess Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* GamepadCursorMaterial;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetSelectionClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetSelectionTouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetDeselectionClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetDeselectionTouchAction;

	/** Camera Dragging Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraDraggingAction;

	/** Camera Dragging Press Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraDraggingPressAction;

	/** Camera Rotating Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraRotatingAction;

	/** Camera Rotating Press Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraRotatingPressAction;

	/** Camera Scrolling Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraZoomingAction;

	/** Camera Scrolling Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetGamepadCursorMovementAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetGamepadMenuSwitchAction;

	/** Event delegate for when the controls flavor for the UI has changed. */
	DECLARE_EVENT_OneParam(AXkController, FControlsFlavorChangedEvent, const EXkControlsFlavor);
	FControlsFlavorChangedEvent& OnControlsFlavorChanged() { return ControlsFlavorChangedEvent; };

	/** Returns the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXU GAMEDEV]")
	virtual EXkControlsFlavor GetControlsFlavor() const { return ControlsFlavor; };

	/** Sets the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXU GAMEDEV]")
	virtual EXkControlsFlavor SetControlsFlavor(const EXkControlsFlavor NewControlsFlavor);

	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXU GAMEDEV]")
	virtual FVector2D GetControlsCursorPositionOnScreen() const;

	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXU GAMEDEV]")
	virtual EXkControlsCursorArea GetControlsCursorArea() const { return ControlsCursorArea; };

protected:
	virtual void SetupInputComponent() override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	/** Input handlers for SetDestination action. */
	virtual void OnInputStarted();
	virtual void OnInputPressing();
	virtual void OnSetSelectionStarted() { OnInputStarted(); };
	virtual void OnSetSelectionTriggered() {};
	virtual void OnSetSelectionPressing() { OnInputPressing(); };
	virtual void OnSetSelectionReleased() {};
	virtual void OnSetDeselectionStarted() { OnInputStarted(); };
	virtual void OnSetDeselectionTriggered() {};
	virtual void OnSetDeselectionPressing() { OnInputPressing(); };
	virtual void OnSetDeselectionReleased() {};
	virtual void OnSetCameraDraggingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraDraggingPressing(const FInputActionValue& Value);
	virtual void OnSetCameraDraggingReleased();
	virtual void OnSetCameraRotatingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraRotatingPressing(const FInputActionValue& Value);
	virtual void OnSetCameraRotatingReleased();
	virtual void OnSetCameraZoomingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraZoomingPressing(const FInputActionValue& Value) {};
	virtual void OnSetCameraZoomingReleased() {};
	virtual void OnSetGamepadCursorMovementTriggered(const FInputActionValue& Value);
	virtual void OnSetGamepadCursorMovementPressing(const FInputActionValue& Value);
	virtual void OnSetGamepadCursorMovementReleased();
	virtual void OnSetGamepadMenuSwitchTriggered(const FInputActionValue& Value);
	virtual void OnSetGamepadMenuSwitchPressing(const FInputActionValue& Value);
	virtual void OnSetGamepadMenuSwitchReleased();
	virtual void OnTouchTriggered();
	virtual void OnTouchReleased();

	UPROPERTY(Transient)
	EXkControlsFlavor ControlsFlavor;
	FControlsFlavorChangedEvent ControlsFlavorChangedEvent;

	UPROPERTY(Transient)
	EXkControlsCursorArea ControlsCursorArea;

	UPROPERTY(Transient)
	FVector2D CachedMouseCursorLocation;
	UPROPERTY(Transient)
	FVector CachedGamepadCursorLocation;
	UPROPERTY(Transient)
	TWeakObjectPtr<class AXkGamepadCursor> GamepadCursor;

	bool bIsCameraDraggingButtonPressing;
	bool bIsCameraRotatingButtonPressing;
	float FollowTime; // For how long it has been pressed

public:
	virtual bool IsShortPress();
	virtual bool ControllerSelect() const;
	virtual bool ControllerSelect(FHitResult& Hit) const;
	virtual bool ControllerSelect(FHitResult& Hit, const ECollisionChannel Channel) const;
	virtual bool ControllerSelect(FVector& HitLocation, FVector& WorldPosition, FVector& WorldDirection) const;
	virtual bool ControllerSelect(FVector& HitLocation, FVector& WorldPosition, FVector& WorldDirection, const ECollisionChannel Channel) const;
	virtual bool IsMouseCursorMoving() const;
	virtual EMouseCursor::Type GetMouseCursorType() const;
	virtual void SetMouseCursorType(const EMouseCursor::Type MouseCursor);
	virtual bool IsGamepadCursorMoving() const;
	virtual void SetGamepadCursorVisibility(const bool Input);
	virtual class AXkGameState* GetGameState() const;
	virtual class AXkTopDownCamera* GetTopDownCamera() const;
};


