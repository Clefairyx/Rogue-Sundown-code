#include "RSMenuGameMode.h"
#include "RSMenuPlayerController.h"
#include "RSGameInstance.h"

ARSMenuGameMode::ARSMenuGameMode()
{
	PlayerControllerClass = ARSMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ARSMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("RSMenuGameMode: BeginPlay called."));

	if (URSGameInstance* GI = GetGameInstance<URSGameInstance>())
	{
		GI->SetCurrentLoadingProgress(1.0f);
		GI->StopLoadingScreen();
		GI->ResetToSingleLocalPlayer();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RSMenuGameMode: Failed to get RSGameInstance in BeginPlay."));
	}
}

void ARSMenuGameMode::FinishMenuStartup()
{
	// No longer used, but kept in case you want to restore delayed startup later.
	UE_LOG(LogTemp, Log, TEXT("RSMenuGameMode: FinishMenuStartup called."));

	if (URSGameInstance* GI = GetGameInstance<URSGameInstance>())
	{
		GI->SetCurrentLoadingProgress(1.0f);
		GI->StopLoadingScreen();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RSMenuGameMode: Could not stop loading screen because RSGameInstance was null."));
	}
}