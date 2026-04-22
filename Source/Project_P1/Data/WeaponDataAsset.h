#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

class UStaticMesh;
class UAttackSetDataAsset;

UCLASS(BlueprintType)
class PROJECT_P1_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	UStaticMesh* WeaponMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	UAttackSetDataAsset* AttackSet = nullptr;
};