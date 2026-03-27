#include "CombatComponent.h"

#include "../Components/HealthComponent.h"
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
	// If an attack is already active, store the input for transition resolution.
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

void UCombatComponent::BeginAttack(const FAttackData& AttackData)
{
	CurrentAttackData = AttackData;

	bIsAttacking = true;
	bAttackOnCooldown = true;
	bHasBufferedAttack = false;
	BufferedInputTime = 0.0f;

	if (const UWorld* World = GetWorld())
	{
		AttackStartTime = World->GetTimeSeconds();
	}
	else
	{
		AttackStartTime = 0.0f;
	}

	GetWorld()->GetTimerManager().SetTimer(
		AttackCooldownHandle,
		this,
		&UCombatComponent::ResetAttackCooldown,
		CurrentAttackData.Cooldown,
		false
	);

	GetWorld()->GetTimerManager().SetTimer(
		AttackHitHandle,
		this,
		&UCombatComponent::ExecuteCurrentAttackHit,
		CurrentAttackData.HitTime,
		false
	);

	GetWorld()->GetTimerManager().SetTimer(
		AttackDurationHandle,
		this,
		&UCombatComponent::EndAttack,
		CurrentAttackData.Duration,
		false
	);
}

void UCombatComponent::ExecuteCurrentAttackHit()
{
	FHitResult Hit;
	const bool bHit = TraceCurrentAttack(Hit);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit missed"));
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit result had no actor"));
		return;
	}

	UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
	if (!HealthComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit ignored: %s has no HealthComponent"), *HitActor->GetName());
		return;
	}

	if (HealthComp->IsDead())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit ignored: %s is already dead"), *HitActor->GetName());
		return;
	}

	AActor* OwnerActor = GetOwner();

	AController* InstigatorController = nullptr;
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		InstigatorController = OwnerPawn->GetController();
	}

	UGameplayStatics::ApplyDamage(
		HitActor,
		CurrentAttackData.Damage,
		InstigatorController,
		OwnerActor,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit confirmed: %s"), *HitActor->GetName());
}

void UCombatComponent::EndAttack()
{
	bIsAttacking = false;

	GetWorld()->GetTimerManager().ClearTimer(AttackDurationHandle);
	GetWorld()->GetTimerManager().ClearTimer(AttackHitHandle);

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
	GetWorld()->GetTimerManager().ClearTimer(AttackCooldownHandle);
}

bool UCombatComponent::TraceCurrentAttack(FHitResult& OutHit) const
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

	const bool bHit = World->SweepSingleByChannel(
		OutHit,
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
	switch (InputType)
	{
	case EAttackInputType::Light:
		OutAttackData = LightAttackData;
		return true;

	case EAttackInputType::Heavy:
		OutAttackData = HeavyAttackData;
		return true;

	default:
		return false;
	}
}

bool UCombatComponent::ResolveTransitionAttack(EAttackInputType InputType, float InputTime, FAttackData& OutAttackData) const
{
	for (const FAttackTransition& Transition : CurrentAttackData.Transitions)
	{
		if (Transition.InputType != InputType)
		{
			continue;
		}

		// If both values are zero, treat the transition like an untimed follow-up.
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
	if (AttackId == LightAttackData.AttackId)
	{
		OutAttackData = LightAttackData;
		return true;
	}

	if (AttackId == LightFollowupData.AttackId)
	{
		OutAttackData = LightFollowupData;
		return true;
	}

	if (AttackId == HeavyAttackData.AttackId)
	{
		OutAttackData = HeavyAttackData;
		return true;
	}

	return false;
}