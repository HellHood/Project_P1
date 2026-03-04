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

bool UCombatComponent::TryLightAttack()
{
	// Prevent spamming: 1 attack per cooldown window.
	if (bLightAttackOnCooldown)
	{
		return false;
	}

	FHitResult Hit;
	const bool bHit = DoLightAttackTrace(Hit);

	// Start cooldown regardless of hit, so attack has consistent tempo.
	bLightAttackOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(
		LightAttackCooldownHandle,
		this,
		&UCombatComponent::ResetLightAttackCooldown,
		LightAttackCooldown,
		false
	);

	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack: hit result had no actor"));
			return false;
		}

		// Filter #1: must have HealthComponent
		UHealthComponent* HC = HitActor->FindComponentByClass<UHealthComponent>();
		if (!HC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack: %s has no HealthComponent"), *HitActor->GetName());
			return false;
		}

		// Filter #2: ignore dead targets
		if (HC->IsDead())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack: %s is already dead"), *HitActor->GetName());
			return false;
		}

		// Determine instigator controller if owner is a pawn (player/enemy later)
		AController* InstigatorController = nullptr;
		if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			LightAttackDamage,
			InstigatorController,
			GetOwner(),                 // DamageCauser (owner of combat)
			UDamageType::StaticClass()
		);

		UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack valid hit: %s (Damage=%.1f)"),
			*HitActor->GetName(), LightAttackDamage);

		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack: no hit"));
	return false;
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