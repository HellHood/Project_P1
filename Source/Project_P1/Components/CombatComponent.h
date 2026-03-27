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

	// Input required to follow this transition.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
	EAttackInputType InputType = EAttackInputType::Light;
	
	// Earliest valid input time relative to attack start.
	// If both WindowStart and WindowEnd are 0, timing is ignored.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float WindowStart = 0.0f;

	// Latest valid input time relative to attack start.
	// If both WindowStart and WindowEnd are 0, timing is ignored.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float WindowEnd = 0.0f;

	// Attack ID to start if this transition is accepted.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FName NextAttackId = NAME_None;
};

USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_BODY()

	// Unique identifier used by transition resolution.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FName AttackId = NAME_None;

	// Damage applied on confirmed hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Damage = 25.f;

	// Total attack duration.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Duration = 0.3f;

	// Time from attack start to hit execution.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float HitTime = 0.1f;

	// Sweep distance.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Range = 150.f;

	// Sweep radius.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Radius = 50.f;

	// Cooldown applied when the attack starts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Cooldown = 0.25f;

	// Allowed follow-up transitions from this attack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	TArray<FAttackTransition> Transitions;
};

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	// Requests an attack entry or follow-up based on input type.
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool RequestAttack(EAttackInputType InputType);

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAttackOnCooldown() const { return bAttackOnCooldown; }

protected:
	virtual void BeginPlay() override;

	// Default light attack entry.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData LightAttackData;

	// Example follow-up node for testing combo transitions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData LightFollowupData;

	// Example heavy entry or follow-up node.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData HeavyAttackData;

	// Runtime copy of the currently active attack.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData CurrentAttackData;

	// Blocks direct restart while attack is active.
	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bIsAttacking = false;

	// Buffered input is consumed when the current attack ends.
	UPROPERTY(VisibleInstanceOnly, Category="Combat|Input")
	bool bHasBufferedAttack = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|Input")
	EAttackInputType BufferedInputType = EAttackInputType::Light;

	UPROPERTY(EditDefaultsOnly, Category="Combat|Debug")
	bool bDrawAttackDebug = true;

private:
	bool bAttackOnCooldown = false;

	// Absolute world time when the current attack started.
	float AttackStartTime = 0.1f;

	// Buffered input timestamp relative to AttackStartTime.
	float BufferedInputTime = 0.8f;

	FTimerHandle AttackDurationHandle;
	FTimerHandle AttackHitHandle;
	FTimerHandle AttackCooldownHandle;

	void BeginAttack(const FAttackData& AttackData);
	void ExecuteCurrentAttackHit();
	void EndAttack();

	void ResetAttackCooldown();

	bool TraceCurrentAttack(FHitResult& OutHit) const;
	bool ResolveDefaultAttackData(EAttackInputType InputType, FAttackData& OutAttackData) const;
	bool ResolveTransitionAttack(EAttackInputType InputType, float InputTime, FAttackData& OutAttackData) const;
	bool ResolveAttackById(FName AttackId, FAttackData& OutAttackData) const;
};