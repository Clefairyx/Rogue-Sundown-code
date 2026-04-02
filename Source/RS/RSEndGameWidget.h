#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RSGameMode.h"
#include "RSEndGameWidget.generated.h"

class UButton;

UCLASS()
class RS_API URSEndGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the player controller when the match ends */
	UFUNCTION(BlueprintCallable, Category = "EndGame")
	void InitializeEndGame(const FString& WinnerName, const TArray<FEndGamePlayerResult>& Results);

	/** Lets the controller focus the first button for gamepad navigation */
	UFUNCTION(BlueprintCallable, Category = "EndGame")
	void FocusFirstButton();

	/** Called from Blueprint to update the big winner text */
	UFUNCTION(BlueprintImplementableEvent, Category = "EndGame")
	void BP_SetWinnerText(const FString& WinnerText);

	/** Called from Blueprint to populate the player results */
	UFUNCTION(BlueprintImplementableEvent, Category = "EndGame")
	void BP_SetResults(const TArray<FEndGamePlayerResult>& Results);

	/** Bind these in the widget blueprint buttons if you want */
	UFUNCTION(BlueprintCallable, Category = "EndGame")
	void HandleRestartClicked();

	UFUNCTION(BlueprintCallable, Category = "EndGame")
	void HandleReturnToMenuClicked();

protected:
	/** controller focus support */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;
};