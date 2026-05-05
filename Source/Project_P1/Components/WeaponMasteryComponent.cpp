#include "WeaponMasteryComponent.h"

UWeaponMasteryComponent::UWeaponMasteryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponMasteryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ActiveWeaponId.IsNone() && !DefaultWeaponId.IsNone())
	{
		SetActiveWeapon(DefaultWeaponId);
	}
}

void UWeaponMasteryComponent::SetActiveWeapon(FName WeaponId)
{
	if (WeaponId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mastery] SetActiveWeapon failed: WeaponId is None"));
		return;
	}

	ActiveWeaponId = WeaponId;
	FindOrCreateWeaponState(WeaponId);

	UE_LOG(LogTemp, Warning, TEXT("[Mastery] Active weapon set: %s"), *WeaponId.ToString());
}

void UWeaponMasteryComponent::AddWeaponXP(FName WeaponId, float XPAmount)
{
	if (WeaponId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mastery] AddWeaponXP failed: WeaponId is None"));
		return;
	}

	if (XPAmount <= 0.0f)
	{
		return;
	}

	FWeaponMasteryState& State = FindOrCreateWeaponState(WeaponId);

	const int32 OldLevel = State.Level;

	State.XP += XPAmount;
	State.Level = CalculateLevelFromXP(State.XP);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Mastery] %s gained %.1f XP. Total=%.1f Level=%d"),
		*WeaponId.ToString(),
		XPAmount,
		State.XP,
		State.Level
	);

	if (State.Level > OldLevel)
	{
		for (int32 Level = OldLevel + 1; Level <= State.Level; ++Level)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Mastery] %s reached level %d"),
				*WeaponId.ToString(),
				Level
			);

			OnWeaponMasteryLevelChanged.Broadcast(WeaponId, Level, State.XP);
		}
	}
}

int32 UWeaponMasteryComponent::GetWeaponLevel(FName WeaponId) const
{
	const FWeaponMasteryState* State = FindWeaponState(WeaponId);
	if (!State)
	{
		return 1;
	}

	return State->Level;
}

float UWeaponMasteryComponent::GetWeaponXP(FName WeaponId) const
{
	const FWeaponMasteryState* State = FindWeaponState(WeaponId);
	if (!State)
	{
		return 0.0f;
	}

	return State->XP;
}

bool UWeaponMasteryComponent::IsAttackUnlocked(
	FName WeaponId,
	bool bRequiresWeaponMastery,
	int32 RequiredWeaponLevel
) const
{
	if (!bRequiresWeaponMastery)
	{
		return true;
	}

	if (WeaponId.IsNone())
	{
		return false;
	}

	return GetWeaponLevel(WeaponId) >= RequiredWeaponLevel;
}

FWeaponMasteryState& UWeaponMasteryComponent::FindOrCreateWeaponState(FName WeaponId)
{
	for (FWeaponMasteryState& State : WeaponStates)
	{
		if (State.WeaponId == WeaponId)
		{
			return State;
		}
	}

	FWeaponMasteryState NewState;
	NewState.WeaponId = WeaponId;
	NewState.XP = 0.0f;
	NewState.Level = 1;

	WeaponStates.Add(NewState);
	return WeaponStates.Last();
}

const FWeaponMasteryState* UWeaponMasteryComponent::FindWeaponState(FName WeaponId) const
{
	for (const FWeaponMasteryState& State : WeaponStates)
	{
		if (State.WeaponId == WeaponId)
		{
			return &State;
		}
	}

	return nullptr;
}

int32 UWeaponMasteryComponent::CalculateLevelFromXP(float XP) const
{
	int32 Level = 1;

	for (int32 Index = 0; Index < LevelThresholds.Num(); ++Index)
	{
		if (XP >= LevelThresholds[Index])
		{
			Level = Index + 1;
		}
	}

	return Level;
}