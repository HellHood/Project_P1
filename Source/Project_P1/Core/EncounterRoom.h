#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterRoom.generated.h"

class UBoxComponent;
class AEnemyCharacter;
class UPrimitiveComponent;
class USceneComponent;
class UHealthComponent;

USTRUCT(BlueprintType)
struct FEncounterSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter")
	TSubclassOf<AEnemyCharacter> EnemyClass = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter")
	USceneComponent* SpawnPoint = nullptr;
};

UCLASS()
class PROJECT_P1_API AEncounterRoom : public AActor
{
	GENERATED_BODY()

public:
	AEncounterRoom();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Encounter")
	UBoxComponent* TriggerBox;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter")
	TArray<FEncounterSpawnEntry> SpawnEntries;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter")
	TArray<AActor*> EncounterBarriers;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Encounter")
	TArray<AEnemyCharacter*> SpawnedEnemies;

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
	
	UFUNCTION()
	void HandleSpawnedEnemyDeath(UHealthComponent* HealthComp, AActor* InstigatorActor);
	
	void StartEncounter();
	void SpawnEncounterEnemies();
	void EnableEncounterBarriers();
	void DisableEncounterBarriers();
	void CheckEncounterCleared();
};
