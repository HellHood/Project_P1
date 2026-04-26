#include "BTTask_AttackTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "../Characters/EnemyCharacter.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	bNotifyTaskFinished = true;
}

uint16 UBTTask_AttackTarget::GetInstanceMemorySize() const
{
	return sizeof(FBTTask_AttackTargetMemory);
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyCharacter)
	{
		return EBTNodeResult::Failed;
	}

	float AttackDuration = 0.f;
	const bool bAttackStarted = EnemyCharacter->TryAttackFromAI(AttackDuration);
	if (!bAttackStarted)
	{
		return EBTNodeResult::Failed;
	}

	if (AttackDuration <= 0.f)
	{
		return EBTNodeResult::Succeeded;
	}

	FBTTask_AttackTargetMemory* TaskMemory = reinterpret_cast<FBTTask_AttackTargetMemory*>(NodeMemory);
	if (!TaskMemory)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	// Keep BT inside attack task until the attack's authored duration has elapsed.
	World->GetTimerManager().SetTimer(
		TaskMemory->TimerHandle,
		FTimerDelegate::CreateWeakLambda(&OwnerComp, [&OwnerComp, this]()
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}),
		AttackDuration,
		false
	);

	return EBTNodeResult::InProgress;
}

void UBTTask_AttackTarget::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTNodeResult::Type TaskResult
)
{
	FBTTask_AttackTargetMemory* TaskMemory = reinterpret_cast<FBTTask_AttackTargetMemory*>(NodeMemory);
	if (!TaskMemory)
	{
		return;
	}

	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(TaskMemory->TimerHandle);
	}
}