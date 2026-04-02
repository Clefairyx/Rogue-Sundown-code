#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "RSMenuPlayerController.generated.h"

class UUserWidget;
class UInputMappingContext;
class UInputAction;
class URSLobbyWidget;

UENUM(BlueprintType)
enum class EMenuScreen : uint8
{
	None      UMETA(DisplayName = "None"),
	MainMenu  UMETA(DisplayName = "Main Menu"),
	Controls  UMETA(DisplayName = "Controls"),
	Lobby     UMETA(DisplayName = "Lobby")
};

UCLASS()
class RS_API ARSMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARSMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowControlsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowLobbyMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ExitGame();

	UFUNCTION(BlueprintPure, Category = "Menu")
	EMenuScreen GetCurrentMenuScreen() const { return CurrentMenuScreen; }

	UFUNCTION(BlueprintPure, Category = "Menu|Lobby")
	bool IsLobbyVisible() const;

	UFUNCTION(BlueprintPure, Category = "Menu|Lobby")
	URSLobbyWidget* GetLobbyWidget() const;

protected:
	void ShowWidget(TSubclassOf<UUserWidget> WidgetClass, EMenuScreen NewScreen);
	void ApplyMenuInputMode();

	void HandleOpenControls();
	void HandleBack();
	void HandleStart();

	void HandleLobbyAddPlayer();
	void HandleLobbyRemovePlayer();
	void HandleLobbyAddBot();
	void HandleLobbyRemoveBot();

	void SetMenuCamera();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Widgets")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Widgets")
	TSubclassOf<UUserWidget> ControlsWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Widgets")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets")
	TObjectPtr<UUserWidget> CurrentWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Menu|Widgets")
	TObjectPtr<URSLobbyWidget> LobbyWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EMenuScreen CurrentMenuScreen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputMappingContext> MenuMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> OpenControlsAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> BackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> StartAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> LobbyAddPlayerAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> LobbyRemovePlayerAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> LobbyAddBotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Input")
	TObjectPtr<UInputAction> LobbyRemoveBotAction;
};