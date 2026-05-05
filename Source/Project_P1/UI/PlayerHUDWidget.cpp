#include "PlayerHUDWidget.h"

#include "GameFramework/Pawn.h"

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] Style bind failed: owning pawn is null"));
		return;
	}

	BoundStyleComponent = OwningPawn->FindComponentByClass<UStyleComponent>();
	if (!BoundStyleComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] Style bind failed: StyleComponent not found"));
		return;
	}

	BoundStyleComponent->OnStyleChanged.AddDynamic(this, &UPlayerHUDWidget::HandleStyleChanged);

	SetStyleDisplay(
		BoundStyleComponent->GetStyle(),
		UStyleComponent::GetRankAsText(BoundStyleComponent->GetCurrentRank()),
		BoundStyleComponent->GetNormalizedStyle()
	);

	UE_LOG(LogTemp, Warning, TEXT("[HUD] Style bound"));
}

void UPlayerHUDWidget::HandleStyleChanged(float NewStyle, EStyleRank NewRank, float NormalizedStyle)
{
	SetStyleDisplay(NewStyle, UStyleComponent::GetRankAsText(NewRank), NormalizedStyle);
}

void UPlayerHUDWidget::NativeDestruct()
{
	if (BoundStyleComponent)
	{
		BoundStyleComponent->OnStyleChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleStyleChanged);
		BoundStyleComponent = nullptr;
	}

	Super::NativeDestruct();
}

