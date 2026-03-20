#include "HealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	bHasDied = false;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::ApplyDamage(float DamageAmount, AActor* InstigatorActor)
{
	if (DamageAmount <= 0.f)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	SetHealth(Health - DamageAmount, -DamageAmount, InstigatorActor);
}

void UHealthComponent::Heal(float HealAmount, AActor* InstigatorActor)
{
	if (HealAmount <= 0.f)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	SetHealth(Health + HealAmount, HealAmount, InstigatorActor);
}

void UHealthComponent::ResetToMax()
{
	bHasDied = false;
	SetHealth(MaxHealth, MaxHealth - Health, nullptr);
}

void UHealthComponent::HandleTakeAnyDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser
)
{
	if (Damage <= 0.f)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	SetHealth(Health - Damage, -Damage, DamageCauser);

	UE_LOG(LogTemp, Warning, TEXT("[Health] Took %.1f damage. New=%.1f"), Damage, Health);
}

void UHealthComponent::SetHealth(float NewHealth, float Delta, AActor* InstigatorActor)
{
	const float ClampedHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (FMath::IsNearlyEqual(ClampedHealth, Health))
	{
		return;
	}

	Health = ClampedHealth;

	OnHealthChanged.Broadcast(this, Health, Delta, InstigatorActor);

	if (!bHasDied && Health <= 0.f)
	{
		bHasDied = true;
		OnDeath.Broadcast(this, InstigatorActor);

		if (bDestroyOwnerOnDeath)
		{
			if (AActor* Owner = GetOwner())
			{
				Owner->SetActorEnableCollision(false);
				Owner->Destroy();
			}
		}
	}
}