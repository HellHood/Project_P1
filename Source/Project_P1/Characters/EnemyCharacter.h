#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "EnemyCharacter.generated.h"

// Enemy v0:
// - Chases the player using AI MoveTo
// - Applies damage in melee range on a cooldown
// No animations yet; pure gameplay loop.

UCLASS()
class PROJECT_P1_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ===== Tuning =====
	UPROPERTY(EditDefaultsOnly, Category="Enemy|AI")
	float ChaseRange = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackRange = 140.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Debug")
	bool bDebugEnemy = false;
	
	UPROPERTY(EditDefaultsOnly, Category="Enemy|AI")
	float RepathInterval = 0.2f;




private:
	// Cached target (player pawn)
	UPROPERTY()
	APawn* TargetPawn = nullptr;

	bool bAttackOnCooldown = false;
	FTimerHandle AttackCooldownHandle;
	
	FTimerHandle RepathHandle;

	void ResetAttackCooldown();

	bool IsTargetValid() const;
	float DistanceToTarget2D() const;
	void ChaseTarget();
	void TryAttackTarget();
	void RepathTick();
	bool HasLineOfSightToTarget() const;
};