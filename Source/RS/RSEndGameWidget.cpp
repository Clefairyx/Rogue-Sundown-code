#include "RSEndGameWidget.h"
#include "RSPlayerController.h"
#include "Components/Button.h"

void URSEndGameWidget::InitializeEndGame(const FString& WinnerName, const TArray<FEndGamePlayerResult>& Results)
{
	BP_SetWinnerText(WinnerName + TEXT(" won"));
	BP_SetResults(Results);
}

void URSEndGameWidget::FocusFirstButton()
{
	if (RestartButton)
	{
		RestartButton->SetUserFocus(GetOwningPlayer());
	}
}

void URSEndGameWidget::HandleRestartClicked()
{
	if (ARSPlayerController* PC = GetOwningPlayer<ARSPlayerController>())
	{
		PC->RestartGame();
	}
}

void URSEndGameWidget::HandleReturnToMenuClicked()
{
	if (ARSPlayerController* PC = GetOwningPlayer<ARSPlayerController>())
	{
		PC->ExitToMainMenu();
	}
}