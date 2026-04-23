#include "EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/GameplayStatics.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BlackboardAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Missing BlackboardAsset on %s"), *GetName());
		return;
	}

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Missing BehaviorTreeAsset on %s"), *GetName());
		return;
	}

	UBlackboardComponent* BlackboardComp = Blackboard.Get();
	if (!UseBlackboard(BlackboardAsset, BlackboardComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Failed to initialize blackboard on %s"), *GetName());
		return;
	}

	RunBehaviorTree(BehaviorTreeAsset);

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		BlackboardComp->SetValueAsObject(TEXT("TargetActor"), PlayerPawn);
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] TargetActor set to %s"), *PlayerPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] PlayerPawn not found"));
	}
}