// Copyright ©XUKAI. All Rights Reserved.

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
};


UCLASS(Blueprintable)
class XKGAMEDEVCORE_API AXkTopDownCamera : public AXkCamera
{
	GENERATED_BODY()

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category="Character [KEVINTSUIXU GAMEDEV]", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

#if WITH_EDITORONLY_DATA
	/** Component shown in the editor only to indicate character facing */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bCameraInvisibleWall;
	/** Camera boom positioning the camera above the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FBox2D CameraInvisibleWall;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	bool bCameraRotationLock;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FVector2D CameraRotationLock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	float CameraZoomArmLength;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera [KEVINTSUIXU GAMEDEV]", meta = (AllowPrivateAccess = "true"))
	FVector2D CameraZoomArmRange;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxVelocity;

	UPROPERTY(Category = "Movement [KEVINTSUIXU GAMEDEV]", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0"))
	float MaxAcceleration;
public:
	/** Default UObject constructor. */
	AXkTopDownCamera(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

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

	FORCEINLINE virtual void AddMovement(const FVector2D& InputValue, const float Speed);
	FORCEINLINE virtual void AddMovement(const FVector& InputValue, const float Speed);
	FORCEINLINE virtual void AddRotation(const FVector2D& InputValue, const float Speed);
	FORCEINLINE virtual void ResetRotation();
	FORCEINLINE virtual void AddCameraZoom(const float InputValue, const float Speed);
	FORCEINLINE virtual void ResetCameraZoom();
	FORCEINLINE virtual void AddMovementTarget(const FVector& InTarget);
	FORCEINLINE virtual FRotator GetForwardRotator() const;
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