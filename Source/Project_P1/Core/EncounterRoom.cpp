#include "EncounterRoom.h"

#include "../Characters/EnemyCharacter.h"
#include "../Characters/PlayerCharacter.h"
#include "../Components/HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

AEncounterRoom::AEncounterRoom()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AEncounterRoom::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEncounterRoom::HandleTriggerBeginOverlap);
	}

	// Freeze enemies before encounter starts
	DeactivateEncounterEnemies();
}

void AEncounterRoom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEncounterStarted)
	{
		return;
	}

	if (bEncounterCleared)
	{
		return;
	}

	CheckEncounterState();
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
	if (bEncounterStarted)
	{
		return;
	}

	if (!Cast<APlayerCharacter>(OtherActor))
	{
		return;
	}

	StartEncounter();
}

void AEncounterRoom::StartEncounter()
{
	if (bEncounterStarted)
	{
		return;
	}

	bEncounterStarted = true;

	// Activate enemies when encounter starts
	ActivateEncounterEnemies();

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Started: %s"), *GetName());
	}
}

void AEncounterRoom::CheckEncounterState()
{
	if (!AreAllEnemiesDead())
	{
		return;
	}

	bEncounterCleared = true;

	if (bDebugEncounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] Cleared: %s"), *GetName());
	}
}

bool AEncounterRoom::AreAllEnemiesDead() const
{
	if (EncounterEnemies.IsEmpty())
	{
		return true;
	}

	for (AEnemyCharacter* Enemy : EncounterEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		const UHealthComponent* HealthComp = Enemy->GetHealthComponent();
		if (!HealthComp)
		{
			continue;
		}

		if (!HealthComp->IsDead())
		{
			return false;
		}
	}

	return true;
}

void AEncounterRoom::DeactivateEncounterEnemies()
{
	for (AEnemyCharacter* Enemy : EncounterEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		Enemy->DeactivateEnemy();
	}
}

void AEncounterRoom::ActivateEncounterEnemies()
{
	for (AEnemyCharacter* Enemy : EncounterEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		Enemy->ActivateEnemy();
	}
}