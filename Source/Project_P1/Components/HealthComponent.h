#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// ============================
// EVENTS (Blueprint + C++)
// ============================

// Called whenever health changes (damage or heal).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnHealthChanged,
	UHealthComponent*, HealthComp,
	float, NewHealth,
	float, Delta,
	AActor*, InstigatorActor
);

// Called once when health reaches 0.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDeath,
	UHealthComponent*, HealthComp,
	AActor*, InstigatorActor
);

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// ============================
	// Public API
	// ============================

	// Current health in [0..MaxHealth]
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return Health <= 0.f; }

	// Apply damage in a controlled way (useful for gameplay code).
	// NOTE: Unreal also has a global damage pipeline; we hook into it in BeginPlay.
	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(float DamageAmount, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(float HealAmount, AActor* InstigatorActor = nullptr);

	// Reset back to max (useful for respawn / debugging).
	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetToMax();

	// ============================
	// Events
	// ============================

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	// Max HP (tuning)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	// Current HP (runtime)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float Health = 100.f;

	// Prevent double death event.
	UPROPERTY(VisibleInstanceOnly, Category="Health")
	bool bHasDied = false;

	// ============================
	// Unreal Damage Pipeline Hook
	// ============================

	// This is called when someone uses UGameplayStatics::ApplyDamage / ApplyPointDamage etc.
	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const class UDamageType* DamageType,
		class AController* InstigatedBy,
		AActor* DamageCauser
	);

private:
	// Internal helper: clamps and fires events.
	void SetHealth(float NewHealth, float Delta, AActor* InstigatorActor);
};