#include "StyleComponent.h"

UStyleComponent::UStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStyleComponent::RegisterAttackHit(FName AttackId, float BaseStyleValue)
{
	if (AttackId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Style] RegisterAttackHit failed: AttackId is None"));
		return;
	}

	if (BaseStyleValue <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Style] RegisterAttackHit failed: BaseStyleValue must be > 0"));
		return;
	}

	const int32 RepeatCount = CountRecentRepeats(AttackId);
	const float RepeatMultiplier = GetRepeatMultiplier(RepeatCount);
	const float StyleToAdd = BaseStyleValue * RepeatMultiplier;

	CurrentStyle = FMath::Clamp(CurrentStyle + StyleToAdd, 0.0f, MaxStyle);

	PushAttackToHistory(AttackId);

	OnStyleChanged.Broadcast(CurrentStyle);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Style] Attack=%s Repeats=%d Multiplier=%.2f Added=%.2f CurrentStyle=%.2f"),
		*AttackId.ToString(),
		RepeatCount,
		RepeatMultiplier,
		StyleToAdd,
		CurrentStyle
	);
}

int32 UStyleComponent::CountRecentRepeats(FName AttackId) const
{
	int32 RepeatCount = 0;

	for (const FName& RecentAttackId : RecentAttackHistory)
	{
		if (RecentAttackId == AttackId)
		{
			++RepeatCount;
		}
	}

	return RepeatCount;
}

float UStyleComponent::GetRepeatMultiplier(int32 RepeatCount) const
{
	if (RepeatCount <= 0)
	{
		return 1.0f;
	}

	if (RepeatCount == 1)
	{
		return FirstRepeatMultiplier;
	}

	if (RepeatCount == 2)
	{
		return SecondRepeatMultiplier;
	}

	return ThirdRepeatMultiplier;
}

void UStyleComponent::PushAttackToHistory(FName AttackId)
{
	RecentAttackHistory.Add(AttackId);

	while (RecentAttackHistory.Num() > RecentAttackMemorySize)
	{
		RecentAttackHistory.RemoveAt(0);
	}
}