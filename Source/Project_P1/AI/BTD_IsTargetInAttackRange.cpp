#include "BTD_IsTargetInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "../Characters/EnemyCharacter.h"

UBTD_IsTargetInAttackRange::UBTD_IsTargetInAttackRange()
{
	NodeName = TEXT("In Attack Range"); // krótsza nazwa do edytora
}

bool UBTD_IsTargetInAttackRange::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
) const
{
	Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyCharacter)
	{
		return false;
	}

	return EnemyCharacter->IsTargetInAttackRange();
}