#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "../Components/CombatComponent.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;
class UCombatComponent;

USTRUCT(BlueprintType)
struct FEnemyAttackOption
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	FName AttackId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float MinRange = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	float MaxRange = 250.f;
};

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
	
	UFUNCTION(BlueprintPure, Category="Enemy|Combat")
	bool IsTargetInAttackRange() const;
	
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsDead() const;
	
	UFUNCTION(BlueprintCallable, Category="Enemy|Combat")
	bool TryAttackFromAI();

	bool TryAttackFromAI(float& OutAttackDuration);
	
	UFUNCTION(BlueprintPure, Category="Enemy|Combat")
	bool CanAttack() const;
	
	UPROPERTY(EditDefaultsOnly, Category="Enemy|Combat")
	TArray<FEnemyAttackOption> AttackOptions;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(EditDefaultsOnly, Category="Enemy|Debug")
	bool bDebugEnemy = false;

	UFUNCTION()
	void HandleEnemyDeath(UHealthComponent* HealthComp, AActor* InstigatorActor);

	UFUNCTION()
	void HandleEnemyHealthChanged(UHealthComponent* HealthComp, float NewHealth, float Delta, AActor* InstigatorActor);

	UFUNCTION()
	void HandleAttackStarted(FAttackData AttackData);
	
	

private:
	UPROPERTY()
	APawn* TargetPawn = nullptr;

	bool bIsDead = false;

	// Store hit reaction until HealthComponent confirms damage.
	bool bHasPendingHitReaction = false;
	EHitReactionType PendingReactionType = EHitReactionType::None;
	FVector PendingHitReactionDirection = FVector::ZeroVector;
	float PendingKnockbackStrength = 0.f;
	float PendingLaunchStrength = 0.f;
	float PendingCarrySpeed = 0.f;
	float PendingCarryDuration = 0.f;

	// Active carry forward state.
	bool bIsCarryForwardActive = false;
	FVector ActiveCarryDirection = FVector::ZeroVector;
	float ActiveCarrySpeed = 0.f;
	float ActiveCarryTimeRemaining = 0.f;
	
	bool IsTargetValid() const;
	float DistanceToTarget2D() const;
	bool HasLineOfSightToTarget() const;
	FName ChooseAttackId() const;
	bool IsAttackOptionInRange(const FEnemyAttackOption& AttackOption, float TargetDistance) const;
	float ScoreAttackOption(const FEnemyAttackOption& AttackOption, float TargetDistance) const;
	void TryAttackTarget();
};