#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StyleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStyleChanged, float, NewStyle);

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UStyleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStyleComponent();

	UFUNCTION(BlueprintPure, Category="Style")
	float GetStyle() const { return CurrentStyle; }

	UFUNCTION(BlueprintCallable, Category="Style")
	void RegisterAttackHit(FName AttackId, float BaseStyleValue);

	UPROPERTY(BlueprintAssignable, Category="Style")
	FOnStyleChanged OnStyleChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Style")
	float MaxStyle = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	int32 RecentAttackMemorySize = 4;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	float FirstRepeatMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	float SecondRepeatMultiplier = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	float ThirdRepeatMultiplier = 0.2f;

	UPROPERTY(VisibleInstanceOnly, Category="Style")
	float CurrentStyle = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category="Style")
	TArray<FName> RecentAttackHistory;

private:
	int32 CountRecentRepeats(FName AttackId) const;
	float GetRepeatMultiplier(int32 RepeatCount) const;
	void PushAttackToHistory(FName AttackId);
};