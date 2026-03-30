#include "TargetingComponent.h"

#include "../Characters/EnemyCharacter.h"
#include "../Components/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTargetingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget)
	{
		return;
	}

	// If the current target becomes invalid, try to replace it.
	if (!IsTargetValid(CurrentTarget))
	{
		CurrentTarget = FindBestTarget();

		if (bDebugTargeting)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Targeting] Current target invalid. New target: %s"),
				CurrentTarget ? *CurrentTarget->GetName() : TEXT("None")
			);
		}
	}
}

void UTargetingComponent::ToggleLockOn()
{
	if (CurrentTarget)
	{
		if (bDebugTargeting)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Targeting] Lock released: %s"), *CurrentTarget->GetName());
		}

		ClearTarget();
		return;
	}

	CurrentTarget = FindBestTarget();

	if (bDebugTargeting)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Targeting] Lock target: %s"),
			CurrentTarget ? *CurrentTarget->GetName() : TEXT("None")
		);
	}
}

void UTargetingComponent::ClearTarget()
{
	CurrentTarget = nullptr;
}

void UTargetingComponent::SwitchTargetLeft()
{
	if (!CurrentTarget)
	{
		CurrentTarget = FindBestTarget();
		return;
	}

	AEnemyCharacter* NewTarget = FindSideTarget(true);
	if (!NewTarget)
	{
		return;
	}

	if (bDebugTargeting)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Targeting] Switch left: %s -> %s"),
			*CurrentTarget->GetName(),
			*NewTarget->GetName()
		);
	}

	CurrentTarget = NewTarget;
}

void UTargetingComponent::SwitchTargetRight()
{
	if (!CurrentTarget)
	{
		CurrentTarget = FindBestTarget();
		return;
	}

	AEnemyCharacter* NewTarget = FindSideTarget(false);
	if (!NewTarget)
	{
		return;
	}

	if (bDebugTargeting)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Targeting] Switch right: %s -> %s"),
			*CurrentTarget->GetName(),
			*NewTarget->GetName()
		);
	}

	CurrentTarget = NewTarget;
}

AEnemyCharacter* UTargetingComponent::FindBestTarget() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), FoundActors);

	AEnemyCharacter* BestTarget = nullptr;
	float BestScore = -9999.f;

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector OwnerForward = GetTargetingForward();

	const float MaxTargetingAngleRadians = FMath::DegreesToRadians(MaxTargetingAngleDegrees);
	const float MinDotThreshold = FMath::Cos(MaxTargetingAngleRadians);

	for (AActor* Actor : FoundActors)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
		if (!Enemy)
		{
			continue;
		}

		if (!IsTargetValid(Enemy))
		{
			continue;
		}

		const FVector ToTarget = Enemy->GetActorLocation() - OwnerLocation;
		const float Distance = ToTarget.Size();
		const FVector Direction = ToTarget.GetSafeNormal();

		const float Dot = FVector::DotProduct(OwnerForward, Direction);
		if (Dot < MinDotThreshold)
		{
			continue;
		}

		// Closer targets get a higher score inside the targeting range.
		const float DistanceScore = 1.0f - FMath::Clamp(Distance / TargetingRange, 0.f, 1.f);

		// Forward alignment is more important than distance.
		const float Score = (Dot * 0.7f) + (DistanceScore * 0.3f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

AEnemyCharacter* UTargetingComponent::FindSideTarget(bool bSearchLeft) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!CurrentTarget)
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), FoundActors);

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector OwnerForward = GetTargetingForward();

	AEnemyCharacter* BestTarget = nullptr;
	float BestScore = -9999.f;

	for (AActor* Actor : FoundActors)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
		if (!Enemy || Enemy == CurrentTarget)
		{
			continue;
		}

		if (!IsTargetValid(Enemy))
		{
			continue;
		}

		const FVector ToTarget = Enemy->GetActorLocation() - OwnerLocation;
		const float Distance = ToTarget.Size();
		const FVector Direction = ToTarget.GetSafeNormal();

		const float Dot = FVector::DotProduct(OwnerForward, Direction);
		const FVector Cross = FVector::CrossProduct(OwnerForward, Direction);

		// In UE, positive Z cross means target is on the left side of forward.
		const bool bIsOnRequestedSide = bSearchLeft ? (Cross.Z > 0.f) : (Cross.Z < 0.f);
		if (!bIsOnRequestedSide)
		{
			continue;
		}

		// Prefer targets that are clearly on the requested side,
		// still somewhat in front of the player, and not too far away.
		const float SideScore = FMath::Abs(Cross.Z);
		const float ForwardScore = FMath::Max(Dot, 0.f);
		const float DistanceScore = 1.0f - FMath::Clamp(Distance / TargetingRange, 0.f, 1.f);

		const float Score =
			(SideScore * 0.5f) +
			(ForwardScore * 0.3f) +
			(DistanceScore * 0.2f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

bool UTargetingComponent::IsTargetValid(const AEnemyCharacter* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	const UHealthComponent* HealthComp = Target->GetHealthComponent();
	if (!HealthComp)
	{
		return false;
	}

	if (HealthComp->IsDead())
	{
		return false;
	}

	const float Distance = FVector::Dist2D(OwnerActor->GetActorLocation(), Target->GetActorLocation());
	if (Distance > TargetingRange)
	{
		return false;
	}

	return true;
}

FVector UTargetingComponent::GetTargetingForward() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return FVector::ForwardVector;
	}

	// Use control rotation yaw so targeting follows camera view, not character mesh facing.
	FVector OwnerForward = OwnerActor->GetActorForwardVector();

	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (const AController* Controller = OwnerPawn->GetController())
		{
			const FRotator ControlRotation = Controller->GetControlRotation();
			const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
			OwnerForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		}
	}

	return OwnerForward;
}