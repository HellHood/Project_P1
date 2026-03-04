#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

// v0 CombatComponent:
// - Owns attack parameters (damage/range/radius/cooldown)
// - Executes traces and applies damage via Unreal damage pipeline
// - Character/weapon just calls TryLightAttack()

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	// Attempt to start a light attack. Returns true if the attack was executed.
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool TryLightAttack();

	// Optional: expose cooldown state for UI/debug.
	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsLightAttackOnCooldown() const { return bLightAttackOnCooldown; }

protected:
	virtual void BeginPlay() override;

	// ===== Light Attack Tuning =====
	// Keep these in the component so later we can move them to a Weapon/AttackData asset easily.

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float LightAttackDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float LightAttackRange = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float LightAttackRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float LightAttackCooldown = 0.25f;

	// Debug draw for traces.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	bool bDrawAttackDebug = true;

private:
	// Cooldown bookkeeping.
	bool bLightAttackOnCooldown = false;
	FTimerHandle LightAttackCooldownHandle;

	void ResetLightAttackCooldown();

	// Core v0 melee hit test.
	bool DoLightAttackTrace(FHitResult& OutHit) const;
};