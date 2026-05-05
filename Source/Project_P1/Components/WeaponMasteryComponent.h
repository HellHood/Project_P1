#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponMasteryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnWeaponMasteryLevelChanged,
	FName, WeaponId,
	int32, NewLevel,
	float, CurrentXP
);

USTRUCT(BlueprintType)
struct FWeaponMasteryState
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon Mastery")
	FName WeaponId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon Mastery")
	float XP = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon Mastery")
	int32 Level = 1;
};

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UWeaponMasteryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponMasteryComponent();

	UFUNCTION(BlueprintCallable, Category="Weapon Mastery")
	void SetActiveWeapon(FName WeaponId);

	UFUNCTION(BlueprintCallable, Category="Weapon Mastery")
	void AddWeaponXP(FName WeaponId, float XPAmount);

	UFUNCTION(BlueprintPure, Category="Weapon Mastery")
	int32 GetWeaponLevel(FName WeaponId) const;

	UFUNCTION(BlueprintPure, Category="Weapon Mastery")
	float GetWeaponXP(FName WeaponId) const;

	UFUNCTION(BlueprintPure, Category="Weapon Mastery")
	bool IsAttackUnlocked(FName WeaponId, bool bRequiresWeaponMastery, int32 RequiredWeaponLevel) const;

	UFUNCTION(BlueprintPure, Category="Weapon Mastery")
	FName GetActiveWeaponId() const { return ActiveWeaponId; }

	UPROPERTY(BlueprintAssignable, Category="Weapon Mastery")
	FOnWeaponMasteryLevelChanged OnWeaponMasteryLevelChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Weapon Mastery")
	TArray<float> LevelThresholds = { 0.0f, 100.0f, 250.0f, 500.0f, 900.0f };

	UPROPERTY(EditDefaultsOnly, Category="Weapon Mastery")
	FName DefaultWeaponId = TEXT("Sword");

	UPROPERTY(VisibleInstanceOnly, Category="Weapon Mastery")
	FName ActiveWeaponId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, Category="Weapon Mastery")
	TArray<FWeaponMasteryState> WeaponStates;

	virtual void BeginPlay() override;

private:
	FWeaponMasteryState& FindOrCreateWeaponState(FName WeaponId);
	const FWeaponMasteryState* FindWeaponState(FName WeaponId) const;

	int32 CalculateLevelFromXP(float XP) const;
};