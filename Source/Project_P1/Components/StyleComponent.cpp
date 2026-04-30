#include "StyleComponent.h"

UStyleComponent::UStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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
	const float RankGainMultiplier = GetRankGainMultiplier();
	const float StyleToAdd = BaseStyleValue * RepeatMultiplier * RankGainMultiplier;

	CurrentStyle = FMath::Clamp(CurrentStyle + StyleToAdd, 0.0f, MaxStyle);
	TimeSinceLastStyleGain = 0.0f;
	
	PushAttackToHistory(AttackId);

	OnStyleChanged.Broadcast(
		CurrentStyle,
		GetCurrentRank(),
		GetNormalizedStyle()
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Style] Attack=%s Repeats=%d RepeatMultiplier=%.2f RankMultiplier=%.2f Added=%.2f CurrentStyle=%.2f Rank=%d Normalized=%.2f"),
		*AttackId.ToString(),
		RepeatCount,
		RepeatMultiplier,
		RankGainMultiplier,
		StyleToAdd,
		CurrentStyle,
		static_cast<int32>(GetCurrentRank()),
		GetNormalizedStyle()
	);
}

EStyleRank UStyleComponent::GetCurrentRank() const
{
	if (CurrentStyle >= RankSSSThreshold)
	{
		return EStyleRank::SSS;
	}

	if (CurrentStyle >= RankSSThreshold)
	{
		return EStyleRank::SS;
	}

	if (CurrentStyle >= RankSThreshold)
	{
		return EStyleRank::S;
	}

	if (CurrentStyle >= RankAThreshold)
	{
		return EStyleRank::A;
	}

	if (CurrentStyle >= RankBThreshold)
	{
		return EStyleRank::B;
	}

	if (CurrentStyle >= RankCThreshold)
	{
		return EStyleRank::C;
	}

	return EStyleRank::D;
}

float UStyleComponent::GetNormalizedStyle() const
{
	const float MinThreshold = GetCurrentRankMinThreshold();
	const float NextThreshold = GetNextRankThreshold();

	if (NextThreshold <= MinThreshold)
	{
		return 1.0f;
	}

	return FMath::Clamp(
		(CurrentStyle - MinThreshold) / (NextThreshold - MinThreshold),
		0.0f,
		1.0f
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

float UStyleComponent::GetCurrentRankMinThreshold() const
{
	switch (GetCurrentRank())
	{
	case EStyleRank::SSS:
		return RankSSSThreshold;

	case EStyleRank::SS:
		return RankSSThreshold;

	case EStyleRank::S:
		return RankSThreshold;

	case EStyleRank::A:
		return RankAThreshold;

	case EStyleRank::B:
		return RankBThreshold;

	case EStyleRank::C:
		return RankCThreshold;

	case EStyleRank::D:
	default:
		return 0.0f;
	}
}

float UStyleComponent::GetNextRankThreshold() const
{
	switch (GetCurrentRank())
	{
	case EStyleRank::D:
		return RankCThreshold;

	case EStyleRank::C:
		return RankBThreshold;

	case EStyleRank::B:
		return RankAThreshold;

	case EStyleRank::A:
		return RankSThreshold;

	case EStyleRank::S:
		return RankSSThreshold;

	case EStyleRank::SS:
		return RankSSSThreshold;

	case EStyleRank::SSS:
	default:
		return MaxStyle;
	}
}

void UStyleComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableStyleDecay)
	{
		return;
	}

	if (CurrentStyle <= 0.0f)
	{
		return;
	}

	TimeSinceLastStyleGain += DeltaTime;

	if (TimeSinceLastStyleGain < DecayDelay)
	{
		return;
	}

	const float OldStyle = CurrentStyle;
	const float RankDecayMultiplier = GetRankDecayMultiplier();
	CurrentStyle = FMath::Max(0.0f,CurrentStyle - DecayRatePerSecond * RankDecayMultiplier * DeltaTime);

	if (!FMath::IsNearlyEqual(OldStyle, CurrentStyle))
	{
		OnStyleChanged.Broadcast(
			CurrentStyle,
			GetCurrentRank(),
			GetNormalizedStyle()
		);
	}
}

float UStyleComponent::GetRankGainMultiplier() const
{
	switch (GetCurrentRank())
	{
	case EStyleRank::D:
		return 1.0f;

	case EStyleRank::C:
		return 0.95f;

	case EStyleRank::B:
		return 0.9f;

	case EStyleRank::A:
		return 0.8f;

	case EStyleRank::S:
		return 0.7f;

	case EStyleRank::SS:
		return 0.6f;

	case EStyleRank::SSS:
		return 0.5f;

	default:
		return 1.0f;
	}
}

float UStyleComponent::GetRankDecayMultiplier() const
{
	switch (GetCurrentRank())
	{
	case EStyleRank::D:
	case EStyleRank::C:
		return 1.0f;

	case EStyleRank::B:
		return 1.1f;

	case EStyleRank::A:
		return 1.25f;

	case EStyleRank::S:
		return 1.45f;

	case EStyleRank::SS:
		return 1.7f;

	case EStyleRank::SSS:
		return 2.0f;

	default:
		return 1.0f;
	}
}