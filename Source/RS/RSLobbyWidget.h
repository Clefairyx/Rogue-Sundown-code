#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RSLobbyWidget.generated.h"

UCLASS()
class RS_API URSLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleAddPlayer();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleRemovePlayer();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleAddBot();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleRemoveBot();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleStartGame();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleBack();

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void RefreshLobbyDisplay();

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetLocalPlayerCount() const { return LocalPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetBotCount() const { return BotCount; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetMaxPlayers() const { return MaxPlayers; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 LocalPlayerCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 BotCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FName GameplayLevelName = NAME_None;
};