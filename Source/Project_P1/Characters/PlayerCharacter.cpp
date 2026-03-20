#include "PlayerCharacter.h"

#include "../Components/CombatComponent.h"
#include "../Core/BaseGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Project_P1/Components/HealthComponent.h"

static int32 GMovementDebug = 0;
static FAutoConsoleVariableRef CVarMovementDebug(
	TEXT("Project_P1.MovementDebug"),
	GMovementDebug,
	TEXT("0=off, 1=on. Prints movement state."),
	ECVF_Default
);

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 350.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);

		MoveComp->MaxWalkSpeed = 800.f;
		MoveComp->BrakingDecelerationWalking = 4096.f;
		MoveComp->AirControl = 0.25f;
		MoveComp->JumpZVelocity = 720.f;
		MoveComp->GravityScale = 1.7f;
	}

	JumpMaxCount = 2;
	JumpMaxHoldTime = 0.50f;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is NOT set on %s"), *GetName());
				}
			}
		}
	}

	if (UHealthComponent* HealthComp = GetHealthComponent())
	{
		HealthComp->OnDeath.AddDynamic(this, &APlayerCharacter::HandlePlayerDeath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No HealthComponent on %s"), *GetName());
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent missing on %s. Did you enable Enhanced Input plugin?"), *GetName());
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveAction is NOT set on %s"), *GetName());
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LookAction is NOT set on %s"), *GetName());
	}

	if (DashAction)
	{
		EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &APlayerCharacter::StartDash);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DashAction is NOT set on %s"), *GetName());
	}

	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::OnJumpPressed);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnJumpReleased);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpAction is NOT set on %s"), *GetName());
	}

	if (LightAttackAction)
	{
		EnhancedInput->BindAction(LightAttackAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLightAttack);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LightAttackAction is NOT set on %s"), *GetName());
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (bIsDashing)
	{
		return;
	}

	if (!Controller)
	{
		return;
	}

	const FVector2D InputValue = Value.Get<FVector2D>();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, InputValue.Y);
	AddMovementInput(Right, InputValue.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D InputValue = Value.Get<FVector2D>();

	AddControllerYawInput(InputValue.X);
	AddControllerPitchInput(InputValue.Y);
}

void APlayerCharacter::OnJumpPressed()
{
	if (bIsDashing)
	{
		StopDash();
	}

	Jump();
}

void APlayerCharacter::OnJumpReleased()
{
	StopJumping();
}

void APlayerCharacter::StartLightAttack()
{
	if (UCombatComponent* CombatComp = GetCombatComponent())
	{
		CombatComp->TryLightAttack();
	}
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDashing)
	{
		HandleDash(DeltaSeconds);
	}

	if (GMovementDebug != 0)
	{
		const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		const bool bFalling = MoveComp ? MoveComp->IsFalling() : false;
		const int32 MovementMode = MoveComp ? static_cast<int32>(MoveComp->MovementMode) : -1;

		UE_LOG(LogTemp, Warning, TEXT("[MoveDebug] Mode=%d Falling=%d Dashing=%d"),
			MovementMode,
			bFalling ? 1 : 0,
			bIsDashing ? 1 : 0
		);
	}
}

bool APlayerCharacter::CanDash() const
{
	if (bIsDashing)
	{
		return false;
	}

	if (bDashOnCooldown)
	{
		return false;
	}

	return true;
}

void APlayerCharacter::StartDash()
{
	if (!CanDash())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	DashPrevMovementMode = MoveComp->MovementMode;
	DashPrevCustomMode = MoveComp->CustomMovementMode;

	DashSavedJumpCount = JumpCurrentCount;
	bDashSavedJumpCountValid = true;

	bIsDashing = true;
	bDashOnCooldown = true;

	DashDirection = GetDashDirection();

	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();

	GetWorldTimerManager().SetTimer(
		DashDurationHandle,
		this,
		&APlayerCharacter::StopDash,
		DashDuration,
		false
	);

	GetWorldTimerManager().SetTimer(
		DashCooldownHandle,
		this,
		&APlayerCharacter::ResetDashCooldown,
		DashCooldown,
		false
	);
}

void APlayerCharacter::HandleDash(float DeltaSeconds)
{
	const FVector Delta = DashDirection * DashSpeed * DeltaSeconds;

	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);

	if (Hit.bBlockingHit)
	{
		StopDash();
	}
}

void APlayerCharacter::StopDash()
{
	if (!bIsDashing)
	{
		return;
	}

	bIsDashing = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(DashPrevMovementMode, DashPrevCustomMode);

		if (bDashSavedJumpCountValid && MoveComp->IsFalling())
		{
			JumpCurrentCount = FMath::Clamp(DashSavedJumpCount, 0, JumpMaxCount);
		}
	}

	bDashSavedJumpCountValid = false;

	GetWorldTimerManager().ClearTimer(DashDurationHandle);
	GetWorldTimerManager().ClearTimer(DashCooldownHandle);
}

void APlayerCharacter::ResetDashCooldown()
{
	bDashOnCooldown = false;
	GetWorldTimerManager().ClearTimer(DashCooldownHandle);
}

FVector APlayerCharacter::GetDashDirection() const
{
	if (const UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const FVector InputVector = MoveComp->GetLastInputVector();
		const FVector FlatInput(InputVector.X, InputVector.Y, 0.0f);

		if (!FlatInput.IsNearlyZero())
		{
			return FlatInput.GetSafeNormal();
		}
	}

	const FVector Forward = GetActorForwardVector();
	return FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();
}

void APlayerCharacter::HandlePlayerDeath(UHealthComponent* HealthComp, AActor* InstigatorActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[Player] Died. Instigator: %s"),
		InstigatorActor ? *InstigatorActor->GetName() : TEXT("None"));

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		DisableInput(PlayerController);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	bIsDashing = false;
	GetWorldTimerManager().ClearTimer(DashDurationHandle);
	GetWorldTimerManager().ClearTimer(DashCooldownHandle);

	if (ABaseGameMode* GameMode = GetWorld()->GetAuthGameMode<ABaseGameMode>())
	{
		GameMode->HandlePlayerDeath();
	}
}