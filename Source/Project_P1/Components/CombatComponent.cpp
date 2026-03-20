#include "CombatComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "../Components/HealthComponent.h"

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
	if (bIsAttacking)
	{
		bBufferedLightAttack = true;
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack buffered"));
		return false;
	}

	if (bLightAttackOnCooldown)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack started"));

	StartAttack(LightAttackData);
	return true;
}

void UCombatComponent::StartAttack(const FAttackData& AttackData)
{
	CurrentAttackData = AttackData;

	bIsAttacking = true;
	bLightAttackOnCooldown = true;

	GetWorld()->GetTimerManager().SetTimer(
		LightAttackCooldownHandle,
		this,
		&UCombatComponent::ResetLightAttackCooldown,
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

	UE_LOG(LogTemp, Warning, TEXT("[Combat] Attack ended"));

	if (bBufferedLightAttack)
	{
		bBufferedLightAttack = false;

		UE_LOG(LogTemp, Warning, TEXT("[Combat] Buffered attack consumed"));

		TryLightAttack();
	}
}

void UCombatComponent::ResetLightAttackCooldown()
{
	bLightAttackOnCooldown = false;
	GetWorld()->GetTimerManager().ClearTimer(LightAttackCooldownHandle);
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