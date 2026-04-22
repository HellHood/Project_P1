#include "EncounterRoom.h"

#include "../Characters/EnemyCharacter.h"
#include "../Components/HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AEncounterRoom::AEncounterRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AEncounterRoom::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEncounterRoom::HandleTriggerBeginOverlap);
	}

	DisableEncounterBarriers();
}

void AEncounterRoom::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bEncounterStarted || bEncounterCleared)
	{
		return;
	}

	if (!OtherActor)
	{
		return;
	}

	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn)
	{
		return;
	}

	if (!OverlappingPawn->IsPlayerControlled())
	{
		return;
	}

	StartEncounter();
}

void AEncounterRoom::StartEncounter()
{
	if (bEncounterStarted || bEncounterCleared)
	{
		return;
	}

	bEncounterStarted = true;

	EnableEncounterBarriers();
	SpawnEncounterEnemies();
	CheckEncounterCleared();

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Started: %s"), *GetName());
	}
}

void AEncounterRoom::SpawnEncounterEnemies()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SpawnedEnemies.Empty();

	for (const FEncounterSpawnEntry& SpawnEntry : SpawnEntries)
	{
		if (!SpawnEntry.EnemyClass)
		{
			if (bDebugEncounter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Encounter] Spawn skipped: EnemyClass is null"));
			}

			continue;
		}

		if (!SpawnEntry.SpawnPoint)
		{
			if (bDebugEncounter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Encounter] Spawn skipped: SpawnPoint is null"));
			}

			continue;
		}

		const FTransform SpawnTransform = SpawnEntry.SpawnPoint->GetComponentTransform();

		AEnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<AEnemyCharacter>(
			SpawnEntry.EnemyClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (!SpawnedEnemy)
		{
			if (bDebugEncounter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Encounter] Spawn failed for class: %s"), *SpawnEntry.EnemyClass->GetName());
			}

			continue;
		}

		SpawnedEnemy->FinishSpawning(SpawnTransform);
		SpawnedEnemy->ActivateEnemy();

		if (UHealthComponent* HealthComp = SpawnedEnemy->GetHealthComponent())
		{
			HealthComp->OnDeath.AddDynamic(this, &AEncounterRoom::HandleSpawnedEnemyDeath);
		}

		SpawnedEnemies.Add(SpawnedEnemy);

		if (bDebugEncounter)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Encounter] Spawned enemy: %s"),
				*SpawnedEnemy->GetName()
			);
		}
	}
}

void AEncounterRoom::EnableEncounterBarriers()
{
	for (AActor* BarrierActor : EncounterBarriers)
	{
		if (!BarrierActor)
		{
			continue;
		}

		BarrierActor->SetActorHiddenInGame(false);
		BarrierActor->SetActorEnableCollision(true);
		BarrierActor->SetActorTickEnabled(true);
	}

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Barriers enabled"));
	}
}

void AEncounterRoom::DisableEncounterBarriers()
{
	for (AActor* BarrierActor : EncounterBarriers)
	{
		if (!BarrierActor)
		{
			continue;
		}

		BarrierActor->SetActorHiddenInGame(true);
		BarrierActor->SetActorEnableCollision(false);
		BarrierActor->SetActorTickEnabled(false);
	}

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Barriers disabled"));
	}
}

void AEncounterRoom::HandleSpawnedEnemyDeath(UHealthComponent* HealthComp, AActor* InstigatorActor)
{
	CheckEncounterCleared();

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Enemy death received"));
	}
}

void AEncounterRoom::CheckEncounterCleared()
{
	if (!bEncounterStarted || bEncounterCleared)
	{
		return;
	}

	int32 AliveEnemies = 0;

	for (AEnemyCharacter* SpawnedEnemy : SpawnedEnemies)
	{
		if (!IsValid(SpawnedEnemy))
		{
			continue;
		}

		if (!SpawnedEnemy->IsDead())
		{
			++AliveEnemies;
		}
	}

	if (AliveEnemies > 0)
	{
		if (bDebugEncounter)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Encounter] Alive enemies remaining: %d"), AliveEnemies);
		}

		return;
	}

	bEncounterCleared = true;
	DisableEncounterBarriers();

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Cleared: %s"), *GetName());
	}
}