#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Damage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Duration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float HitTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Range = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Radius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Cooldown = 0.25f;
};

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool TryLightAttack();

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsLightAttackOnCooldown() const { return bLightAttackOnCooldown; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData LightAttackData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData CurrentAttackData;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|Input")
	bool bBufferedLightAttack = false;

	UPROPERTY(EditDefaultsOnly, Category="Combat|Debug")
	bool bDrawAttackDebug = true;

private:
	bool bLightAttackOnCooldown = false;

	FTimerHandle AttackDurationHandle;
	FTimerHandle AttackHitHandle;
	FTimerHandle LightAttackCooldownHandle;

	void StartAttack(const FAttackData& AttackData);
	void ExecuteCurrentAttackHit();
	void EndAttack();

	void ResetLightAttackCooldown();

	bool TraceCurrentAttack(FHitResult& OutHit) const;
};