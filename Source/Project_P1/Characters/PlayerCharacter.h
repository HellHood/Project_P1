#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "../Components/TargetingComponent.h"
#include "../Components/CombatComponent.h"
#include "PlayerCharacter.generated.h"

class UTargetingComponent;
class UCameraComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
class UUserWidget;
class UStyleComponent;
class UWeaponDataAsset;
class UWeaponMasteryComponent;

UCLASS()
class PROJECT_P1_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	
	UFUNCTION(BlueprintPure, Category="Components")
	UStyleComponent* GetStyleComponent() const { return StyleComponent; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UWeaponMasteryComponent* WeaponMasteryComponent;
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EquipWeaponByIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void NextWeapon();
	
	UFUNCTION(Exec)
	void AddWeaponXP(float XPAmount);
	
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UStyleComponent* StyleComponent = nullptr;
	
	
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* NextWeaponAction;
	
	// Temporary visual weapon mesh attached to the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	UStaticMeshComponent* WeaponMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName WeaponSocketName = TEXT("weapon_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	UPROPERTY()
	UUserWidget* PlayerHUDWidget = nullptr;
	
	

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TArray<UWeaponDataAsset*> AvailableWeapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	int32 CurrentWeaponIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	UWeaponDataAsset* CurrentWeaponData = nullptr;

	UFUNCTION()
	void HandleAttackStarted(FAttackData AttackData);

	UFUNCTION()
	void HandlePlayerHealthChanged(UHealthComponent* HealthComp, float NewHealth, float Delta, AActor* InstigatorActor);
	
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
	void UpdatePlayerHUD();
	void OnNextWeaponPressed();

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
