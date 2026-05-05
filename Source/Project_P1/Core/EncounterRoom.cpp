#include "EncounterRoom.h"

#include "../Characters/EnemyCharacter.h"
#include "../Components/HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "../Characters/PlayerCharacter.h"
#include "../Components/StyleComponent.h"
#include "../Components/WeaponMasteryComponent.h"
#include "Kismet/GameplayStatics.h"

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

		const FTransform SpawnTransform = SpawnEntry.SpawnPoint->GetActorTransform();

		AEnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<AEnemyCharacter>(
			SpawnEntry.EnemyClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (!SpawnEntry.SpawnPoint)
		{
			if (bDebugEncounter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Encounter] Spawn skipped: SpawnPoint is null"));
			}
			continue;
		}

		SpawnedEnemy->FinishSpawning(SpawnTransform);

		SpawnedEnemy->SpawnDefaultController();

		// optional debug
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Controller after spawn: %s"),
			SpawnedEnemy->GetController() ? *SpawnedEnemy->GetController()->GetName() : TEXT("NONE"));

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

	AwardEncounterRewards();
	DisableEncounterBarriers();

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Cleared: %s"), *GetName());
	}
}

void AEncounterRoom::AwardEncounterRewards()
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0)
	);

	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Reward skipped: player not found"));
		return;
	}

	UStyleComponent* StyleComponent = PlayerCharacter->FindComponentByClass<UStyleComponent>();
	UWeaponMasteryComponent* MasteryComponent = PlayerCharacter->FindComponentByClass<UWeaponMasteryComponent>();

	if (!StyleComponent || !MasteryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Reward skipped: missing Style or Mastery component"));
		return;
	}

	const float FinalStyle = StyleComponent->GetStyle();
	const float TotalWeaponXP = FinalStyle * 0.1f;
	const TMap<FName, int32>& WeaponHitCounts = StyleComponent->GetWeaponHitCounts();

	int32 TotalWeaponHits = 0;
	for (const TPair<FName, int32>& WeaponHitPair : WeaponHitCounts)
	{
		TotalWeaponHits += WeaponHitPair.Value;
	}

	if (FinalStyle <= 0.0f || TotalWeaponXP <= 0.0f || TotalWeaponHits <= 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Encounter] Reward skipped: FinalStyle=%.1f TotalHits=%d"),
			FinalStyle,
			TotalWeaponHits
		);

		StyleComponent->ResetStyle();
		return;
	}

	for (const TPair<FName, int32>& WeaponHitPair : WeaponHitCounts)
	{
		const FName WeaponId = WeaponHitPair.Key;
		const int32 HitCount = WeaponHitPair.Value;

		if (WeaponId.IsNone() || HitCount <= 0)
		{
			continue;
		}

		const float WeaponRatio = static_cast<float>(HitCount) / static_cast<float>(TotalWeaponHits);
		const float WeaponXP = TotalWeaponXP * WeaponRatio;

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Encounter] Weapon reward: Weapon=%s Hits=%d Ratio=%.2f XP=%.1f"),
			*WeaponId.ToString(),
			HitCount,
			WeaponRatio,
			WeaponXP
		);

		MasteryComponent->AddWeaponXP(WeaponId, WeaponXP);
	}

	StyleComponent->ResetStyle();
}