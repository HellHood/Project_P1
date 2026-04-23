#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UBlackboardData;

UCLASS()
class PROJECT_P1_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI")
	UBehaviorTree* BehaviorTreeAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI")
	UBlackboardData* BlackboardAsset = nullptr;
};