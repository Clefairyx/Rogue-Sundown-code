#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Sound/SoundBase.h"
#include "RSGameMode.generated.h"

class APlayerStart;
class ARSCharacter;
class ARSAIController;
class URSGameInstance;

USTRUCT(BlueprintType)
struct FEndGamePlayerResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;
};

UCLASS()
class RS_API ARSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARSGameMode();

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;

	void HandlePlayerDeath(ARSCharacter* DeadCharacter);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* BackgroundMusic;

	UPROPERTY()
	TArray<APlayerStart*> ShuffledPlayerStarts;

	UPROPERTY()
	TMap<TWeakObjectPtr<AController>, TWeakObjectPtr<APlayerStart>> AssignedPlayerStarts;

	int32 NextPlayerStartIndex = 0;
	bool bMatchEnded = false;

	UFUNCTION()
	void FinishGameStartup();

	void EnsurePlayerStartsInitialized();
	void SpawnBots(int32 NumBots);
	APlayerStart* GetNextAvailablePlayerStart();

	void CheckForMatchEnd();
	void EndMatchAndShowResults();
	FString GetWinnerName() const;
	void BuildEndGameResults(TArray<FEndGamePlayerResult>& OutResults) const;
};