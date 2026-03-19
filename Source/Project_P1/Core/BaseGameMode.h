#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaseGameMode.generated.h"

UCLASS()
class PROJECT_P1_API ABaseGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ABaseGameMode();

	// Called when the player dies.
	void HandlePlayerDeath();

protected:
	// Delay before restarting the level after death.
	UPROPERTY(EditDefaultsOnly, Category="Game Rules")
	float RestartLevelDelay = 2.0f;

private:
	FTimerHandle RestartLevelTimerHandle;

	void RestartCurrentLevel();
};