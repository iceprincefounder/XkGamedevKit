// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "XkCamera.generated.h"

class UCapsuleComponent;
class UArrowComponent;

UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkCamera : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]")
	bool bEnableStylizePostProcess;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]")
	TArray<class UMaterialInterface*> StylizedPostProcessMaterials;

	UPROPERTY(Transient)
	TArray<class UMaterialInstanceDynamic*> StylizedPostProcessMaterialDyns;
public:
	/** Default UObject constructor. */
	AXkCamera(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin Actor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End Actor Interface

	virtual void SetEnableStylizedPostProcess(IConsoleVariable* Var);
};


UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkCharacterCamera : public AXkCamera
{
	GENERATED_BODY()

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category="Character [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Character camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class USceneCaptureComponent2D* SceneCaptureComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

#if WITH_EDITORONLY_DATA
	/** Component shown in the editor only to indicate character facing */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

public:
	/** Default UObject constructor. */
	AXkCharacterCamera(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin AXkCamera Interface
	virtual void SetEnableStylizedPostProcess(IConsoleVariable* Var) override;
	//~ End AXkCamera Interface

	//~ Begin AXkCharacterCamera Interface
	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class USceneCaptureComponent2D* GetSceneCaptureComponent() const { return SceneCaptureComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CapsuleComponent subobject **/
	FORCEINLINE class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
	FORCEINLINE virtual void AddRotation(const FVector2D& InputValue, const float Speed);
	//~ End AXkCharacterCamera Interface
};


UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkTopDownCamera : public AXkCamera
{
	GENERATED_BODY()

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category="Character [KEVINTSUIXUGAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

#if WITH_EDITORONLY_DATA
	/** Component shown in the editor only to indicate character facing */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UMaterialInterface* PostProcessMaterial;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* PostProcessMaterialDyn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bUseCameraRotationLock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FVector2D CameraRotationLock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float CameraZoomArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXUGAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FVector2D CameraZoomArmRange;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxVelocity;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxAcceleration;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bTravelingMode;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator TravelingView;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float TravelingZoom;

	UPROPERTY(Category = "Movement [KEVINTSUIXUGAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float TravelingSpeed;
public:
	/** Default UObject constructor. */
	AXkTopDownCamera(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin Actor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End Actor Interface

	//~ Begin AXkCamera Interface
	virtual void SetEnableStylizedPostProcess(IConsoleVariable* Var) override;
	//~ End AXkCamera Interface

	//~ Begin AXkTopDownCamera Interface
	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CapsuleComponent subobject **/
	FORCEINLINE class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
	virtual void ResetCamera();
	virtual void AddMovement(const FVector2D& InputValue, const float Speed);
	virtual void AddMovement(const FVector& InputValue, const float Speed);
	virtual void AddRotation(const FVector2D& InputValue, const float Speed);
	virtual void ResetRotation();
	virtual void AddCameraZoom(const float InputValue, const float Speed);
	virtual void ResetCameraZoom();
	virtual void AddMoveTarget(const FVector& InTarget);
	virtual void MoveToTarget(const FVector& InTarget, const bool bImmediately = false);
	virtual FRotator GetForwardRotator() const;
	virtual bool IsTravelingMode() const { return bTravelingMode; }
	virtual void SetTravelingMode(const bool bInTravelingMode);
	//~ End AXkTopDownCamera Interface
private:
	UPROPERTY()
	bool bMoveToTarget;

	UPROPERTY()
	FVector MovementTarget;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FVector Acceleration;
};


UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkThirdPersonCamera : public AXkCamera
{
	GENERATED_BODY()

public:
	// @TODO: implement third person character camera
};