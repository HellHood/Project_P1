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
	
	if (UHealthComponent* HealthComp = GetHealthComponent())
	{
		HealthComp->OnDeath.AddDynamic(this, &AEnemyCharacter::HandleEnemyDeath);
		HealthComp->OnHealthChanged.AddDynamic(this, &AEnemyCharacter::HandleEnemyHealthChanged);
	}

	if (UCombatComponent* CombatComp = GetCombatComponent())
	{		
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

bool AEnemyCharacter::TryAttackFromAI(float& OutAttackDuration)
{
	OutAttackDuration = 0.f;

	if (bIsDead || bIsCarryForwardActive)
	{
		return false;
	}

	if (!IsTargetInAttackRange())
	{
		return false;
	}

	UCombatComponent* CombatComp = GetCombatComponent();
	if (!CombatComp)
	{
		return false;
	}

	const FName ChosenAttackId = ChooseAttackId();
	if (ChosenAttackId.IsNone())
	{
		return false;
	}

	FAttackData AttackData;
	if (!CombatComp->GetAttackDataById(ChosenAttackId, AttackData))
	{
		return false;
	}

	const bool bAttackStarted = CombatComp->RequestAttackById(ChosenAttackId);
	if (!bAttackStarted)
	{
		return false;
	}

	OutAttackDuration = AttackData.Duration;

	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Attack triggered: %s, Duration: %.2f"), *ChosenAttackId.ToString(), OutAttackDuration);
	}

	return true;
}

void AEnemyCharacter::TryAttackTarget()
{
	TryAttackFromAI();
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
	bHasPendingHitReaction = false;

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

bool AEnemyCharacter::IsTargetInAttackRange() const
{
	if (!IsTargetValid())
	{
		if (bDebugEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy] Range check failed: invalid target"));
		}

		return false;
	}

	if (!HasLineOfSightToTarget())
	{
		if (bDebugEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy] Range check failed: no line of sight"));
		}

		return false;
	}

	const float TargetDistance = DistanceToTarget2D();

	for (const FEnemyAttackOption& AttackOption : AttackOptions)
	{
		if (AttackOption.AttackId.IsNone())
		{
			if (bDebugEnemy)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Enemy] Range option skipped: AttackId is None"));
			}

			continue;
		}

		if (IsAttackOptionInRange(AttackOption, TargetDistance))
		{
			if (bDebugEnemy)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Enemy] In range: Distance %.1f, Attack %s, Range %.1f-%.1f"),
					TargetDistance,
					*AttackOption.AttackId.ToString(),
					AttackOption.MinRange,
					AttackOption.MaxRange
				);
			}

			return true;
		}
	}

	if (bDebugEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] Range check failed: Distance %.1f, Options %d"), TargetDistance, AttackOptions.Num());
	}

	return false;
}

bool AEnemyCharacter::CanAttack() const
{
	if (bIsDead || bIsCarryForwardActive)
	{
		return false;
	}

	const UCombatComponent* CombatComp = GetCombatComponent();
	if (!CombatComp)
	{
		return false;
	}

	const FName ChosenAttackId = ChooseAttackId();
	if (ChosenAttackId.IsNone())
	{
		return false;
	}

	const bool bCanStartAttack = CombatComp->CanStartAttackById(ChosenAttackId);

	if (bDebugEnemy)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Enemy] CanAttack candidate: %s, CanStart: %s"),
			*ChosenAttackId.ToString(),
			bCanStartAttack ? TEXT("true") : TEXT("false")
		);
	}

	return bCanStartAttack;
}

bool AEnemyCharacter::IsAttackOptionInRange(const FEnemyAttackOption& AttackOption,float TargetDistance) const
{
	return TargetDistance >= AttackOption.MinRange && TargetDistance <= AttackOption.MaxRange;
}

float AEnemyCharacter::ScoreAttackOption(const FEnemyAttackOption& AttackOption, float TargetDistance) const
{
	const float IdealRange = (AttackOption.MinRange + AttackOption.MaxRange) * 0.5f;
	return FMath::Abs(TargetDistance - IdealRange);
}

FName AEnemyCharacter::ChooseAttackId() const
{
	const UCombatComponent* CombatComp = GetCombatComponent();
	if (!CombatComp)
	{
		return NAME_None;
	}

	if (!IsTargetValid())
	{
		return NAME_None;
	}

	const float TargetDistance = DistanceToTarget2D();

	TArray<TPair<FName, float>> ScoredAttacks;

	for (const FEnemyAttackOption& AttackOption : AttackOptions)
	{
		if (AttackOption.AttackId.IsNone())
		{
			continue;
		}

		if (!IsAttackOptionInRange(AttackOption, TargetDistance))
		{
			continue;
		}

		if (!CombatComp->HasAttackById(AttackOption.AttackId))
		{
			continue;
		}

		const float Score = ScoreAttackOption(AttackOption, TargetDistance);

		ScoredAttacks.Add({ AttackOption.AttackId, Score });

		if (bDebugEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Enemy] Score: %s -> %.1f"), *AttackOption.AttackId.ToString(), Score);
		}
	}

	ScoredAttacks.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
	{
		return A.Value < B.Value; // mniejszy score = lepszy
	});
	
	const int32 TopCount = FMath::Min(2, ScoredAttacks.Num()); // top 2
	if (TopCount == 0)
	{
		return NAME_None;
	}

	const int32 RandomIndex = FMath::RandRange(0, TopCount - 1);

	const FName ChosenAttack = ScoredAttacks[RandomIndex].Key;
	const float ChosenScore = ScoredAttacks[RandomIndex].Value;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Enemy] Chosen (soft): %s, Score: %.1f"),
		*ChosenAttack.ToString(),
		ChosenScore
	);

	return ChosenAttack;
}

bool AEnemyCharacter::TryAttackFromAI()
{
	float IgnoredDuration = 0.f;
	return TryAttackFromAI(IgnoredDuration);
}