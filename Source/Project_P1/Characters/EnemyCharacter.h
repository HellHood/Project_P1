#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;

UCLASS()
class PROJECT_P1_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

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

private:
	UPROPERTY()
	APawn* TargetPawn = nullptr;

	bool bAttackOnCooldown = false;
	bool bIsDead = false;

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