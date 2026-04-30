#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StyleComponent.generated.h"

UENUM(BlueprintType)
enum class EStyleRank : uint8
{
	D	UMETA(DisplayName="D"),
	C	UMETA(DisplayName="C"),
	B	UMETA(DisplayName="B"),
	A	UMETA(DisplayName="A"),
	S	UMETA(DisplayName="S"),
	SS	UMETA(DisplayName="SS"),
	SSS	UMETA(DisplayName="SSS")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnStyleChanged,
	float, NewStyle,
	EStyleRank, NewRank,
	float, NormalizedStyle
);

UCLASS(ClassGroup=(Project_P1), meta=(BlueprintSpawnableComponent))
class PROJECT_P1_API UStyleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStyleComponent();

	UFUNCTION(BlueprintPure, Category="Style")
	float GetStyle() const { return CurrentStyle; }

	UFUNCTION(BlueprintPure, Category="Style")
	EStyleRank GetCurrentRank() const;

	UFUNCTION(BlueprintPure, Category="Style")
	float GetNormalizedStyle() const;

	UFUNCTION(BlueprintCallable, Category="Style")
	void RegisterAttackHit(FName AttackId, float BaseStyleValue);

	UPROPERTY(BlueprintAssignable, Category="Style")
	FOnStyleChanged OnStyleChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Style")
	float MaxStyle = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankCThreshold = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankBThreshold = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankAThreshold = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankSThreshold = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankSSThreshold = 2600.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Ranks")
	float RankSSSThreshold = 3500.0f;

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
	
	UPROPERTY(EditDefaultsOnly, Category="Style|Decay")
	bool bEnableStyleDecay = true;

	UPROPERTY(EditDefaultsOnly, Category="Style|Decay")
	float DecayDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="Style|Decay")
	float DecayRatePerSecond = 150.0f;

	UPROPERTY(VisibleInstanceOnly, Category="Style|Decay")
	float TimeSinceLastStyleGain = 0.0f;
	
private:
	int32 CountRecentRepeats(FName AttackId) const;
	float GetRepeatMultiplier(int32 RepeatCount) const;
	void PushAttackToHistory(FName AttackId);
	float GetRankGainMultiplier() const;
	float GetRankDecayMultiplier() const;
	float GetCurrentRankMinThreshold() const;
	float GetNextRankThreshold() const;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
};