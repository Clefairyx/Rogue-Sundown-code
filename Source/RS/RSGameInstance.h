#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RSGameInstance.generated.h"

class UTexture2D;

UCLASS()
class RS_API URSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// Travel (used by lobby instead of OpenLevel)
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void TravelToLevel(const FName LevelName);

	// Stop loading screen manually when gameplay is fully ready
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void StopLoadingScreen();

	// Create/remove local players BEFORE loading into gameplay
	UFUNCTION(BlueprintCallable, Category = "Players")
	void SetupLocalPlayersForGame();

	// Return to a single local player (useful when going back to main menu)
	UFUNCTION(BlueprintCallable, Category = "Players")
	void ResetToSingleLocalPlayer();

	// Loading progress
	float GetCurrentLoadingProgress() const { return CurrentLoadingProgress; }

	void SetCurrentLoadingProgress(float InProgress)
	{
		CurrentLoadingProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	}

	// Background image for loading screen
	UTexture2D* GetLoadingBackgroundTexture() const { return LoadingBackgroundTexture; }

	// ===== LOBBY DATA =====
	void SetDesiredLocalPlayerCount(int32 Num) { NumLocalPlayers = FMath::Clamp(Num, 1, 4); }
	void SetNumBots(int32 Num) { NumBots = FMath::Max(Num, 0); }

	int32 GetDesiredLocalPlayerCount() const { return NumLocalPlayers; }
	int32 GetNumBots() const { return NumBots; }

private:
	// Loading screen hooks
	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* LoadedWorld);

private:
	// ===== LOADING SYSTEM =====
	FName PendingLevelName = NAME_None;
	float CurrentLoadingProgress = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TObjectPtr<UTexture2D> LoadingBackgroundTexture = nullptr;

	// ===== LOBBY DATA =====
	int32 NumLocalPlayers = 1;
	int32 NumBots = 0;
};