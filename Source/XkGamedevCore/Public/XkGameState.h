// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "XkGameState.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FButtonPressedEvent, const int32, Input);

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
	UPROPERTY(BlueprintAssignable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	FButtonPressedEvent OnButtonPressedEvent;

	/** Add buttons.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void SetButtonWidgets(const TArray<class UUserWidget*>& InWidgets);

	/** Clear the map.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void ResetButtonWidgets();

	/** Get current button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual class UTileView* GetTileViewWidget() const { return TileViewWidget.Get(); };

	/** Set current focus tile view widget.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void SetTileViewWidget(class UTileView* InWidget);

	/** Reset current focus tile view widget.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void ResetTileViewWidget();

	/** Get buttons by sort button index.*/
	virtual TArray<int32> GetValidButtonWidgetsNum() const;

	/** Get current button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual int32 GetCurrentButtonIndex() const;

	/** Gamepad pressed to switch to next button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheNext();

	/** Gamepad pressed to switch to last button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheLast();

	/** Gamepad pressed to switch to up button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheTop();

	/** Gamepad pressed to switch to down button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheBottom();

	/** Gamepad pressed to switch to left button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheLeft();

	/** Gamepad pressed to switch to right button index.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnNavigationToTheRight();

	/** Call current index button OnButtonPressedEvent event binded function which might bind at Blueprint.*/
	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnButtonPressed();

	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	virtual void OnInputModeGameAndUI(class UUserWidget* InWidgetToFocus);

	UFUNCTION(BlueprintCallable, Category = "Navigation [KEVINTSUIXUGAMEDEV]")
	void OnInitFocusWidget(class UWidget* InWidgetToFocus, const bool bForceToFocus = false);

protected:
	/* This not true button index but a controller input index*/
	UPROPERTY(Transient)
	int32 ButtonIndex;

	/* Saved button maps.*/
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<class UUserWidget>> ButtonWidgets;

	/* Saved tile view widget.*/
	UPROPERTY(Transient)
	TWeakObjectPtr<class UTileView> TileViewWidget;
};
