// Copyright ©XUKAI. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "XkGameState.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameButtonPressedEvent, const int32, Input);

/**
 * AXkGameState
 */
UCLASS()
class XKGAMEDEVCORE_API AXkGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	/** Default UObject constructor. */
	AXkGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** We add all buttons into a list for Gamepad controls, switch and press in GameState. */
	UPROPERTY(BlueprintAssignable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	FGameButtonPressedEvent OnGameButtonPressedEvent;

	/** Add button to the map and count the total number.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void AddButtonWidgetIntoMap(class UUserWidget* InWidget, const int32 ButtonIndex) const;

	/** Clear the map.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void ResetButtonWidgetIntoMap() const;
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")

	/** Get buttons by sort button index.*/
	virtual TArray<int32> GetButtonWidgetsValidIndex() const;

	/** Get current button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual int32 GetCurrentButtonIndex() const;

	/** Gamepad pressed to switch to next button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToNextButton() const;

	/** Gamepad pressed to switch to last button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToLastButton() const;

	/** Call current index button OnGameButtonPressedEvent event binded function which might bind at Blueprint.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnCallCurrentButton() const;
protected:
	/* This not true button index but a controller input index*/
	UPROPERTY(Transient)
	mutable int32 CurrentButtonSwitchIndex;

	/* Saved button maps.*/
	UPROPERTY(Transient)
	mutable TMap<int32, TWeakObjectPtr<class UUserWidget>> ButtonWidgetMap;
};
