#include "EnemyCharacter.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Project_P1/Components/HealthComponent.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = 450.f;
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
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	// Carry forward temporarily overrides normal chase/attack logic.
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

	if (!IsTargetValid())
	{
		TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		return;
	}

	const float DistanceToTarget = DistanceToTarget2D();

	if (DistanceToTarget <= AttackRange)
	{
		TryAttackTarget();
	}

	if (bDebugEnemy)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Enemy] Dist=%.1f Chase=%d AttackCooldown=%d"),
			DistanceToTarget,
			DistanceToTarget <= ChaseRange ? 1 : 0,
			bAttackOnCooldown ? 1 : 0
		);
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

void AEnemyCharacter::TryAttackTarget()
{
	if (bIsDead || bIsCarryForwardActive)
	{
		return;
	}

	if (bAttackOnCooldown)
	{
		return;
	}

	if (!HasLineOfSightToTarget())
	{
		return;
	}

	bAttackOnCooldown = true;

	GetWorldTimerManager().SetTimer(
		AttackCooldownHandle,
		this,
		&AEnemyCharacter::ResetAttackCooldown,
		AttackCooldown,
		false
	);

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
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack triggered"));
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

	SetActorTickEnabled(false);
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
			// Standard push away from attacker
			MoveComp->AddImpulse(PendingHitReactionDirection * PendingKnockbackStrength, true);
			break;
		}

		case EHitReactionType::Launch:
		{
			// Launch character upward (used for uppercuts / air combat)
			const FVector LaunchVelocity =
				PendingHitReactionDirection * PendingKnockbackStrength +
				FVector(0.f, 0.f, PendingLaunchStrength);

			LaunchCharacter(LaunchVelocity, true, true);
			break;
		}

		case EHitReactionType::CarryForward:
		{
			// Carry the target forward for a short controlled duration.
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

	// Consume cached data after the reaction was used once.
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
	// Disable tick so no AI / logic runs.
	SetActorTickEnabled(false);

	// Stop movement and disable it.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Deactivated: %s"), *GetName());
}

void AEnemyCharacter::ActivateEnemy()
{
	// Enable tick again.
	SetActorTickEnabled(true);

	// Restore movement.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Activated: %s"), *GetName());
}