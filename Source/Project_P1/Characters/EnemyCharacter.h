#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "../Components/CombatComponent.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;

UCLASS()
class PROJECT_P1_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	// Cache hit reaction data before damage is applied.
	void SetPendingHitReaction(
		EHitReactionType ReactionType,
		const FVector& Direction,
		float KnockbackStrength,
		float LaunchStrength,
		float CarrySpeed,
		float CarryDuration
	);
	
		// Enables full enemy behavior (movement, logic, attacking).
	void ActivateEnemy();

	// Disables enemy behavior before encounter start.
	void DeactivateEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|AI")
	float ChaseRange = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|AI")
	float RepathInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackRange = 140.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Debug")
	bool bDebugEnemy = false;

	UFUNCTION()
	void HandleEnemyDeath(UHealthComponent* HealthComp, AActor* InstigatorActor);

	UFUNCTION()
	void HandleEnemyHealthChanged(UHealthComponent* HealthComp, float NewHealth, float Delta, AActor* InstigatorActor);
	
	UFUNCTION()
	void HandleAttackStarted(FAttackData AttackData);
	
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsDead() const;

private:
	UPROPERTY()
	APawn* TargetPawn = nullptr;

	bool bAttackOnCooldown = false;
	bool bIsDead = false;

	// Store hit reaction until HealthComponent confirms damage.
	bool bHasPendingHitReaction = false;
	EHitReactionType PendingReactionType = EHitReactionType::None;
	FVector PendingHitReactionDirection = FVector::ZeroVector;
	float PendingKnockbackStrength = 0.f;
	float PendingLaunchStrength = 0.f;
	float PendingCarrySpeed = 0.f;
	float PendingCarryDuration = 0.f;

	// Active carry forward state
	bool bIsCarryForwardActive = false;
	FVector ActiveCarryDirection = FVector::ZeroVector;
	float ActiveCarrySpeed = 0.f;
	float ActiveCarryTimeRemaining = 0.f;

	FTimerHandle AttackCooldownHandle;
	FTimerHandle RepathHandle;

	bool IsTargetValid() const;
	float DistanceToTarget2D() const;
	bool HasLineOfSightToTarget() const;

	void ChaseTarget();
	void TryAttackTarget();
	void RepathTick();
	void ResetAttackCooldown();
};