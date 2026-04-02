#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RSMenuGameMode.generated.h"

UCLASS()
class RS_API ARSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARSMenuGameMode();

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void FinishMenuStartup();
};