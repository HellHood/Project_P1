#include "BTD_CanAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "../Characters/EnemyCharacter.h"
UBTD_CanAttack::UBTD_CanAttack()
{
	NodeName = TEXT("Can Attack");
}

bool UBTD_CanAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	Super::CalculateRawConditionValue(OwnerComp, NodeMemory);
	UE_LOG(LogTemp, Warning, TEXT("[BTD_CanAttack] Evaluated"));
	
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

	const bool bCanAttack = EnemyCharacter->CanAttack();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[BTD_CanAttack] Result: %s"),
		bCanAttack ? TEXT("true") : TEXT("false")
	);

	return bCanAttack;
}