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
	ButtonIndex = -1;
}


void AXkGameState::SetButtonWidgets(const TArray<class UUserWidget*>& InWidgets)
{
	ResetButtonWidgets();
	for (class UUserWidget* InWidget : InWidgets)
	{
		ButtonWidgets.Add(MakeWeakObjectPtr(InWidget));
	}
}


void AXkGameState::ResetButtonWidgets()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (AXkController* Controller = Cast<AXkController>(PlayerController))
	{
		if (Controller->GetControlsFlavor() == EXkControlsFlavor::Gamepad)
		{
			ButtonIndex = 0;
		}
		else
		{
			ButtonIndex = -1;
		}
	}	
	ButtonWidgets.Empty();
}


void AXkGameState::SetTileViewWidget(class UTileView* InWidget)
{
	TileViewWidget = MakeWeakObjectPtr(InWidget);
}


void AXkGameState::ResetTileViewWidget()
{
	TileViewWidget.Reset();
	ButtonIndex = -1;
}


TArray<int32> AXkGameState::GetValidButtonWidgetsNum() const
{
	TArray<int32> Results;
	for (TWeakObjectPtr<class UUserWidget> ButtonWidget : ButtonWidgets)
	{
		if (ButtonWidget.IsValid())
		{
			bool bIsEnabled = ButtonWidget->GetIsEnabled();
			bool bIsVisible = ButtonWidget->GetVisibility() == ESlateVisibility::Visible;
			if (bIsEnabled && bIsVisible)
			{
				int32 Index = ButtonWidgets.IndexOfByKey(ButtonWidget);
				Results.Add(Index);
			}
		}
	}
	Results.Sort([](const int32& A, const int32& B) { return (A < B); });
	return Results;
}


void AXkGameState::OnNavigationToTheNext()
{
	TArray<int32> ButtonWidgetsValidIndex = GetValidButtonWidgetsNum();
	if (!ButtonWidgetsValidIndex.IsEmpty())
	{
		ButtonIndex = (ButtonIndex + 1) % ButtonWidgetsValidIndex.Num();
		TWeakObjectPtr<class UUserWidget> CurrentWidget = ButtonWidgets[ButtonIndex];
		CurrentWidget->SetKeyboardFocus();
	}
}


void AXkGameState::OnNavigationToTheLast()
{
	TArray<int32> ButtonWidgetsValidIndex = GetValidButtonWidgetsNum();
	if (!ButtonWidgetsValidIndex.IsEmpty())
	{
		ButtonIndex = (ButtonIndex - 1 + ButtonWidgetsValidIndex.Num()) % ButtonWidgetsValidIndex.Num();
		TWeakObjectPtr<class UUserWidget> CurrentWidget = ButtonWidgets[ButtonIndex];
		CurrentWidget->SetKeyboardFocus();
	}
}


void AXkGameState::OnNavigationToTheTop()
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
			if (CurrentIndex >= Num - RowNum)
			{
				TileViewWidget->ScrollToBottom();
			}
			else if (CurrentIndex < RowNum)
			{
				TileViewWidget->ScrollToTop();
			}
			else
			{
				TileViewWidget->ScrollIndexIntoView(CurrentIndex);
			}
			TileViewWidget->RequestRefresh();
		}
	}
	else
	{
		OnNavigationToTheLast();
	}
}


void AXkGameState::OnNavigationToTheBottom()
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
			if (CurrentIndex >= Num - RowNum)
			{
				TileViewWidget->ScrollToBottom();
			}
			else if (CurrentIndex < RowNum)
			{
				TileViewWidget->ScrollToTop();
			}
			else
			{
				TileViewWidget->ScrollIndexIntoView(CurrentIndex);
			}
			TileViewWidget->RequestRefresh();
		}
	}
	else
	{
		OnNavigationToTheNext();
	}
}


void AXkGameState::OnNavigationToTheLeft()
{
	if (TileViewWidget.IsValid())
	{
		int32 WidthSize = TileViewWidget->GetCachedGeometry().Size.X;
		int32 RowNum = WidthSize / TileViewWidget->GetEntryWidth();
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			CurrentIndex = (CurrentIndex - 1 + Num) % Num;
			TileViewWidget->SetSelectedIndex(CurrentIndex);
			if (CurrentIndex >= Num - RowNum)
			{
				TileViewWidget->ScrollToBottom();
			}
			else if (CurrentIndex < RowNum)
			{
				TileViewWidget->ScrollToTop();
			}
			else
			{
				TileViewWidget->ScrollIndexIntoView(CurrentIndex);
			}
			TileViewWidget->RequestRefresh();
		}
	}
	else
	{
		OnNavigationToTheLast();
	}
}


void AXkGameState::OnNavigationToTheRight()
{
	if (TileViewWidget.IsValid())
	{
		int32 WidthSize = TileViewWidget->GetCachedGeometry().Size.X;
		int32 RowNum = WidthSize / TileViewWidget->GetEntryWidth();
		int32 CurrentIndex = TileViewWidget->GetIndexForItem(TileViewWidget->GetSelectedItem());
		int32 Num = TileViewWidget->GetNumItems();
		if (Num > 0)
		{
			CurrentIndex = (CurrentIndex + 1 + Num) % Num;
			TileViewWidget->SetSelectedIndex(CurrentIndex);
			if (CurrentIndex >= Num - RowNum)
			{
				TileViewWidget->ScrollToBottom();
			}
			else if (CurrentIndex < RowNum)
			{
				TileViewWidget->ScrollToTop();
			}
			else
			{
				TileViewWidget->ScrollIndexIntoView(CurrentIndex);
			}
			TileViewWidget->RequestRefresh();
		}
	}
	else
	{
		OnNavigationToTheNext();
	}
}


void AXkGameState::OnButtonPressed()
{
	OnButtonPressedEvent.Broadcast(GetCurrentButtonIndex());
}


void AXkGameState::OnInputModeGameAndUI(UUserWidget* InWidgetToFocus)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetWidgetToFocus(InWidgetToFocus->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}


void AXkGameState::OnInitFocusWidget(class UWidget* InWidgetToFocus, const bool bForceToFocus)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (AXkController* Controller = Cast<AXkController>(PlayerController))
	{
		if (bForceToFocus || Controller->GetControlsFlavor() == EXkControlsFlavor::Gamepad)
		{
			InWidgetToFocus->SetKeyboardFocus();
		}
	}
}


int32 AXkGameState::GetCurrentButtonIndex() const
{
	TArray<int32> ButtonWidgetsValidIndex = GetValidButtonWidgetsNum();
	if (ButtonIndex >= 0 && ButtonIndex < ButtonWidgetsValidIndex.Num())
	{
		return ButtonWidgetsValidIndex[ButtonIndex];
	}
	return ButtonIndex;
}
