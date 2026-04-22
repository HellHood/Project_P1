#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;
class UAttackSetDataAsset;

UENUM(BlueprintType)
enum class EAttackInputType : uint8
{
	Light,
	Heavy
};

UENUM(BlueprintType)
enum class EHitReactionType : uint8
{
	None,
	Knockback,
	Launch,
	CarryForward
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

	// Whether this attack can be selected directly from input as an entry attack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	bool bCanStartAttack = false;

	// Input used to start this attack directly.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	EAttackInputType StarterInputType = EAttackInputType::Light;

	// Damage applied on confirmed hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Damage = 25.f;

	// Total attack duration.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Duration = 0.3f;

	// Time from attack start to hit window activation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float HitStartTime = 0.1f;

	// Time from attack start to hit window end.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float HitEndTime = 0.2f;

	// Sweep distance.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Range = 150.f;

	// Sweep radius.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Radius = 50.f;

	// Cooldown applied when the attack starts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float Cooldown = 0.25f;

	// Montage played when this attack starts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Animation")
	UAnimMontage* AttackMontage = nullptr;

	// Movement reaction type applied to hit targets.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|HitReaction")
	EHitReactionType HitReactionType = EHitReactionType::Knockback;

	// Horizontal reaction strength used for knockback and launch.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|HitReaction")
	float KnockbackStrength = 400.f;

	// Vertical reaction strength used by launch attacks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|HitReaction")
	float LaunchStrength = 0.f;

	// Speed used while carrying the target forward.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|HitReaction")
	float CarrySpeed = 0.f;

	// Duration of the carry forward reaction.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|HitReaction")
	float CarryDuration = 0.f;

	// Allowed follow-up transitions from this attack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	TArray<FAttackTransition> Transitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style")
	float BaseStyleValue = 10.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStartedSignature, FAttackData, AttackData);

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

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnAttackStartedSignature OnAttackStarted;
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool RequestAttackById(FName AttackId);

	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetAttackSet(UAttackSetDataAsset* NewAttackSet);

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAttacking() const { return bIsAttacking; }
protected:
	virtual void BeginPlay() override;

	// Data-driven attack set for the current owner.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|AttackData")
	UAttackSetDataAsset* AttackSet = nullptr;

	// Runtime copy of the currently active attack.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|AttackData")
	FAttackData CurrentAttackData;

	// Blocks direct restart while attack is active.
	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bHitWindowActive = false;

	UPROPERTY(VisibleInstanceOnly, Category="Combat|State")
	bool bStyleRegisteredThisAttack = false;

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
	float AttackStartTime = 0.0f;

	// Buffered input timestamp relative to AttackStartTime.
	float BufferedInputTime = 0.0f;

	TSet<AActor*> HitActorsThisWindow;

	FTimerHandle AttackDurationHandle;
	FTimerHandle AttackCooldownHandle;
	FTimerHandle HitWindowStartHandle;
	FTimerHandle HitWindowEndHandle;
	FTimerHandle HitWindowTickHandle;

	void BeginAttack(const FAttackData& AttackData);
	void StartHitWindow();
	void ProcessHitWindowTick();
	void EndHitWindow();
	void EndAttack();

	void ResetAttackCooldown();

	bool TraceCurrentAttack(TArray<FHitResult>& OutHits) const;
	bool ResolveDefaultAttackData(EAttackInputType InputType, FAttackData& OutAttackData) const;
	bool ResolveTransitionAttack(EAttackInputType InputType, float InputTime, FAttackData& OutAttackData) const;
	bool ResolveAttackById(FName AttackId, FAttackData& OutAttackData) const;
};