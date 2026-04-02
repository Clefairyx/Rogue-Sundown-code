#include "RSMenuPlayerController.h"

#include "RSLobbyWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

ARSMenuPlayerController::ARSMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	CurrentWidget = nullptr;
	LobbyWidget = nullptr;
	CurrentMenuScreen = EMenuScreen::None;
}

void ARSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MenuMappingContext)
			{
				InputSubsystem->AddMappingContext(MenuMappingContext, 0);
			}
		}
	}

	SetMenuCamera();
	ShowMainMenu();
}

void ARSMenuPlayerController::SetMenuCamera()
{
	AActor* FoundCamera = UGameplayStatics::GetActorOfClass(this, ACameraActor::StaticClass());

	if (FoundCamera)
	{
		SetViewTarget(FoundCamera);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Menu camera not found in level!"));
	}
}

void ARSMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (OpenControlsAction)
		{
			EnhancedInput->BindAction(OpenControlsAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleOpenControls);
		}

		if (BackAction)
		{
			EnhancedInput->BindAction(BackAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleBack);
		}

		if (StartAction)
		{
			EnhancedInput->BindAction(StartAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleStart);
		}

		if (LobbyAddPlayerAction)
		{
			EnhancedInput->BindAction(LobbyAddPlayerAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleLobbyAddPlayer);
		}

		if (LobbyRemovePlayerAction)
		{
			EnhancedInput->BindAction(LobbyRemovePlayerAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleLobbyRemovePlayer);
		}

		if (LobbyAddBotAction)
		{
			EnhancedInput->BindAction(LobbyAddBotAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleLobbyAddBot);
		}

		if (LobbyRemoveBotAction)
		{
			EnhancedInput->BindAction(LobbyRemoveBotAction, ETriggerEvent::Started, this, &ARSMenuPlayerController::HandleLobbyRemoveBot);
		}
	}
}

void ARSMenuPlayerController::ApplyMenuInputMode()
{
	FInputModeGameAndUI InputMode;

	if (CurrentWidget)
	{
		InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

bool ARSMenuPlayerController::IsLobbyVisible() const
{
	return CurrentMenuScreen == EMenuScreen::Lobby
		&& IsValid(LobbyWidget)
		&& LobbyWidget->IsInViewport()
		&& LobbyWidget->GetVisibility() == ESlateVisibility::Visible;
}

URSLobbyWidget* ARSMenuPlayerController::GetLobbyWidget() const
{
	if (!IsLobbyVisible())
	{
		return nullptr;
	}

	return LobbyWidget.Get();
}

void ARSMenuPlayerController::ShowWidget(TSubclassOf<UUserWidget> WidgetClass, EMenuScreen NewScreen)
{
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowWidget failed: WidgetClass is null."));
		return;
	}

	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	LobbyWidget = nullptr;

	// Create the lobby explicitly as URSLobbyWidget so the controller reference is guaranteed.
	if (NewScreen == EMenuScreen::Lobby)
	{
		URSLobbyWidget* CreatedLobby = CreateWidget<URSLobbyWidget>(this, WidgetClass);
		if (!CreatedLobby)
		{
			UE_LOG(LogTemp, Warning, TEXT("ShowWidget failed: CreateWidget<URSLobbyWidget> returned null."));
			return;
		}

		LobbyWidget = CreatedLobby;
		CurrentWidget = CreatedLobby;

		UE_LOG(LogTemp, Warning, TEXT("ShowWidget: Lobby widget created successfully."));
	}
	else
	{
		CurrentWidget = CreateWidget<UUserWidget>(this, WidgetClass);
		if (!CurrentWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("ShowWidget failed: CreateWidget<UUserWidget> returned null."));
			return;
		}
	}

	CurrentWidget->AddToViewport();
	CurrentWidget->SetVisibility(ESlateVisibility::Visible);
	CurrentMenuScreen = NewScreen;

	if (NewScreen == EMenuScreen::Lobby)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowWidget: CurrentMenuScreen set to Lobby. LobbyWidget valid = %s"),
			IsValid(LobbyWidget) ? TEXT("true") : TEXT("false"));
	}

	ApplyMenuInputMode();
}

void ARSMenuPlayerController::ShowMainMenu()
{
	ShowWidget(MainMenuWidgetClass, EMenuScreen::MainMenu);
}

void ARSMenuPlayerController::ShowControlsMenu()
{
	ShowWidget(ControlsWidgetClass, EMenuScreen::Controls);
}

void ARSMenuPlayerController::ShowLobbyMenu()
{
	ShowWidget(LobbyWidgetClass, EMenuScreen::Lobby);
}

void ARSMenuPlayerController::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ARSMenuPlayerController::HandleOpenControls()
{
	if (CurrentMenuScreen != EMenuScreen::MainMenu)
	{
		return;
	}

	ShowControlsMenu();
}

void ARSMenuPlayerController::HandleBack()
{
	if (CurrentMenuScreen == EMenuScreen::Lobby)
	{
		ShowMainMenu();
		return;
	}

	if (CurrentMenuScreen == EMenuScreen::Controls)
	{
		ShowMainMenu();
		return;
	}
}

void ARSMenuPlayerController::HandleStart()
{
	if (CurrentMenuScreen == EMenuScreen::Lobby)
	{
		if (URSLobbyWidget* Lobby = GetLobbyWidget())
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleStart: Lobby valid, starting game."));
			Lobby->HandleStartGame();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleStart: Current screen is Lobby, but LobbyWidget is invalid."));
		}
		return;
	}

	if (CurrentMenuScreen == EMenuScreen::MainMenu)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleStart: Showing lobby menu."));
		ShowLobbyMenu();
		return;
	}
}

void ARSMenuPlayerController::HandleLobbyAddPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddPlayer input fired"));

	if (URSLobbyWidget* Lobby = GetLobbyWidget())
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddPlayer: Lobby valid, calling HandleAddPlayer"));
		Lobby->HandleAddPlayer();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddPlayer: Lobby invalid"));
	}
}

void ARSMenuPlayerController::HandleLobbyRemovePlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemovePlayer input fired"));

	if (URSLobbyWidget* Lobby = GetLobbyWidget())
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemovePlayer: Lobby valid, calling HandleRemovePlayer"));
		Lobby->HandleRemovePlayer();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemovePlayer: Lobby invalid"));
	}
}

void ARSMenuPlayerController::HandleLobbyAddBot()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddBot input fired"));

	if (URSLobbyWidget* Lobby = GetLobbyWidget())
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddBot: Lobby valid, calling HandleAddBot"));
		Lobby->HandleAddBot();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyAddBot: Lobby invalid"));
	}
}

void ARSMenuPlayerController::HandleLobbyRemoveBot()
{
	UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemoveBot input fired"));

	if (URSLobbyWidget* Lobby = GetLobbyWidget())
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemoveBot: Lobby valid, calling HandleRemoveBot"));
		Lobby->HandleRemoveBot();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLobbyRemoveBot: Lobby invalid"));
	}
}