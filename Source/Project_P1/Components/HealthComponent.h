#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnHealthChanged,
	UHealthComponent*, HealthComp,
	float, NewHealth,
	float, Delta,
	AActor*, InstigatorActor
);

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

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return Health <= 0.f; }

	UFUNCTION(BlueprintCallable, Category="Health")
	void ApplyDamage(float DamageAmount, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(float HealAmount, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetToMax();

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Health|Death")
	bool bDestroyOwnerOnDeath = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float Health = 100.f;

	UPROPERTY(VisibleInstanceOnly, Category="Health")
	bool bHasDied = false;

	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const class UDamageType* DamageType,
		class AController* InstigatedBy,
		AActor* DamageCauser
	);

private:
	void SetHealth(float NewHealth, float Delta, AActor* InstigatorActor);
};