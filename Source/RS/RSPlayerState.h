#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RSPlayerState.generated.h"

UCLASS()
class RS_API ARSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ARSPlayerState();

protected:

	/** Total kills for this player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	/** Total deaths for this player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

public:

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetDeaths() const { return Deaths; }

	void AddKill();
	void AddDeath();
};