#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// === Capsule (optional tuning) ===
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// === SpringArm ===
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 350.f;        // distance behind the character
	SpringArm->bUsePawnControlRotation = true; // rotate arm based on controller (mouse)
	SpringArm->bEnableCameraLag = false;       // keep it tight for now

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
	MoveComp->AirControl = 0.45f;
	
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent missing on %s. Did you enable Enhanced Input plugin?"), *GetName());
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
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

	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
}