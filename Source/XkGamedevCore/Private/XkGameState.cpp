// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkGameState.h"
#include "XkController.h"
#include "Slate/SObjectTableRow.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/TileView.h"


AXkGameState::AXkGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentSwitchIndex = -1;
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
			CurrentSwitchIndex = 0;
		}
		else
		{
			CurrentSwitchIndex = -1;
		}
	}	
	ButtonWidgetMap.Reset();
}


void AXkGameState::SetTileViewWidget(class UTileView* InWidget) const
{
	TileViewWidget = MakeWeakObjectPtr(InWidget);
}


void AXkGameState::ResetTileViewWidget() const
{
	TileViewWidget.Reset();
	CurrentSwitchIndex = -1;
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
		CurrentSwitchIndex = (CurrentSwitchIndex + 1) % ButtonWidgetsValidIndex.Num();
	}
}


void AXkGameState::OnSwitchToLastButton() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetButtonWidgetsValidIndex();
	if (!ButtonWidgetsValidIndex.IsEmpty())
	{
		CurrentSwitchIndex = (CurrentSwitchIndex - 1 + ButtonWidgetsValidIndex.Num()) % ButtonWidgetsValidIndex.Num();
	}
}


void AXkGameState::OnNavigationToTheTop() const
{
	if (TileViewWidget.IsValid())
	{
		int32 ItemWidth = TileViewWidget->GetEntryWidth();
		int32 WidthSize = TileViewWidget->GetCachedGeometry().Size.X;
		int32 RowNum = WidthSize / TileViewWidget->GetEntryWidth();
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			int32 TotalRawNum = FMath::CeilToInt32((float)Num / (float)RowNum) * RowNum;
			CurrentIndex = (CurrentIndex - RowNum + TotalRawNum) % TotalRawNum;
			if (CurrentIndex >= Num)
			{
				CurrentIndex -= RowNum;
			}
			TileViewWidget->SetSelectedIndex(CurrentIndex);
		}
	}
	else
	{
		OnSwitchToLastButton();
	}
}


void AXkGameState::OnNavigationToTheBottom() const
{
	if (TileViewWidget.IsValid())
	{
		int32 WidthSize = TileViewWidget->GetCachedGeometry().Size.X;
		int32 RowNum = WidthSize / TileViewWidget->GetEntryWidth();
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			int32 TotalRawNum = FMath::CeilToInt32((float)Num / (float)RowNum) * RowNum;
			CurrentIndex = (CurrentIndex + RowNum) % TotalRawNum;
			if (CurrentIndex >= Num)
			{
				CurrentIndex = (CurrentIndex + RowNum) % TotalRawNum;
			}
			TileViewWidget->SetSelectedIndex(CurrentIndex);
		}
	}
	else
	{
		OnSwitchToNextButton();
	}
}


void AXkGameState::OnNavigationToTheLeft() const
{
	if (TileViewWidget.IsValid())
	{
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			CurrentIndex = (CurrentIndex - 1 + Num) % Num;
			TileViewWidget->SetSelectedIndex(CurrentIndex);
		}
	}
	else
	{
		OnSwitchToNextButton();
	}
}


void AXkGameState::OnNavigationToTheRight() const
{
	if (TileViewWidget.IsValid())
	{
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			CurrentIndex = (CurrentIndex + 1 + Num) % Num;
			TileViewWidget->SetSelectedIndex(CurrentIndex);
		}
	}
	else
	{
		OnSwitchToNextButton();
	}
}


void AXkGameState::OnCallCurrentButton() const
{
	OnGameButtonPressedEvent.Broadcast(GetCurrentButtonIndex());
}


int32 AXkGameState::GetCurrentButtonIndex() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetButtonWidgetsValidIndex();
	if (CurrentSwitchIndex >= 0 && CurrentSwitchIndex < ButtonWidgetsValidIndex.Num())
	{
		return ButtonWidgetsValidIndex[CurrentSwitchIndex];
	}
	return CurrentSwitchIndex;
}
