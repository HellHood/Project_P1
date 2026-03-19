#include "BaseGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABaseGameMode::ABaseGameMode()
{
}

void ABaseGameMode::HandlePlayerDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player died. Restarting level in %.2f seconds."), RestartLevelDelay);

	GetWorldTimerManager().SetTimer(
		RestartLevelTimerHandle,
		this,
		&ABaseGameMode::RestartCurrentLevel,
		RestartLevelDelay,
		false
	);
}

void ABaseGameMode::RestartCurrentLevel()
{
	const FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}