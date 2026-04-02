// Copyright Epic Games, Inc. All Rights Reserved.

#include "RSPlayerController.h"
#include "RSPauseMenuWidget.h"
#include "RSPlayerHUD.h"
#include "RSEndGameWidget.h"
#include "RSCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "RS.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	TWeakObjectPtr<ARSPlayerController> ActivePauseOwner;
	TWeakObjectPtr<URSPauseMenuWidget> ActivePauseWidget;
}

void ARSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// Create this local player's HUD
	if (IsLocalPlayerController())
	{
		CreatePlayerHUD();
	}

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogRS, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void ARSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalPlayerController())
	{
		CreatePlayerHUD();
	}

	if (ARSCharacter* RSCharacter = Cast<ARSCharacter>(InPawn))
	{
		RSCharacter->SyncHUDState();
	}
}

void ARSPlayerController::CreatePlayerHUD()
{
	if (HUDWidget)
	{
		return;
	}

	if (!HUDWidgetClass)
	{
		UE_LOG(LogRS, Warning, TEXT("%s: HUDWidgetClass is not assigned."), *GetName());
		return;
	}

	HUDWidget = CreateWidget<URSPlayerHUD>(this, HUDWidgetClass);

	if (HUDWidget)
	{
		HUDWidget->AddToPlayerScreen(0);
		UE_LOG(LogRS, Log, TEXT("%s: HUD widget created successfully."), *GetName());
	}
	else
	{
		UE_LOG(LogRS, Error, TEXT("%s: Failed to create HUD widget."), *GetName());
	}
}

void ARSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				if (CurrentContext)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			// IMCs if not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					if (CurrentContext)
					{
						Subsystem->AddMappingContext(CurrentContext, 0);
					}
				}
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ARSPlayerController::HandlePause);
		}
	}
}

bool ARSPlayerController::ShouldUseTouchControls() const
{
	return false;
}

bool ARSPlayerController::IsPrimaryLocalPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return IsLocalPlayerController() && LocalPlayer && LocalPlayer->GetControllerId() == 0;
}

void ARSPlayerController::HandlePause()
{
	// Do not allow pause toggling while the endgame menu is up.
	if (EndGameMenuWidget && EndGameMenuWidget->IsInViewport())
	{
		return;
	}

	if (UGameplayStatics::IsGamePaused(this))
	{
		if (ActivePauseOwner.IsValid())
		{
			ActivePauseOwner->HidePauseMenu();
		}
		else
		{
			HidePauseMenu();
		}
	}
	else
	{
		ShowPauseMenu();
	}
}

void ARSPlayerController::ShowPauseMenu()
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		return;
	}

	// Do not show pause if endgame is already visible.
	if (EndGameMenuWidget && EndGameMenuWidget->IsInViewport())
	{
		return;
	}

	UGameplayStatics::SetGamePaused(this, true);

	ActivePauseOwner = this;

	if (!PauseMenuWidget && PauseMenuClass)
	{
		PauseMenuWidget = CreateWidget<URSPauseMenuWidget>(this, PauseMenuClass);
	}

	if (PauseMenuWidget)
	{
		ActivePauseWidget = PauseMenuWidget;

		if (!PauseMenuWidget->IsInViewport())
		{
			PauseMenuWidget->AddToViewport(100);
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		SetInputMode(InputMode);
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;

		PauseMenuWidget->FocusFirstButton();
	}
	else
	{
		UE_LOG(LogRS, Warning, TEXT("PauseMenuClass is not assigned or pause menu widget could not be created."));
	}
}

void ARSPlayerController::HidePauseMenu()
{
	// Remove the globally active pause widget, regardless of which controller calls this.
	if (ActivePauseWidget.IsValid())
	{
		ActivePauseWidget->RemoveFromParent();
	}

	UGameplayStatics::SetGamePaused(this, false);

	// Restore input to the controller that opened the pause menu.
	if (ActivePauseOwner.IsValid())
	{
		FInputModeGameOnly InputMode;
		ActivePauseOwner->SetInputMode(InputMode);
		ActivePauseOwner->bShowMouseCursor = false;
		ActivePauseOwner->bEnableClickEvents = false;
		ActivePauseOwner->bEnableMouseOverEvents = false;
	}

	ActivePauseWidget = nullptr;
	ActivePauseOwner = nullptr;
}

void ARSPlayerController::ExitToMainMenu()
{
	if (ActivePauseWidget.IsValid())
	{
		ActivePauseWidget->RemoveFromParent();
	}

	if (EndGameMenuWidget)
	{
		EndGameMenuWidget->RemoveFromParent();
	}

	UGameplayStatics::SetGamePaused(this, false);

	ActivePauseWidget = nullptr;
	ActivePauseOwner = nullptr;
	EndGameMenuWidget = nullptr;

	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void ARSPlayerController::ShowEndGameMenu(const FString& WinnerName, const TArray<FEndGamePlayerResult>& Results)
{
	// Remove pause menu if it is open.
	if (ActivePauseWidget.IsValid())
	{
		ActivePauseWidget->RemoveFromParent();
		ActivePauseWidget = nullptr;
		ActivePauseOwner = nullptr;
	}

	if (!EndGameMenuWidget && EndGameMenuClass)
	{
		EndGameMenuWidget = CreateWidget<URSEndGameWidget>(this, EndGameMenuClass);
	}

	if (!EndGameMenuWidget)
	{
		UE_LOG(LogRS, Warning, TEXT("EndGameMenuClass is not assigned or endgame menu widget could not be created."));
		return;
	}

	if (!EndGameMenuWidget->IsInViewport())
	{
		EndGameMenuWidget->AddToViewport(200);
	}

	EndGameMenuWidget->InitializeEndGame(WinnerName, Results);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(EndGameMenuWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	EndGameMenuWidget->FocusFirstButton();
}

void ARSPlayerController::RestartGame()
{
	if (ActivePauseWidget.IsValid())
	{
		ActivePauseWidget->RemoveFromParent();
		ActivePauseWidget = nullptr;
		ActivePauseOwner = nullptr;
	}

	if (EndGameMenuWidget)
	{
		EndGameMenuWidget->RemoveFromParent();
		EndGameMenuWidget = nullptr;
	}

	UGameplayStatics::SetGamePaused(this, false);

	const FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void ARSPlayerController::SetCrosshairVisible(bool bVisible)
{
	if (HUDWidget)
	{
		HUDWidget->SetCrosshairVisible(bVisible);
	}
}