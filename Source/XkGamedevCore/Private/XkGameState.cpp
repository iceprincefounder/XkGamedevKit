// Copyright ©XUKAI. All Rights Reserved.


#include "XkGameState.h"
#include "XkController.h"
#include "Slate/SObjectTableRow.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"


AXkGameState::AXkGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentButtonSwitchIndex = -1;
}


void AXkGameState::AddButtonWidgetIntoMap(class UUserWidget* InWidget, const int32 ButtonIndex) const
{
	if (InWidget && IsValid(InWidget))
	{
		ButtonWidgetMap.Add(ButtonIndex, MakeWeakObjectPtr(InWidget));
	}
}


void AXkGameState::ResetButtonWidgetIntoMap() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (AXkController* Controller = Cast<AXkController>(PlayerController))
	{
		if (Controller->GetControlsFlavor() == EXkControlsFlavor::Gamepad)
		{
			CurrentButtonSwitchIndex = 0;
		}
		else
		{
			CurrentButtonSwitchIndex = -1;
		}
	}	
	ButtonWidgetMap.Reset();
}


TArray<int32> AXkGameState::GetButtonWidgetsValidIndex() const
{
	TArray<int32> Results;
	for (TPair<int32, TWeakObjectPtr<class UUserWidget>> ButtonWidgetPair : ButtonWidgetMap)
	{
		int32 ButtonIndex = ButtonWidgetPair.Key;
		TWeakObjectPtr<class UUserWidget> ButtonWidget = ButtonWidgetPair.Value;
		if (ButtonWidget.IsValid())
		{
			bool bIsEnabled = ButtonWidget->GetIsEnabled();
			bool bIsVisible = ButtonWidget->GetVisibility() == ESlateVisibility::Visible;
			if (bIsEnabled && bIsVisible)
			{
				Results.Add(ButtonIndex);
			}
		}
	}
	Results.Sort([](const int32& A, const int32& B) { return (A < B); });
	return Results;
}


void AXkGameState::OnSwitchToNextButton() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetButtonWidgetsValidIndex();
	if (!ButtonWidgetsValidIndex.IsEmpty())
	{
		CurrentButtonSwitchIndex = (CurrentButtonSwitchIndex + 1) % ButtonWidgetsValidIndex.Num();
	}
}


void AXkGameState::OnSwitchToLastButton() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetButtonWidgetsValidIndex();
	if (!ButtonWidgetsValidIndex.IsEmpty())
	{
		CurrentButtonSwitchIndex = (CurrentButtonSwitchIndex - 1 + ButtonWidgetsValidIndex.Num()) % ButtonWidgetsValidIndex.Num();
	}
}


void AXkGameState::OnCallCurrentButton() const
{
	OnGameButtonPressedEvent.Broadcast(GetCurrentButtonIndex());
}


int32 AXkGameState::GetCurrentButtonIndex() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetButtonWidgetsValidIndex();
	if (CurrentButtonSwitchIndex >= 0 && CurrentButtonSwitchIndex < ButtonWidgetsValidIndex.Num())
	{
		return ButtonWidgetsValidIndex[CurrentButtonSwitchIndex];
	}
	return CurrentButtonSwitchIndex;
}
