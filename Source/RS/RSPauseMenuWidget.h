#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RSPauseMenuWidget.generated.h"

class UButton;

UCLASS()
class RS_API URSPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void FocusFirstButton();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;
};