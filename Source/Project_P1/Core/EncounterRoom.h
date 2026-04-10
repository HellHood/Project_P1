#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterRoom.generated.h"

class UBoxComponent;
class AEnemyCharacter;

UCLASS()
class PROJECT_P1_API AEncounterRoom : public AActor
{
	GENERATED_BODY()

public:
	AEncounterRoom();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
	UBoxComponent* TriggerBox;

	// Enemies assigned to this encounter in the level.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter")
	TArray<AEnemyCharacter*> EncounterEnemies;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Encounter")
	bool bEncounterStarted = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Encounter")
	bool bEncounterCleared = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Encounter|Debug")
	bool bDebugEncounter = true;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void StartEncounter();
	void CheckEncounterState();
	bool AreAllEnemiesDead() const;

	// Controls enemy activation state
	void DeactivateEncounterEnemies();
	void ActivateEncounterEnemies();
};
