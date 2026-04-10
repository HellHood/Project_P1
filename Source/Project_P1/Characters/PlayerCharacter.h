#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "../Components/TargetingComponent.h"
#include "PlayerCharacter.generated.h"

class UTargetingComponent;
class UCameraComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class PROJECT_P1_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* Camera;
	
	// Targeting system component (handles lock-on logic)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Targeting")
	UTargetingComponent* TargetingComponent;
	
	UPROPERTY(EditDefaultsOnly, Category="Targeting")
	float LockOnRotationSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category="Targeting")
	float LockOnCameraRotationSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* LightAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* LockOnAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* SwitchTargetLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* SwitchTargetRightAction;

	// Temporary visual weapon mesh attached to the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	UStaticMeshComponent* WeaponMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName WeaponSocketName = TEXT("weapon_socket");

	UPROPERTY(EditDefaultsOnly, Category="Dash")
	float DashSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category="Dash")
	float DashDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category="Dash")
	float DashCooldown = 0.35f;

	UPROPERTY(VisibleInstanceOnly, Category="Dash")
	bool bIsDashing = false;

	UFUNCTION(BlueprintCallable, Category="Dash")
	void StartDash();

	UFUNCTION(BlueprintCallable, Category="Dash")
	void StopDash();

	UFUNCTION(BlueprintCallable, Category="Dash")
	bool CanDash() const;
	
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool ConsumeJumpStartTrigger();
	
	UFUNCTION()
	void HandlePlayerDeath(UHealthComponent* HealthComp, AActor* InstigatorActor);

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnJumpPressed();
	void OnJumpReleased();
	void StartLightAttack();
	void StartHeavyAttack();
	void OnLockOnPressed();
	void OnSwitchTargetLeftPressed();
	void OnSwitchTargetRightPressed();
	void UpdateLockOnRotation(float DeltaSeconds);
	void UpdateLockOnCamera(float DeltaSeconds);
	void UpdateMovementRotationMode();

private:
	virtual void Tick(float DeltaSeconds) override;

	void HandleDash(float DeltaSeconds);
	FVector GetDashDirection() const;
	void ResetDashCooldown();

	UPROPERTY(VisibleInstanceOnly, Category="Dash")
	bool bDashOnCooldown = false;

	FTimerHandle DashDurationHandle;
	FTimerHandle DashCooldownHandle;

	EMovementMode DashPrevMovementMode = MOVE_Walking;
	uint8 DashPrevCustomMode = 0;

	int32 DashSavedJumpCount = 0;
	bool bDashSavedJumpCountValid = false;

	FVector DashDirection = FVector::ZeroVector;
	
	bool bJumpStartTriggered = false;
};