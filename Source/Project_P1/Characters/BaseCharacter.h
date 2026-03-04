// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UHealthComponent;
class UCombatComponent;

UCLASS()
class PROJECT_P1_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();
	
	UFUNCTION(BlueprintPure, Category="Systems|Health")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	
	UFUNCTION(BlueprintPure, Category="Systems|Combat")
	UCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Systems")
	UHealthComponent* HealthComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="System")
	UCombatComponent* CombatComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
