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