#include "RSLobbyWidget.h"
#include "Kismet/GameplayStatics.h"
#include "RSGameInstance.h"

void URSLobbyWidget::HandleAddPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleAddPlayer called"));

	if (LocalPlayerCount + BotCount >= MaxPlayers)
	{
		return;
	}

	LocalPlayerCount++;
	RefreshLobbyDisplay();
}

void URSLobbyWidget::HandleRemovePlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleRemovePlayer called"));

	if (LocalPlayerCount <= 1)
	{
		return;
	}

	LocalPlayerCount--;
	RefreshLobbyDisplay();
}

void URSLobbyWidget::HandleAddBot()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleAddBot called"));

	if (LocalPlayerCount + BotCount >= MaxPlayers)
	{
		return;
	}

	BotCount++;
	RefreshLobbyDisplay();
}

void URSLobbyWidget::HandleRemoveBot()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleRemoveBot called"));

	if (BotCount <= 0)
	{
		return;
	}

	BotCount--;
	RefreshLobbyDisplay();
}

void URSLobbyWidget::HandleStartGame()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleStartGame called"));

	if (GameplayLevelName == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameplayLevelName is not set!"));
		return;
	}

	if (URSGameInstance* GI = GetGameInstance<URSGameInstance>())
	{
		const int32 ClampedLocalPlayers = FMath::Clamp(LocalPlayerCount, 1, MaxPlayers);
		const int32 ClampedBots = FMath::Clamp(BotCount, 0, MaxPlayers - ClampedLocalPlayers);

		GI->SetDesiredLocalPlayerCount(ClampedLocalPlayers);
		GI->SetNumBots(ClampedBots);

		UE_LOG(LogTemp, Log, TEXT("Starting game with %d local players and %d bots"),
			GI->GetDesiredLocalPlayerCount(),
			GI->GetNumBots());

		// Create/remove local players before loading the gameplay map
		GI->SetupLocalPlayersForGame();

		// Load the gameplay level
		GI->TravelToLevel(GameplayLevelName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get RSGameInstance"));
	}
}

void URSLobbyWidget::HandleBack()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleBack called"));
}