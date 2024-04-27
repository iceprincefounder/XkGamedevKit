// Fill out your copyright notice in the Description page of Project Settings.

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
	UPROPERTY(BlueprintAssignable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	FGameButtonPressedEvent OnGameButtonPressedEvent;
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual void AddButtonWidgetIntoMap(class UUserWidget* InWidget, const int32 ButtonIndex);
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual void ResetButtonWidgetIntoMap();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual TArray<int32> GetButtonWidgetsValidIndex() const;
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual void OnSwitchToNextButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual void OnSwitchToLastButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual void OnCallCurrentButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXU GAMEDEV]")
	virtual int32 GetCurrentButtonIndex() const;

protected:
	/* This not true button index but a controller input index*/
	UPROPERTY(Transient)
	int32 CurrentButtonSwitchIndex;
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<class UUserWidget>> ButtonWidgetMap;
};
