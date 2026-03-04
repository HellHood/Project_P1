#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Components/CombatComponent.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
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

	// === Capsule (optional tuning) ===
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// === SpringArm ===
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 350.f;        // distance behind the character
	SpringArm->bUsePawnControlRotation = true; // rotate arm based on controller (mouse)
	SpringArm->bEnableCameraLag = false;       // keep it tight for now
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw   = true;
	SpringArm->bInheritRoll  = false;
	
	// === Camera ===
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;  // camera does not rotate separately; arm does

	// === Rotation behavior (TPP action feel) ===
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;           // face movement direction
	MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);   // fast turn rate

	// Basic movement tuning (you can tweak later)
	MoveComp->MaxWalkSpeed = 800.f;
	MoveComp->BrakingDecelerationWalking = 4096.f;
	MoveComp->AirControl = 0.25f;
	MoveComp->JumpZVelocity = 720.f;
	MoveComp->GravityScale = 1.7f;
	JumpMaxCount = 2;
	JumpMaxHoldTime = 0.50f;
	
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UHealthComponent* HC = GetHealthComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] Before: %.1f / %.1f"), HC->GetHealth(), HC->GetMaxHealth());

		HC->ApplyDamage(25.f, this);

		UE_LOG(LogTemp, Warning, TEXT("[HealthTest] After:  %.1f / %.1f"), HC->GetHealth(), HC->GetMaxHealth());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[HealthTest] No HealthComponent on %s"), *GetName());
	}
	// Add mapping context for the local player
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
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// We need EnhancedInputComponent to bind Input Actions
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		else
			UE_LOG(LogTemp, Warning, TEXT("MoveAction is NOT set on %s"), *GetName());

		if (LookAction)
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		else
			UE_LOG(LogTemp, Warning, TEXT("LookAction is NOT set on %s"), *GetName());
		if (DashAction)
			EIC->BindAction(DashAction, ETriggerEvent::Started, this, &APlayerCharacter::StartDash);
		else
			UE_LOG(LogTemp, Warning, TEXT("DashAction is NOT set on %s"), *GetName());
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::OnJumpPressed);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnJumpReleased);
		}
			else
			UE_LOG(LogTemp, Warning, TEXT("JumpAction is NOT set on %s"), *GetName());
		if (LightAttackAction)
		{
			EIC->BindAction(LightAttackAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLightAttack);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LightAttackAction is NOT set on %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent missing on %s. Did you enable Enhanced Input plugin?"), *GetName());
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (bIsDashing) return;
	
	const FVector2D V = Value.Get<FVector2D>();
	if (!Controller) return;

	// Camera-relative movement: take controller yaw only
	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, V.Y);
	AddMovementInput(Right, V.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	// UE_LOG(LogTemp, Warning, TEXT("Look V: X=%f Y=%f"), V.X, V.Y);

	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
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
		const UCharacterMovementComponent* MC = GetCharacterMovement();
		const bool bFalling = MC ? MC->IsFalling() : false;
		const int32 Mode = MC ? (int32)MC->MovementMode : -1;
		
		UE_LOG(LogTemp, Warning, TEXT("[MoveDebug] Mode=%d Falling=%d Dashing=%d"), Mode, bFalling ? 1: 0, bIsDashing ? 1: 0);
	}
}

bool APlayerCharacter::CanDash() const
{
	if (bIsDashing) return false;
	if (bDashOnCooldown) return false;
	
	return true;
}

void APlayerCharacter::StartDash()
{
	if (!CanDash())
	{
		return;
	}
	
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (!MoveComp) return;
	
	DashPrevMovementMode = MoveComp->MovementMode;
	DashPrevCustomMode = MoveComp->CustomMovementMode;

	// Remember jumps before dash
	DashSavedJumpCount = JumpCurrentCount;
	bDashSavedJumpCountValid = true;
	
	bIsDashing = true;
	bDashOnCooldown = true;
	
	// Lock direction
	DashDirection = GetDashDirection();
	
	// Lockout standard movement
	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
	
	// End
	GetWorldTimerManager().SetTimer(DashDurationHandle, this, &APlayerCharacter::StopDash, DashDuration, false);
	
	// Cooldown
	GetWorldTimerManager().SetTimer(DashCooldownHandle, this, &APlayerCharacter::ResetDashCooldown, DashCooldown, false);
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
	if (!bIsDashing) return;

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
	// Prefer last input vector
	if (const UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const FVector Input = MoveComp->GetLastInputVector();
		const FVector Flat(Input.X, Input.Y, 0.0f);
		
		if (!Flat.IsNearlyZero())
		{
			return Flat.GetSafeNormal();
		}
	}
	
	// Fallback: forward
	const FVector Forward = GetActorForwardVector();
	return FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();
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
	if (UCombatComponent* CC = GetCombatComponent())
	{
		CC->TryLightAttack();
	}
}