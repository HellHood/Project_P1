#include "EnemyCharacter.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Project_P1/Components/HealthComponent.h"
#include "TimerManager.h"
#include "../Components/CombatComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CombatFaction = ECombatFaction::Enemy;

	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = 450.f;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	
	GetWorldTimerManager().SetTimer(
		RepathHandle,
		this,
		&AEnemyCharacter::RepathTick,
		RepathInterval,
		true
	);

	if (UHealthComponent* HealthComp = GetHealthComponent())
	{
		HealthComp->OnDeath.AddDynamic(this, &AEnemyCharacter::HandleEnemyDeath);
		HealthComp->OnHealthChanged.AddDynamic(this, &AEnemyCharacter::HandleEnemyHealthChanged);
	}

	if (UCombatComponent* CombatComp = GetCombatComponent())
	{
		if (EnemyAttackSet)
		{
			CombatComp->SetAttackSet(EnemyAttackSet);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy] Missing EnemyAttackSet on %s"), *GetName());
		}
		
			CombatComp->OnAttackStarted.AddDynamic(this, &AEnemyCharacter::HandleAttackStarted);
		
	}
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	// Carry forward temporarily overrides normal movement.
	if (bIsCarryForwardActive)
	{
		ActiveCarryTimeRemaining -= DeltaSeconds;

		if (ActiveCarryTimeRemaining <= 0.f)
		{
			bIsCarryForwardActive = false;
			ActiveCarryDirection = FVector::ZeroVector;
			ActiveCarrySpeed = 0.f;
			ActiveCarryTimeRemaining = 0.f;

			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				MoveComp->StopMovementImmediately();
			}
		}
		else
		{
			const FVector DeltaMove = ActiveCarryDirection * ActiveCarrySpeed * DeltaSeconds;
			AddActorWorldOffset(DeltaMove, true);
		}

		return;
	}
}

void AEnemyCharacter::SetPendingHitReaction(
	EHitReactionType ReactionType,
	const FVector& Direction,
	float KnockbackStrength,
	float LaunchStrength,
	float CarrySpeed,
	float CarryDuration
)
{
	bHasPendingHitReaction = true;

	PendingReactionType = ReactionType;
	PendingHitReactionDirection = Direction;

	PendingKnockbackStrength = KnockbackStrength;
	PendingLaunchStrength = LaunchStrength;
	PendingCarrySpeed = CarrySpeed;
	PendingCarryDuration = CarryDuration;
}

bool AEnemyCharacter::IsTargetValid() const
{
	return IsValid(TargetPawn);
}

float AEnemyCharacter::DistanceToTarget2D() const
{
	if (!IsTargetValid())
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist2D(GetActorLocation(), TargetPawn->GetActorLocation());
}

bool AEnemyCharacter::HasLineOfSightToTarget() const
{
	if (!IsTargetValid())
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = TargetPawn->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	return !bHit || Hit.GetActor() == TargetPawn;
}

void AEnemyCharacter::ChaseTarget()
{
	if (bIsDead || bIsCarryForwardActive)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	AIController->MoveToActor(TargetPawn, AttackRange - 10.f, true);
}

bool AEnemyCharacter::TryAttackFromAI()
{
	if (bIsDead || bIsCarryForwardActive)
	{
		return false;
	}

	if (bAttackOnCooldown)
	{
		return false;
	}

	if (!HasLineOfSightToTarget())
	{
		return false;
	}

	if (DefaultAttackId.IsNone())
	{
		if (bDebugEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack skipped: DefaultAttackId is None"));
		}

		return false;
	}

	if (UCombatComponent* CombatComp = GetCombatComponent())
	{
		const bool bAttackStarted = CombatComp->RequestAttackById(DefaultAttackId);
		if (!bAttackStarted)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	bAttackOnCooldown = true;

	GetWorldTimerManager().SetTimer(
		AttackCooldownHandle,
		this,
		&AEnemyCharacter::ResetAttackCooldown,
		AttackCooldown,
		false
	);

	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack triggered: %s"), *DefaultAttackId.ToString());
	}

	return true;
}

void AEnemyCharacter::TryAttackTarget()
{
	TryAttackFromAI();
	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack triggered: %s"), *DefaultAttackId.ToString());
	}
}

void AEnemyCharacter::RepathTick()
{
	if (bIsDead || bIsCarryForwardActive)
	{
		return;
	}

	if (!IsTargetValid())
	{
		return;
	}

	const float DistanceToTarget = DistanceToTarget2D();
	if (DistanceToTarget <= ChaseRange)
	{
		ChaseTarget();
	}
}

void AEnemyCharacter::ResetAttackCooldown()
{
	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Cooldown reset"));
	bAttackOnCooldown = false;
	GetWorldTimerManager().ClearTimer(AttackCooldownHandle);
}

void AEnemyCharacter::HandleEnemyDeath(UHealthComponent* HealthComp, AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsCarryForwardActive = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Enemy] Died. Instigator: %s"),
		InstigatorActor ? *InstigatorActor->GetName() : TEXT("None")
	);

	GetWorldTimerManager().ClearTimer(RepathHandle);
	GetWorldTimerManager().ClearTimer(AttackCooldownHandle);
	bAttackOnCooldown = false;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

void AEnemyCharacter::HandleEnemyHealthChanged(UHealthComponent* HealthComp, float NewHealth, float Delta, AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	// Ignore heal events for hit reaction.
	if (Delta >= 0.f)
	{
		return;
	}

	if (bDebugEnemy)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Enemy] Took damage: %.1f"),
			-Delta
		);
	}

	// Only react if combat code prepared hit reaction data.
	if (!bHasPendingHitReaction)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		switch (PendingReactionType)
		{
		case EHitReactionType::Knockback:
		{
			MoveComp->AddImpulse(PendingHitReactionDirection * PendingKnockbackStrength, true);
			break;
		}

		case EHitReactionType::Launch:
		{
			const FVector LaunchVelocity =
				PendingHitReactionDirection * PendingKnockbackStrength +
				FVector(0.f, 0.f, PendingLaunchStrength);

			LaunchCharacter(LaunchVelocity, true, true);
			break;
		}

		case EHitReactionType::CarryForward:
		{
			bIsCarryForwardActive = true;
			ActiveCarryDirection = PendingHitReactionDirection;
			ActiveCarrySpeed = PendingCarrySpeed;
			ActiveCarryTimeRemaining = PendingCarryDuration;

			if (AAIController* AIController = Cast<AAIController>(GetController()))
			{
				AIController->StopMovement();
			}

			MoveComp->StopMovementImmediately();
			break;
		}

		default:
			break;
		}
	}

	bHasPendingHitReaction = false;
	PendingReactionType = EHitReactionType::None;
	PendingHitReactionDirection = FVector::ZeroVector;
	PendingKnockbackStrength = 0.f;
	PendingLaunchStrength = 0.f;
	PendingCarrySpeed = 0.f;
	PendingCarryDuration = 0.f;
}

void AEnemyCharacter::DeactivateEnemy()
{
	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Deactivated: %s"), *GetName());
}

void AEnemyCharacter::ActivateEnemy()
{
	SetActorTickEnabled(true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);
	}

	bIsCarryForwardActive = false;
	bAttackOnCooldown = false;
	bHasPendingHitReaction = false;

	GetWorldTimerManager().ClearTimer(AttackCooldownHandle);

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Activated: %s"), *GetName());
}

void AEnemyCharacter::HandleAttackStarted(FAttackData AttackData)
{
	if (!AttackData.AttackMontage)
	{
		return;
	}

	PlayAnimMontage(AttackData.AttackMontage);
}

bool AEnemyCharacter::IsDead() const
{
	const UHealthComponent* HealthComp = GetHealthComponent();
	if (!HealthComp)
	{
		return false;
	}

	return HealthComp->IsDead();
}