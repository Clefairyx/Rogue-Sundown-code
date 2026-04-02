// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RSGameMode.h"
#include "RSPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class URSPauseMenuWidget;
class URSPlayerHUD;
class URSEndGameWidget;
class APawn;

UCLASS(abstract)
class RS_API ARSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Called by the pause menu widget to resume gameplay */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void HidePauseMenu();

	/** Called by the pause menu widget to exit back to the main menu */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void ExitToMainMenu();

	/** Returns this controller's HUD widget */
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	URSPlayerHUD* GetPlayerHUDWidget() const { return HUDWidget; }

	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	void SetCrosshairVisible(bool bVisible);

	/** Shows the endgame menu */
	UFUNCTION(BlueprintCallable, Category = "UI|EndGame")
	void ShowEndGameMenu(const FString& WinnerName, const TArray<FEndGamePlayerResult>& Results);

	/** Called by the endgame widget to restart the current level */
	UFUNCTION(BlueprintCallable, Category = "UI|EndGame")
	void RestartGame();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Pause input action */
	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	TObjectPtr<UInputAction> PauseAction;

	/** HUD widget to spawn for this local player */
	UPROPERTY(EditAnywhere, Category = "UI|HUD")
	TSubclassOf<URSPlayerHUD> HUDWidgetClass;

	/** Pointer to this controller's HUD widget */
	UPROPERTY()
	TObjectPtr<URSPlayerHUD> HUDWidget;

	/** Pause menu widget to spawn */
	UPROPERTY(EditAnywhere, Category = "UI|Pause")
	TSubclassOf<URSPauseMenuWidget> PauseMenuClass;

	/** Pointer to this controller's pause menu widget */
	UPROPERTY()
	TObjectPtr<URSPauseMenuWidget> PauseMenuWidget;

	/** Endgame menu widget to spawn */
	UPROPERTY(EditAnywhere, Category = "UI|EndGame")
	TSubclassOf<URSEndGameWidget> EndGameMenuClass;

	/** Pointer to the endgame menu widget */
	UPROPERTY()
	TObjectPtr<URSEndGameWidget> EndGameMenuWidget;

	/** Menu level to open when exiting from pause menu */
	UPROPERTY(EditAnywhere, Category = "UI|Pause")
	FName MainMenuLevelName = TEXT("MainMenu");

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Called when this controller possesses a pawn */
	virtual void OnPossess(APawn* InPawn) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** Returns true only for the primary local player controller */
	bool IsPrimaryLocalPlayerController() const;

	/** Handles pause input */
	UFUNCTION()
	void HandlePause();

	/** Shows the pause menu and pauses the whole game */
	void ShowPauseMenu();

	/** Creates this controller's HUD widget */
	UFUNCTION()
	void CreatePlayerHUD();
};