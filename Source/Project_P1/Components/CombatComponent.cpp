#include "CombatComponent.h"

#include "../Characters/BaseCharacter.h"
#include "../Characters/EnemyCharacter.h"
#include "../Components/HealthComponent.h"
#include "../Components/StyleComponent.h"
#include "../Data/AttackSetDataAsset.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static const ECollisionChannel MeleeChannel = ECC_GameTraceChannel1;

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UCombatComponent::RequestAttack(EAttackInputType InputType)
{
	if (bIsAttacking)
	{
		bHasBufferedAttack = true;
		BufferedInputType = InputType;

		if (const UWorld* World = GetWorld())
		{
			BufferedInputTime = World->GetTimeSeconds() - AttackStartTime;
		}
		else
		{
			BufferedInputTime = 0.0f;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack buffered"));
		return false;
	}

	if (bAttackOnCooldown)
	{
		return false;
	}

	FAttackData AttackData;
	if (!ResolveDefaultAttackData(InputType, AttackData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] No default attack found for requested input"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack started: %s"), *AttackData.AttackId.ToString());

	BeginAttack(AttackData);
	return true;
}

bool UCombatComponent::RequestAttackById(FName AttackId)
{
	if (bIsAttacking)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack by id blocked: already attacking"));
		return false;
	}

	if (bAttackOnCooldown)
	{
		return false;
	}

	FAttackData AttackData;
	if (!ResolveAttackById(AttackId, AttackData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack by id failed: %s"), *AttackId.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack started: %s"), *AttackData.AttackId.ToString());

	BeginAttack(AttackData);
	return true;
}

void UCombatComponent::BeginAttack(const FAttackData& AttackData)
{
	CurrentAttackData = AttackData;
	OnAttackStarted.Broadcast(CurrentAttackData);

	bIsAttacking = true;
	bAttackOnCooldown = true;
	bHitWindowActive = false;
	bStyleRegisteredThisAttack = false;
	bHasBufferedAttack = false;
	BufferedInputTime = 0.0f;
	HitActorsThisWindow.Empty();

	if (const UWorld* World = GetWorld())
	{
		AttackStartTime = World->GetTimeSeconds();
	}
	else
	{
		AttackStartTime = 0.0f;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		AttackCooldownHandle,
		this,
		&UCombatComponent::ResetAttackCooldown,
		CurrentAttackData.Cooldown,
		false
	);

	World->GetTimerManager().SetTimer(
		HitWindowStartHandle,
		this,
		&UCombatComponent::StartHitWindow,
		CurrentAttackData.HitStartTime,
		false
	);

	World->GetTimerManager().SetTimer(
		HitWindowEndHandle,
		this,
		&UCombatComponent::EndHitWindow,
		CurrentAttackData.HitEndTime,
		false
	);

	World->GetTimerManager().SetTimer(
		AttackDurationHandle,
		this,
		&UCombatComponent::EndAttack,
		CurrentAttackData.Duration,
		false
	);
}

void UCombatComponent::StartHitWindow()
{
	if (bHitWindowActive)
	{
		return;
	}

	bHitWindowActive = true;
	HitActorsThisWindow.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		HitWindowTickHandle,
		this,
		&UCombatComponent::ProcessHitWindowTick,
		0.02f,
		true
	);
}

void UCombatComponent::ProcessHitWindowTick()
{
	if (!bHitWindowActive)
	{
		return;
	}

	TArray<FHitResult> Hits;
	const bool bHit = TraceCurrentAttack(Hits);

	if (!bHit)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(OwnerActor);

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector OwnerForward = OwnerActor->GetActorForwardVector();

	AController* InstigatorController = nullptr;
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		InstigatorController = OwnerPawn->GetController();
	}

	// Prevent multiple damage applications to the same actor from one trace.
	TSet<AActor*> UniqueActors;
	TArray<AActor*> ValidTargets;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// Ignore self.
		if (HitActor == OwnerActor)
		{
			continue;
		}

		if (UniqueActors.Contains(HitActor))
		{
			continue;
		}

		UniqueActors.Add(HitActor);

		// Ignore already hit actors during the same active hit window.
		if (HitActorsThisWindow.Contains(HitActor))
		{
			continue;
		}

		ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(HitActor);
		if (OwnerCharacter && HitCharacter)
		{
			if (OwnerCharacter->GetCombatFaction() == HitCharacter->GetCombatFaction())
			{
				continue;
			}
		}

		UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
		if (!HealthComp)
		{
			continue;
		}

		if (HealthComp->IsDead())
		{
			continue;
		}

		ValidTargets.Add(HitActor);
	}

	if (ValidTargets.IsEmpty())
	{
		return;
	}

	// Register style once per successful attack, not once per target or per tick.
	if (!bStyleRegisteredThisAttack)
	{
		if (UStyleComponent* StyleComp = OwnerActor->FindComponentByClass<UStyleComponent>())
		{
			StyleComp->RegisterAttackHit(
				CurrentAttackData.AttackId,
				CurrentAttackData.BaseStyleValue
			);
		}

		bStyleRegisteredThisAttack = true;
	}

	// Choose the target that feels most natural for the current swing.
	AActor* PrimaryTarget = nullptr;
	float BestScore = -9999.f;

	for (AActor* TargetActor : ValidTargets)
	{
		const FVector ToTarget = TargetActor->GetActorLocation() - OwnerLocation;
		const float Distance = ToTarget.Size();
		const FVector Direction = ToTarget.GetSafeNormal();

		const float Dot = FVector::DotProduct(OwnerForward, Direction);
		const float DistanceScore = 1.0f - FMath::Clamp(Distance / CurrentAttackData.Range, 0.f, 1.f);
		const float Score = (Dot * 0.7f) + (DistanceScore * 0.3f);

		if (Score > BestScore)
		{
			BestScore = Score;
			PrimaryTarget = TargetActor;
		}
	}

	const float PrimaryReactionMultiplier = 1.0f;
	const float SecondaryReactionMultiplier = 0.5f;

	for (AActor* TargetActor : ValidTargets)
	{
		HitActorsThisWindow.Add(TargetActor);

		const FVector HitDirection = (TargetActor->GetActorLocation() - OwnerLocation).GetSafeNormal();
		const float ReactionMultiplier =
			TargetActor == PrimaryTarget ? PrimaryReactionMultiplier : SecondaryReactionMultiplier;

		const float KnockbackStrength = CurrentAttackData.KnockbackStrength * ReactionMultiplier;
		const float LaunchStrength = CurrentAttackData.LaunchStrength * ReactionMultiplier;
		const float CarrySpeed = CurrentAttackData.CarrySpeed * ReactionMultiplier;
		const float CarryDuration = CurrentAttackData.CarryDuration;

		if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(TargetActor))
		{
			EnemyCharacter->SetPendingHitReaction(
				CurrentAttackData.HitReactionType,
				HitDirection,
				KnockbackStrength,
				LaunchStrength,
				CarrySpeed,
				CarryDuration
			);
		}

		UGameplayStatics::ApplyDamage(
			TargetActor,
			CurrentAttackData.Damage,
			InstigatorController,
			OwnerActor,
			UDamageType::StaticClass()
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Combat] Hit confirmed: %s Primary=%d"),
			*TargetActor->GetName(),
			TargetActor == PrimaryTarget ? 1 : 0
		);
	}
}

void UCombatComponent::EndHitWindow()
{
	if (!bHitWindowActive)
	{
		return;
	}

	bHitWindowActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitWindowTickHandle);
	}
}

void UCombatComponent::EndAttack()
{
	bIsAttacking = false;
	bHitWindowActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackDurationHandle);
		World->GetTimerManager().ClearTimer(HitWindowStartHandle);
		World->GetTimerManager().ClearTimer(HitWindowEndHandle);
		World->GetTimerManager().ClearTimer(HitWindowTickHandle);
	}

	HitActorsThisWindow.Empty();

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack ended: %s"), *CurrentAttackData.AttackId.ToString());

	if (!bHasBufferedAttack)
	{
		return;
	}

	const EAttackInputType NextInputType = BufferedInputType;
	const float InputTime = BufferedInputTime;

	bHasBufferedAttack = false;
	BufferedInputTime = 0.0f;

	FAttackData NextAttackData;
	if (ResolveTransitionAttack(NextInputType, InputTime, NextAttackData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Transition accepted: %s"), *NextAttackData.AttackId.ToString());
		BeginAttack(NextAttackData);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Buffered input had no valid transition"));
}

void UCombatComponent::ResetAttackCooldown()
{
	bAttackOnCooldown = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackCooldownHandle);
	}
}

bool UCombatComponent::TraceCurrentAttack(TArray<FHitResult>& OutHits) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector End = Start + Forward * CurrentAttackData.Range;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);

	const bool bHit = World->SweepMultiByChannel(
		OutHits,
		Start,
		End,
		FQuat::Identity,
		MeleeChannel,
		FCollisionShape::MakeSphere(CurrentAttackData.Radius),
		Params
	);

	if (bDrawAttackDebug)
	{
		DrawDebugLine(World, Start, End, FColor::Red, false, 1.0f);
		DrawDebugSphere(World, End, CurrentAttackData.Radius, 16, FColor::Red, false, 1.0f);
	}

	return bHit;
}

bool UCombatComponent::ResolveDefaultAttackData(EAttackInputType InputType, FAttackData& OutAttackData) const
{
	if (!AttackSet)
	{
		return false;
	}

	return AttackSet->FindStarterAttack(InputType, OutAttackData);
}

bool UCombatComponent::ResolveTransitionAttack(EAttackInputType InputType, float InputTime, FAttackData& OutAttackData) const
{
	for (const FAttackTransition& Transition : CurrentAttackData.Transitions)
	{
		if (Transition.InputType != InputType)
		{
			continue;
		}

		const bool bUseTimingWindow =
			!FMath::IsNearlyZero(Transition.WindowStart) ||
			!FMath::IsNearlyZero(Transition.WindowEnd);

		if (bUseTimingWindow)
		{
			if (InputTime < Transition.WindowStart || InputTime > Transition.WindowEnd)
			{
				continue;
			}
		}

		return ResolveAttackById(Transition.NextAttackId, OutAttackData);
	}

	return false;
}

bool UCombatComponent::ResolveAttackById(FName AttackId, FAttackData& OutAttackData) const
{
	if (!AttackSet)
	{
		return false;
	}

	return AttackSet->FindAttackById(AttackId, OutAttackData);
}

void UCombatComponent::SetAttackSet(UAttackSetDataAsset* NewAttackSet)
{
	if (bIsAttacking)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] AttackSet change blocked: attack in progress"));
		return;
	}

	AttackSet = NewAttackSet;
}

bool UCombatComponent::CanStartAttackById(FName AttackId) const
{
	if (bIsAttacking)
	{
		return false;
	}

	if (bAttackOnCooldown)
	{
		return false;
	}

	if (AttackId.IsNone())
	{
		return false;
	}

	FAttackData AttackData;
	return ResolveAttackById(AttackId, AttackData);
}

bool UCombatComponent::HasAttackById(FName AttackId) const
{
	if (AttackId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] HasAttackById failed: AttackId is None"));
		return false;
	}

	if (!AttackSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] HasAttackById failed: AttackSet is NULL on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	FAttackData AttackData;
	const bool bFoundAttack = ResolveAttackById(AttackId, AttackData);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Combat] HasAttackById %s in %s -> %s"),
		*AttackId.ToString(),
		*GetNameSafe(AttackSet),
		bFoundAttack ? TEXT("FOUND") : TEXT("NOT FOUND")
	);

	return bFoundAttack;
}

bool UCombatComponent::GetAttackDataById(FName AttackId, FAttackData& OutAttackData) const
{
	return ResolveAttackById(AttackId, OutAttackData);
}