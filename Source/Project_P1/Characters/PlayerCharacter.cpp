#include "PlayerCharacter.h"

#include "../Components/CombatComponent.h"
#include "../Components/TargetingComponent.h"
#include "../Core/BaseGameMode.h"
#include "EnemyCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Project_P1/Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"

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

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// // Temporary weapon mesh for early gameplay testing.
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh());
	WeaponMesh->SetMobility(EComponentMobility::Movable);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

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
	if (GetMesh())
	{
		const bool bHasSocket = GetMesh()->DoesSocketExist(TEXT("weapon_socket"));
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Does socket exist: %s"), bHasSocket ? TEXT("YES") : TEXT("NO"));
	}
	if (GetMesh())
	{
		const FTransform SocketTransform = GetMesh()->GetSocketTransform(TEXT("weapon_socket"), RTS_World);
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Socket world location: %s"), *SocketTransform.GetLocation().ToString());
	}

	
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
	
	if (WeaponMesh && GetMesh())
	{
		const bool bAttached = WeaponMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			WeaponSocketName
		);

		WeaponMesh->SetRelativeLocation(FVector::ZeroVector);
		WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
		WeaponMesh->SetRelativeScale3D(FVector::OneVector);

		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Attach result: %s"), bAttached ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Attach Parent: %s"),
			WeaponMesh->GetAttachParent() ? *WeaponMesh->GetAttachParent()->GetName() : TEXT("None"));
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Attach Socket: %s"),
			*WeaponMesh->GetAttachSocketName().ToString());
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

	if (HeavyAttackAction)
	{
		EnhancedInput->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &APlayerCharacter::StartHeavyAttack);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HeavyAttackAction is NOT set on %s"), *GetName());
	}

	if (LockOnAction)
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &APlayerCharacter::OnLockOnPressed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LockOnAction is NOT set on %s"), *GetName());
	}

	if (SwitchTargetLeftAction)
	{
		EnhancedInput->BindAction(SwitchTargetLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::OnSwitchTargetLeftPressed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchTargetLeftAction is NOT set on %s"), *GetName());
	}

	if (SwitchTargetRightAction)
	{
		EnhancedInput->BindAction(SwitchTargetRightAction, ETriggerEvent::Started, this, &APlayerCharacter::OnSwitchTargetRightPressed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchTargetRightAction is NOT set on %s"), *GetName());
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

	const bool bIsLockedOn =
		TargetingComponent &&
		TargetingComponent->IsLockedOn();

	// During lock-on, camera yaw is controlled by target tracking.
	// Keep manual pitch so the player can still adjust vertical view slightly.
	if (!bIsLockedOn)
	{
		AddControllerYawInput(InputValue.X);
	}

	AddControllerPitchInput(InputValue.Y);
}

void APlayerCharacter::OnJumpPressed()
{
	if (bIsDashing)
	{
		StopDash();
	}

	Jump();
	bJumpStartTriggered = true;
}

void APlayerCharacter::OnJumpReleased()
{
	StopJumping();
}

void APlayerCharacter::StartLightAttack()
{
	if (UCombatComponent* CombatComp = GetCombatComponent())
	{
		CombatComp->RequestAttack(EAttackInputType::Light);
	}
}

void APlayerCharacter::StartHeavyAttack()
{
	if (UCombatComponent* CombatComp = GetCombatComponent())
	{
		CombatComp->RequestAttack(EAttackInputType::Heavy);
	}
}

void APlayerCharacter::OnLockOnPressed()
{
	if (!TargetingComponent)
	{
		return;
	}

	TargetingComponent->ToggleLockOn();
}

void APlayerCharacter::OnSwitchTargetLeftPressed()
{
	if (!TargetingComponent)
	{
		return;
	}

	TargetingComponent->SwitchTargetLeft();
}

void APlayerCharacter::OnSwitchTargetRightPressed()
{
	if (!TargetingComponent)
	{
		return;
	}

	TargetingComponent->SwitchTargetRight();
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDashing)
	{
		HandleDash(DeltaSeconds);
	}

	UpdateMovementRotationMode();

	if (TargetingComponent && TargetingComponent->IsLockedOn())
	{
		UpdateLockOnRotation(DeltaSeconds);
		UpdateLockOnCamera(DeltaSeconds);
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

void APlayerCharacter::UpdateLockOnRotation(float DeltaSeconds)
{
	if (!TargetingComponent)
	{
		return;
	}

	AEnemyCharacter* CurrentTarget = TargetingComponent->GetCurrentTarget();
	if (!CurrentTarget)
	{
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);

	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = FlatDirection.Rotation();

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		LockOnRotationSpeed
	);

	SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
}

void APlayerCharacter::UpdateLockOnCamera(float DeltaSeconds)
{
	if (!TargetingComponent || !Controller)
	{
		return;
	}

	AEnemyCharacter* CurrentTarget = TargetingComponent->GetCurrentTarget();
	if (!CurrentTarget)
	{
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);

	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentControlRotation = Controller->GetControlRotation();
	const FRotator DesiredYawRotation = FlatDirection.Rotation();
	const FRotator TargetControlRotation(
		CurrentControlRotation.Pitch,
		DesiredYawRotation.Yaw,
		CurrentControlRotation.Roll
	);

	const FRotator NewControlRotation = FMath::RInterpTo(
		CurrentControlRotation,
		TargetControlRotation,
		DeltaSeconds,
		LockOnCameraRotationSpeed
	);

	Controller->SetControlRotation(NewControlRotation);
}

void APlayerCharacter::UpdateMovementRotationMode()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const bool bShouldUseLockOnRotation =
		TargetingComponent &&
		TargetingComponent->IsLockedOn();

	// Free movement rotates the character into movement direction.
	// Lock-on keeps the character facing the current target instead.
	MoveComp->bOrientRotationToMovement = !bShouldUseLockOnRotation;
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

bool APlayerCharacter::ConsumeJumpStartTrigger()
{
	if (!bJumpStartTriggered)
	{
		return false;
	}

	bJumpStartTriggered = false;
	return true;
}