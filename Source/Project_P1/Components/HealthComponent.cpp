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

	// Initialize current health to max at runtime.
	Health = MaxHealth;
	bHasDied = false;

	// Hook into Unreal's built-in damage pipeline.
	// If someone calls ApplyDamage(...) on the owning actor, we receive it here.
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::ApplyDamage(float DamageAmount, AActor* InstigatorActor)
{
	if (DamageAmount <= 0.f) return;
	if (IsDead()) return;

	// In health logic, "damage" reduces health (negative delta).
	const float NewHealth = Health - DamageAmount;
	SetHealth(NewHealth, -DamageAmount, InstigatorActor);
}

void UHealthComponent::Heal(float HealAmount, AActor* InstigatorActor)
{
	if (HealAmount <= 0.f) return;
	if (IsDead()) return; // v1 rule: no healing from death (change later if you want revives)

	const float NewHealth = Health + HealAmount;
	SetHealth(NewHealth, +HealAmount, InstigatorActor);
}

void UHealthComponent::ResetToMax()
{
	bHasDied = false;
	SetHealth(MaxHealth, MaxHealth - Health, /*InstigatorActor*/ nullptr);
}

void UHealthComponent::HandleTakeAnyDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser
)
{
	// Unreal conventions:
	// - Damage can be 0, ignore it.
	// - DamageCauser is usually the weapon / projectile / attacker actor.
	if (Damage <= 0.f) return;
	if (IsDead()) return;

	AActor* InstigatorActor = DamageCauser;
	SetHealth(Health - Damage, -Damage, InstigatorActor);
}

void UHealthComponent::SetHealth(float NewHealth, float Delta, AActor* InstigatorActor)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (FMath::IsNearlyEqual(Clamped, Health))
	{
		// No change => no events.
		return;
	}

	Health = Clamped;

	// Notify listeners (UI, animations, hit reactions, etc.)
	OnHealthChanged.Broadcast(this, Health, Delta, InstigatorActor);

	// Fire death once.
	if (!bHasDied && Health <= 0.f)
	{
		bHasDied = true;
		OnDeath.Broadcast(this, InstigatorActor);
	}
}