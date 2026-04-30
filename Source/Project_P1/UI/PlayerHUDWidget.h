#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Project_P1/Components/StyleComponent.h"
#include "PlayerHUDWidget.generated.h"

UCLASS()
class PROJECT_P1_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void SetHealthPercent(float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void SetStyleDisplay(float NewStyle, EStyleRank NewRank, float NormalizedStyle);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UStyleComponent* BoundStyleComponent = nullptr;

	UFUNCTION()
	void HandleStyleChanged(float NewStyle, EStyleRank NewRank, float NormalizedStyle);
};