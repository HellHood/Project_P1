#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

UENUM(BlueprintType)
enum class EAttackInputType : uint8
{
	Light,
	Heavy
};

USTRUCT(BlueprintType)
struct FAttackTransition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	EAttackInputType InputType = EAttackInputType::Light;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FName NextAttackId = NAME_None;
};

USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FName AttackId = NAME_None;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	TArray<FAttackTransition> Transitions;
};

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool RequestAttack(EAttackInputType InputType);

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAttackOnCooldown() const { return bAttackOnCooldown; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData LightAttackData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData LightFollowupData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData HeavyAttackData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData CurrentAttackData;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|Input")
	bool bHasBufferedAttack = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|Input")
	EAttackInputType BufferedInputType = EAttackInputType::Light;

	UPROPERTY(EditDefaultsOnly, Category="Combat|Debug")
	bool bDrawAttackDebug = true;

private:
	bool bAttackOnCooldown = false;

	FTimerHandle AttackDurationHandle;
	FTimerHandle AttackHitHandle;
	FTimerHandle AttackCooldownHandle;

	void BeginAttack(const FAttackData& AttackData);
	void ExecuteCurrentAttackHit();
	void EndAttack();

	void ResetAttackCooldown();

	bool TraceCurrentAttack(FHitResult& OutHit) const;
	bool ResolveAttackData(EAttackInputType InputType, FAttackData& OutAttackData) const;
	bool ResolveTransitionAttack(EAttackInputType InputType, FAttackData& OutAttackData) const;
	bool ResolveAttackById(FName AttackId, FAttackData& OutAttackData) const;
};