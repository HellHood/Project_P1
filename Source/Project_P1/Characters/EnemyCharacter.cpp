#include "EnemyCharacter.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_P1/Components/HealthComponent.h"

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
	GetWorldTimerManager().SetTimer(RepathHandle, this, &AEnemyCharacter::RepathTick, RepathInterval, true);
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (UHealthComponent* HC = GetHealthComponent())
	{
		if (HC->IsDead())
		{
			return;
		}
	}

	if (!IsTargetValid())
	{
		TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		return;
	}

	const float Dist = DistanceToTarget2D();
	

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
	return IsValid(TargetPawn);
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
	if (!HasLineOfSightToTarget()) return;
	
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

void AEnemyCharacter::RepathTick()
{
	if (!IsTargetValid()) return;

	// Dead guard (important when you stop destroying enemies on death)
	if (UHealthComponent* HC = GetHealthComponent(); HC && HC->IsDead()) return;

	const float Dist = DistanceToTarget2D();
	if (Dist <= ChaseRange)
	{
		ChaseTarget();
	}
}

bool AEnemyCharacter::HasLineOfSightToTarget() const
{
	if (!IsTargetValid()) return false;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const FVector Start = GetActorLocation() + FVector(0,0,50);
	const FVector End   = TargetPawn->GetActorLocation() + FVector(0,0,50);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	// If nothing hit, LOS is clear. If hit, must be the target.
	return !bHit || Hit.GetActor() == TargetPawn;
}