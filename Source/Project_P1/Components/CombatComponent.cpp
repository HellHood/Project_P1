#include "CombatComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h" 
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "../Components/HealthComponent.h"
#include "../Components/HealthComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

static const ECollisionChannel MeleeChannel = ECC_GameTraceChannel1;


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UCombatComponent::PerformLightAttackHit()
{
	FHitResult Hit;
	const bool bHit = DoLightAttackTrace(Hit);

	if (bHit && Hit.GetActor())
	{
		AActor* OwnerActor = GetOwner();

		UGameplayStatics::ApplyDamage(
			Hit.GetActor(),
			LightAttackDamage,
			nullptr,
			OwnerActor,
			UDamageType::StaticClass()
		);

		UE_LOG(LogTemp, Warning, TEXT("[Combat] HIT confirmed: %s"), *Hit.GetActor()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] HIT missed"));
	}
}

void UCombatComponent::EndAttack()
{
	bIsAttacking = false;

	GetWorld()->GetTimerManager().ClearTimer(AttackDurationHandle);
	GetWorld()->GetTimerManager().ClearTimer(AttackHitHandle);
	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack END"));
}

bool UCombatComponent::TryLightAttack()
{
	if (bLightAttackOnCooldown || bIsAttacking)
	{
		return false;
	}

	bIsAttacking = true;
	bLightAttackOnCooldown = true;
	
	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack START"));

	// Start cooldown
	GetWorld()->GetTimerManager().SetTimer(
		LightAttackCooldownHandle,
		this,
		&UCombatComponent::ResetLightAttackCooldown,
		LightAttackCooldown,
		false
	);

	// Schedule HIT moment
	GetWorld()->GetTimerManager().SetTimer(
		AttackHitHandle,
		this,
		&UCombatComponent::PerformLightAttackHit,
		LightAttackHitTime,
		false
	);

	// End attack
	GetWorld()->GetTimerManager().SetTimer(
		AttackDurationHandle,
		this,
		&UCombatComponent::EndAttack,
		LightAttackDuration,
		false
	);

	return true;
}

void UCombatComponent::ResetLightAttackCooldown()
{
	bLightAttackOnCooldown = false;
	GetWorld()->GetTimerManager().ClearTimer(LightAttackCooldownHandle);
}

bool UCombatComponent::DoLightAttackTrace(FHitResult& OutHit) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	// v0: forward sphere sweep from owner's position.
	// NOTE: Later we will source Start/End from a weapon socket or animation-driven hit window.

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector End = Start + Forward * LightAttackRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);

	const bool bHit = World->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		MeleeChannel,
		FCollisionShape::MakeSphere(LightAttackRadius),
		Params
	);

	if (bDrawAttackDebug)
	{
		DrawDebugLine(World, Start, End, FColor::Red, false, 1.0f);
		DrawDebugSphere(World, End, LightAttackRadius, 16, FColor::Red, false, 1.0f);
	}

	return bHit;
}

