#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RSPlayerHUD.generated.h"

UCLASS()
class RS_API URSPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called from C++ whenever health changes
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateHealthUI(float HealthPercent, const FLinearColor& HealthColor);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateAmmoUI(int32 CurrentAmmo, int32 MaxAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdatePlayerNameUI(const FString& NewPlayerName);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void SetCrosshairVisible(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void SetHealingTextVisible(bool bVisible);
};