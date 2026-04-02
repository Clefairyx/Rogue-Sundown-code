#include "RSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "RSCharacter.h"
#include "RSPlayerState.h"
#include "RSGameInstance.h"
#include "RSPlayerController.h"
#include "RSAIController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"

ARSGameMode::ARSGameMode()
{
	DefaultPawnClass = ARSCharacter::StaticClass();
	PlayerControllerClass = ARSPlayerController::StaticClass();
	PlayerStateClass = ARSPlayerState::StaticClass();
}

void ARSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	if (ULocalPlayer* LP = NewPlayer->GetLocalPlayer())
	{
		const int32 ControllerId = LP->GetControllerId();

		if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
		{
			PS->SetPlayerName(FString::Printf(TEXT("Player %d"), ControllerId + 1));
		}
	}
}

void ARSGameMode::StartPlay()
{
	Super::StartPlay();

	if (BackgroundMusic)
	{
		UGameplayStatics::PlaySound2D(this, BackgroundMusic);
	}

	URSGameInstance* GI = GetGameInstance<URSGameInstance>();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("RSGameMode: Failed to get RSGameInstance."));
		return;
	}

	GI->SetCurrentLoadingProgress(0.7f);

	const int32 DesiredPlayers = FMath::Clamp(GI->GetDesiredLocalPlayerCount(), 1, 4);
	const int32 NumBots = FMath::Clamp(GI->GetNumBots(), 0, 3);

	UE_LOG(LogTemp, Warning, TEXT("RSGameMode: StartPlay running with %d local player(s) already set up and %d bot(s)."),
		DesiredPlayers,
		NumBots);

	SpawnBots(NumBots);

	FinishGameStartup();
}

void ARSGameMode::FinishGameStartup()
{
	UE_LOG(LogTemp, Log, TEXT("RSGameMode: FinishGameStartup called."));

	if (URSGameInstance* GI = GetGameInstance<URSGameInstance>())
	{
		GI->SetCurrentLoadingProgress(1.0f);
		GI->StopLoadingScreen();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RSGameMode: Could not stop loading screen because RSGameInstance was null."));
	}
}

void ARSGameMode::EnsurePlayerStartsInitialized()
{
	if (ShuffledPlayerStarts.Num() > 0)
	{
		return;
	}

	NextPlayerStartIndex = 0;

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), FoundStarts);

	for (AActor* Actor : FoundStarts)
	{
		if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
		{
			ShuffledPlayerStarts.Add(PlayerStart);
			UE_LOG(LogTemp, Warning, TEXT("RSGameMode: Found PlayerStart %s"), *PlayerStart->GetName());
		}
	}

	for (int32 i = 0; i < ShuffledPlayerStarts.Num(); ++i)
	{
		const int32 SwapIndex = FMath::RandRange(i, ShuffledPlayerStarts.Num() - 1);
		ShuffledPlayerStarts.Swap(i, SwapIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("RSGameMode: Found and shuffled %d PlayerStarts."), ShuffledPlayerStarts.Num());
}

AActor* ARSGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPlayerStart: Player was null. Falling back to Super."));
		return Super::FindPlayerStart_Implementation(Player, IncomingName);
	}

	if (TWeakObjectPtr<APlayerStart>* ExistingStartPtr = AssignedPlayerStarts.Find(Player))
	{
		if (ExistingStartPtr->IsValid())
		{
			const FVector Loc = ExistingStartPtr->Get()->GetActorLocation();

			UE_LOG(LogTemp, Warning, TEXT("FindPlayerStart: Reusing assigned start %s for %s at location X=%f Y=%f Z=%f"),
				*GetNameSafe(ExistingStartPtr->Get()),
				*GetNameSafe(Player),
				Loc.X,
				Loc.Y,
				Loc.Z);

			return ExistingStartPtr->Get();
		}
	}

	AActor* Chosen = ChoosePlayerStart(Player);

	UE_LOG(LogTemp, Warning, TEXT("FindPlayerStart: ChoosePlayerStart returned %s for %s"),
		*GetNameSafe(Chosen),
		*GetNameSafe(Player));

	if (Chosen)
	{
		return Chosen;
	}

	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

AActor* ARSGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	EnsurePlayerStartsInitialized();

	const FString ControllerName = GetNameSafe(Player);

	UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart called for %s | NextPlayerStartIndex: %d | NumStarts: %d"),
		*ControllerName,
		NextPlayerStartIndex,
		ShuffledPlayerStarts.Num());

	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Player was null. Falling back to Super."));
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	if (TWeakObjectPtr<APlayerStart>* ExistingStartPtr = AssignedPlayerStarts.Find(Player))
	{
		if (ExistingStartPtr->IsValid())
		{
			const FVector Loc = ExistingStartPtr->Get()->GetActorLocation();

			UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Reusing already assigned PlayerStart %s for %s at location X=%f Y=%f Z=%f"),
				*GetNameSafe(ExistingStartPtr->Get()),
				*ControllerName,
				Loc.X,
				Loc.Y,
				Loc.Z);

			return ExistingStartPtr->Get();
		}
	}

	while (ShuffledPlayerStarts.IsValidIndex(NextPlayerStartIndex))
	{
		APlayerStart* ChosenStart = ShuffledPlayerStarts[NextPlayerStartIndex];
		++NextPlayerStartIndex;

		if (!ChosenStart)
		{
			continue;
		}

		bool bAlreadyUsed = false;
		for (const TPair<TWeakObjectPtr<AController>, TWeakObjectPtr<APlayerStart>>& Pair : AssignedPlayerStarts)
		{
			if (Pair.Value.Get() == ChosenStart)
			{
				bAlreadyUsed = true;
				break;
			}
		}

		if (bAlreadyUsed)
		{
			UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: PlayerStart %s already used, skipping."),
				*ChosenStart->GetName());
			continue;
		}

		AssignedPlayerStarts.Add(Player, ChosenStart);

		const FVector Loc = ChosenStart->GetActorLocation();

		UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Assigned %s to %s at location X=%f Y=%f Z=%f"),
			*ChosenStart->GetName(),
			*ControllerName,
			Loc.X,
			Loc.Y,
			Loc.Z);

		return ChosenStart;
	}

	UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Ran out of unique shuffled PlayerStarts for %s. Falling back to Super."),
		*ControllerName);

	AActor* Fallback = Super::ChoosePlayerStart_Implementation(Player);

	UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Fallback PlayerStart for %s is %s"),
		*ControllerName,
		*GetNameSafe(Fallback));

	return Fallback;
}

APlayerStart* ARSGameMode::GetNextAvailablePlayerStart()
{
	EnsurePlayerStartsInitialized();

	while (ShuffledPlayerStarts.IsValidIndex(NextPlayerStartIndex))
	{
		APlayerStart* ChosenStart = ShuffledPlayerStarts[NextPlayerStartIndex];
		++NextPlayerStartIndex;

		if (!ChosenStart)
		{
			continue;
		}

		bool bAlreadyUsed = false;
		for (const TPair<TWeakObjectPtr<AController>, TWeakObjectPtr<APlayerStart>>& Pair : AssignedPlayerStarts)
		{
			if (Pair.Value.Get() == ChosenStart)
			{
				bAlreadyUsed = true;
				break;
			}
		}

		if (!bAlreadyUsed)
		{
			return ChosenStart;
		}
	}

	return nullptr;
}

void ARSGameMode::SpawnBots(int32 NumBots)
{
	if (NumBots <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < NumBots; ++i)
	{
		APlayerStart* Start = GetNextAvailablePlayerStart();
		if (!Start)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnBots: No available PlayerStart for bot %d"), i + 1);
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ARSCharacter* BotCharacter = World->SpawnActor<ARSCharacter>(
			DefaultPawnClass,
			Start->GetActorLocation(),
			Start->GetActorRotation(),
			SpawnParams
		);

		if (!BotCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnBots: Failed to spawn bot character %d"), i + 1);
			continue;
		}

		ARSAIController* BotController = World->SpawnActor<ARSAIController>(
			ARSAIController::StaticClass(),
			Start->GetActorLocation(),
			Start->GetActorRotation(),
			SpawnParams
		);

		if (!BotController)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnBots: Failed to spawn AIController for bot %d"), i + 1);
			BotCharacter->Destroy();
			continue;
		}

		BotController->Possess(BotCharacter);

		if (APlayerState* PS = BotController->PlayerState)
		{
			PS->SetPlayerName(FString::Printf(TEXT("Bot %d"), i + 1));
		}

		AssignedPlayerStarts.Add(BotController, Start);

		UE_LOG(LogTemp, Log, TEXT("SpawnBots: Spawned Bot %d at %s"), i + 1, *Start->GetName());
	}
}

void ARSGameMode::HandlePlayerDeath(ARSCharacter* DeadCharacter)
{
	if (bMatchEnded)
	{
		return;
	}

	CheckForMatchEnd();
}

void ARSGameMode::CheckForMatchEnd()
{
	if (bMatchEnded)
	{
		return;
	}

	int32 AlivePlayers = 0;

	for (TActorIterator<ARSCharacter> It(GetWorld()); It; ++It)
	{
		ARSCharacter* Character = *It;
		if (IsValid(Character) && !Character->IsDead())
		{
			++AlivePlayers;
		}
	}

	if (AlivePlayers <= 1)
	{
		EndMatchAndShowResults();
	}
}

FString ARSGameMode::GetWinnerName() const
{
	for (TActorIterator<ARSCharacter> It(GetWorld()); It; ++It)
	{
		const ARSCharacter* Character = *It;
		if (IsValid(Character) && !Character->IsDead())
		{
			if (const APlayerState* PS = Character->GetPlayerState())
			{
				return PS->GetPlayerName();
			}
		}
	}

	return TEXT("Nobody");
}

void ARSGameMode::BuildEndGameResults(TArray<FEndGamePlayerResult>& OutResults) const
{
	OutResults.Empty();

	if (!GameState)
	{
		return;
	}

	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (!PS)
		{
			continue;
		}

		FEndGamePlayerResult Result;
		Result.PlayerName = PS->GetPlayerName();

		if (const ARSPlayerState* RSPS = Cast<ARSPlayerState>(PS))
		{
			Result.Kills = RSPS->GetKills();
		}

		OutResults.Add(Result);
	}
}

void ARSGameMode::EndMatchAndShowResults()
{
	if (bMatchEnded)
	{
		return;
	}

	bMatchEnded = true;

	const FString WinnerName = GetWinnerName();

	TArray<FEndGamePlayerResult> Results;
	BuildEndGameResults(Results);

	UGameplayStatics::SetGamePaused(this, true);

	if (ARSPlayerController* PC = Cast<ARSPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->ShowEndGameMenu(WinnerName, Results);
	}
}