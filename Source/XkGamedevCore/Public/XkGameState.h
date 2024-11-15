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

	/** Get current button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual class UTileView* GetTileViewWidget() const { return TileViewWidget.Get(); };

	/** Set current focus tile view widget.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void SetTileViewWidget(class UTileView* InWidget) const;

	/** Reset current focus tile view widget.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void ResetTileViewWidget() const;

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

	/** Gamepad pressed to switch to up button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToTheUp() const;

	/** Gamepad pressed to switch to down button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToTheDown() const;

	/** Gamepad pressed to switch to left button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToTheLeft() const;

	/** Gamepad pressed to switch to right button index.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToTheRight() const;

	/** Call current index button OnGameButtonPressedEvent event binded function which might bind at Blueprint.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnCallCurrentButton() const;

protected:
	/* This not true button index but a controller input index*/
	UPROPERTY(Transient)
	mutable int32 CurrentSwitchIndex;

	/* Saved button maps.*/
	UPROPERTY(Transient)
	mutable TMap<int32, TWeakObjectPtr<class UUserWidget>> ButtonWidgetMap;

	/* Saved tile view widget.*/
	UPROPERTY(Transient)
	mutable TWeakObjectPtr<class UTileView> TileViewWidget;
};
