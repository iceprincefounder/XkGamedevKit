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
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void AddButtonWidgetIntoMap(class UUserWidget* InWidget, const int32 ButtonIndex);
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void ResetButtonWidgetIntoMap();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual TArray<int32> GetButtonWidgetsValidIndex() const;
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToNextButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnSwitchToLastButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual void OnCallCurrentButton();
	UFUNCTION(BlueprintCallable, Category = "Gameplay Button Press [KEVINTSUIXUGAMEDEV]")
	virtual int32 GetCurrentButtonIndex() const;

protected:
	/* This not true button index but a controller input index*/
	UPROPERTY(Transient)
	int32 CurrentButtonSwitchIndex;
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<class UUserWidget>> ButtonWidgetMap;
};
