#include "AttackSetDataAsset.h"

bool UAttackSetDataAsset::FindStarterAttack(EAttackInputType InputType, FAttackData& OutAttackData) const
{
	for (const FAttackData& AttackData : Attacks)
	{
		if (!AttackData.bCanStartAttack)
		{
			continue;
		}

		if (AttackData.StarterInputType != InputType)
		{
			continue;
		}

		OutAttackData = AttackData;
		return true;
	}

	return false;
}

bool UAttackSetDataAsset::FindAttackById(FName AttackId, FAttackData& OutAttackData) const
{
	if (AttackId.IsNone())
	{
		return false;
	}

	for (const FAttackData& AttackData : Attacks)
	{
		if (AttackData.AttackId != AttackId)
		{
			continue;
		}

		OutAttackData = AttackData;
		return true;
	}

	return false;
}