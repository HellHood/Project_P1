#include "BaseCharacter.h"

#include "../Components/CombatComponent.h"
#include "../Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);

		MoveComp->MaxWalkSpeed = 650.f;
		MoveComp->MaxAcceleration = 4096.f;
		MoveComp->BrakingDecelerationWalking = 4096.f;
		MoveComp->GroundFriction = 8.f;
		MoveComp->BrakingFrictionFactor = 2.f;

		MoveComp->AirControl = 0.45f;
		MoveComp->GravityScale = 1.2f;
		MoveComp->JumpZVelocity = 650.f;
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}