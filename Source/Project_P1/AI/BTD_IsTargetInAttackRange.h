#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_IsTargetInAttackRange.generated.h"

UCLASS()
class PROJECT_P1_API UBTD_IsTargetInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTD_IsTargetInAttackRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};