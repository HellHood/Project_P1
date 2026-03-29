#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class AEnemyCharacter;

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	UFUNCTION(BlueprintCallable, Category="Targeting")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category="Targeting")
	void ClearTarget();

	UFUNCTION(BlueprintPure, Category="Targeting")
	bool IsLockedOn() const { return CurrentTarget != nullptr; }

	UFUNCTION(BlueprintPure, Category="Targeting")
	AEnemyCharacter* GetCurrentTarget() const { return CurrentTarget; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	float TargetingRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	float MaxTargetingAngleDegrees = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting|Debug")
	bool bDebugTargeting = false;

private:
	UPROPERTY()
	AEnemyCharacter* CurrentTarget = nullptr;

	AEnemyCharacter* FindBestTarget() const;
	bool IsTargetValid(const AEnemyCharacter* Target) const;
};