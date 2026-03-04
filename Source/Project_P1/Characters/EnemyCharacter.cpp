#include "EnemyCharacter.h"

#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Enemy movement feel (tweak later)
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->MaxWalkSpeed = 450.f;
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// v0: first player pawn
	TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsTargetValid())
	{
		TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		return;
	}

	const float Dist = DistanceToTarget2D();

	// Chase if in range
	if (Dist <= ChaseRange)
	{
		ChaseTarget();
	}

	// Attack if close enough
	if (Dist <= AttackRange)
	{
		TryAttackTarget();
	}

	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Dist=%.1f Chase=%d AttackCD=%d"),
			Dist, Dist <= ChaseRange ? 1 : 0, bAttackOnCooldown ? 1 : 0);
	}
}

bool AEnemyCharacter::IsTargetValid() const
{
	return TargetPawn && !TargetPawn->IsPendingKill();
}

float AEnemyCharacter::DistanceToTarget2D() const
{
	if (!IsTargetValid()) return TNumericLimits<float>::Max();

	const FVector A = GetActorLocation();
	const FVector B = TargetPawn->GetActorLocation();
	return FVector::Dist2D(A, B);
}

void AEnemyCharacter::ChaseTarget()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	// v0: spam-safe enough; later we'll throttle / use BT
	AIC->MoveToActor(TargetPawn, /*AcceptanceRadius*/ AttackRange - 10.f, /*bStopOnOverlap*/ true);
}

void AEnemyCharacter::TryAttackTarget()
{
	if (bAttackOnCooldown) return;

	// Start cooldown immediately so we don't multi-hit same frame
	bAttackOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		AttackCooldownHandle,
		this,
		&AEnemyCharacter::ResetAttackCooldown,
		AttackCooldown,
		false
	);

	// Apply damage through Unreal pipeline (HealthComponent listens on the player)
	AController* InstigatorController = GetController();
	UGameplayStatics::ApplyDamage(
		TargetPawn,
		AttackDamage,
		InstigatorController,
		this,
		UDamageType::StaticClass()
	);

	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack! Damage=%.1f"), AttackDamage);
	}
}

void AEnemyCharacter::ResetAttackCooldown()
{
	bAttackOnCooldown = false;
	GetWorldTimerManager().ClearTimer(AttackCooldownHandle);
}