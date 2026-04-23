#include "BTTask_AttackTarget.h"

#include "../AI/EnemyAIController.h"
#include "../Characters/EnemyCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_AttackTarget] Failed: missing AIController"));
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_AttackTarget] Failed: controlled pawn is not EnemyCharacter"));
		return EBTNodeResult::Failed;
	}

	const bool bAttackStarted = EnemyCharacter->TryAttackFromAI();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[BTTask_AttackTarget] Result: %s"),
		bAttackStarted ? TEXT("Succeeded") : TEXT("Failed")
	);

	return bAttackStarted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}