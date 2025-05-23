// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DecalActor.h"
#include "InputActionValue.h"
#include "XkController.generated.h"

UENUM(BlueprintType, Blueprintable)
enum class EXkControlsMode : uint8
{
	RealTime = 0,
	TurnBased,
};


UENUM(BlueprintType, Blueprintable)
enum class EXkControlsFlavor : uint8
{
	Keyboard,
	Gamepad,
	Touchpad,
	None
};


UENUM(BlueprintType, Blueprintable)
enum class EXkControlsCursorArea: uint8
{
	SafeArea = 0,
	TopArea,
	DownArea,
	LeftArea,
	RightArea
};


UCLASS(BlueprintType , Blueprintable)
class XKGAMEDEVCORE_API AXkParabolaCurve : public AActor
{
	GENERATED_BODY()

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneComponent> RootScene;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USplineComponent> ParabolaSpline;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<class USplineMeshComponent*> ParabolaSplineMeshes;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMesh* ParabolaMesh;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* ParabolaMeshMaterial;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* ParabolaMeshMaterialDyn;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float ParabolaStartScale;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float ParabolaEndScale;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 ParabolaPointsNum;

	UPROPERTY(Category = "GuideLine [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 ParabolaSortPriority;

public:
	/** Default UObject constructor. */
	AXkParabolaCurve(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "GuideLine [KEVINTSUIXUGAMEDEV]")
	virtual void UpdateParabolaCurve(const FVector& Start, const FVector& End, const float ParaCurveArc);


	UFUNCTION(BlueprintCallable, Category = "GuideLine [KEVINTSUIXUGAMEDEV]")
	void SetParabolaCurveColor(const FLinearColor& Color);

	UFUNCTION(BlueprintCallable, Category = "GuideLine [KEVINTSUIXUGAMEDEV]")
	void SetParabolaScale(const float StartScale, const float EndScale) { ParabolaStartScale = StartScale; ParabolaEndScale = EndScale;};

	UFUNCTION(BlueprintCallable, Category = "GuideLine [KEVINTSUIXUGAMEDEV]")
	void SetParabolaTranslucentPriority(const int32 Priority) { ParabolaSortPriority = Priority;};

	UFUNCTION(BlueprintCallable, Category = "GuideLine [KEVINTSUIXUGAMEDEV]")
	void SetParabolaNumPoints(const int32 Input) { ParabolaPointsNum = Input; };
};


UCLASS(BlueprintType , Blueprintable)
class XKGAMEDEVCORE_API AXkGamepadCursor : public ADecalActor
{
	GENERATED_BODY()

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float Radius;

public:
	/** Default UObject constructor. */
	AXkGamepadCursor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	FORCEINLINE virtual void AddMovement(const FVector2D& InputValue, const FRotator& ForwardRotator, const float Speed);
	FORCEINLINE virtual void AddMovement(const FVector& InputValue, const FRotator& ForwardRotator, const float Speed);

	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (bTraceComplex = true))
	bool GetHitResultUnderGamepadCursor(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;
	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXUGAMEDEV]")
	void SetVisibility(const bool Input);

private:
	bool bMovementFreeze = false;
};


UCLASS(BlueprintType , Blueprintable)
class XKGAMEDEVCORE_API AXkController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Default UObject constructor. */
	AXkController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	TSubclassOf<AXkGamepadCursor> GamepadCursorClass;

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float ShortPressThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float MaxHoveringThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float CameraScrollingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float CameraDraggingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float CameraRotatingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float CameraZoomingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float MouseDraggingSensibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float MouseRotatingSensibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]")
	float GamepadCursorMovingSpeed;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultRealTimeMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultTurnBasedMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetCharacterMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetCharacterJumpAction;

	/** Camera Dragging Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraDraggingAction;

	/** Camera Dragging Press Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraDraggingPressAction;

	/** Camera Rotating Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraRotatingAction;

	/** Camera Rotating Press Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraRotatingPressAction;

	/** Camera Scrolling Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetCameraZoomingAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetSelectionClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetSelectionTouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetDeselectionClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetDeselectionTouchAction;

	/** Camera Scrolling Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetGamepadCursorMovementAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UInputAction* SetGamepadNavigationAction;

	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual EXkControlsMode GetControlsMode() const { return ControlsMode; };

	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual void SetControlsMode(const EXkControlsMode NewControlsMode) { ControlsMode = NewControlsMode; };

	/** Event delegate for when the controls flavor for the UI has changed. */
	DECLARE_EVENT_OneParam(AXkController, FControlsFlavorChangedEvent, const EXkControlsFlavor);
	FControlsFlavorChangedEvent& OnControlsFlavorChanged() { return ControlsFlavorChangedEvent; };

	/** Returns the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual EXkControlsFlavor GetControlsFlavor() const { return ControlsFlavor; };

	/** Sets the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual void SetControlsFlavor(const EXkControlsFlavor NewControlsFlavor);

	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual FVector2D GetControlsCursorPositionOnScreen() const;

	/** Returns the mouse area to switch mouse cursor. */
	UFUNCTION(BlueprintPure, Category = "Input [KEVINTSUIXUGAMEDEV]")
	virtual EXkControlsCursorArea GetControlsCursorArea() const { return ControlsCursorArea; };

protected:
	virtual void SetupInputComponent() override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	/** Input handlers for SetDestination action. */
	virtual void OnInputStarted();
	virtual void OnInputPressing();
	virtual void OnSetCharacterMoveTriggered(const FInputActionValue& Value);
	virtual void OnSetCharacterJumpTriggered(const FInputActionValue& Value);
	virtual void OnSetCharacterJumpCompleted(const FInputActionValue& Value);
	virtual void OnSetCameraDraggingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraDraggingPressing(const FInputActionValue& Value);
	virtual void OnSetCameraDraggingReleased();
	virtual void OnSetCameraRotatingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraRotatingPressing(const FInputActionValue& Value);
	virtual void OnSetCameraRotatingReleased();
	virtual void OnSetCameraZoomingTriggered(const FInputActionValue& Value);
	virtual void OnSetCameraZoomingPressing(const FInputActionValue& Value) {};
	virtual void OnSetCameraZoomingReleased() {};
	virtual void OnSetSelectionStarted() { OnInputStarted(); };
	virtual void OnSetSelectionTriggered() {};
	virtual void OnSetSelectionPressing() { OnInputPressing(); };
	virtual void OnSetSelectionReleased() {};
	virtual void OnSetDeselectionStarted() { OnInputStarted(); };
	virtual void OnSetDeselectionTriggered() {};
	virtual void OnSetDeselectionPressing() { OnInputPressing(); };
	virtual void OnSetDeselectionReleased() {};
	virtual void OnSetGamepadCursorMovementTriggered(const FInputActionValue& Value);
	virtual void OnSetGamepadCursorMovementPressing(const FInputActionValue& Value);
	virtual void OnSetGamepadCursorMovementReleased();
	virtual void OnSetGamepadNavigationTriggered(const FInputActionValue& Value);
	virtual void OnSetGamepadNavigationPressing(const FInputActionValue& Value);
	virtual void OnSetGamepadNavigationReleased();
	virtual void OnTouchTriggered();
	virtual void OnTouchReleased();

	UPROPERTY(Transient)
	EXkControlsMode ControlsMode;

	UPROPERTY(Transient)
	EXkControlsFlavor ControlsFlavor;
	FControlsFlavorChangedEvent ControlsFlavorChangedEvent;

	UPROPERTY(Transient)
	EXkControlsCursorArea ControlsCursorArea;

	UPROPERTY(Transient)
	mutable FVector2D CachedMouseCursorLocation;

	UPROPERTY(Transient)
	mutable FVector CachedGamepadCursorLocation;

	UPROPERTY(Transient)
	float HoveringTime;

	UPROPERTY(Transient)
	TWeakObjectPtr<class AXkGamepadCursor> GamepadCursor;

	bool bIsCameraDraggingButtonPressing;
	bool bIsCameraRotatingButtonPressing;
	float FollowTime; // For how long it has been pressed

public:
	virtual bool IsShortPress();
	virtual bool IsHoveringMotionless() const { return HoveringTime > MaxHoveringThreshold; };
	virtual bool IsOnUI() const { return false; };
	virtual bool IsCameraDragging() const { return bIsCameraDraggingButtonPressing; };
	virtual bool IsCameraRotating() const { return bIsCameraRotatingButtonPressing; };
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
	virtual class AXkCharacter* GetControlledCharacter() const;
};