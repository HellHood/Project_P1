#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../Components/CombatComponent.h"
#include "AttackSetDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECT_P1_API UAttackSetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack Set")
	TArray<FAttackData> Attacks;

	bool FindStarterAttack(EAttackInputType InputType, FAttackData& OutAttackData) const;
	bool FindAttackById(FName AttackId, FAttackData& OutAttackData) const;
};