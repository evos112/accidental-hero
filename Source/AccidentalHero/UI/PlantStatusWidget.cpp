// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlantStatusWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetComponent.h"
#include "Items/CropPlant.h"

ACropPlant* UPlantStatusWidget::GetCrop() const
{
	// The widget component owns this widget, and the crop owns the component.
	const UWidgetComponent* Component = Cast<UWidgetComponent>(GetOuter());
	return Component ? Cast<ACropPlant>(Component->GetOwner()) : Cast<ACropPlant>(GetOwningPlayerPawn());
}

void UPlantStatusWidget::SetBar(const FString& Prefix, float Fraction)
{
	if (!WidgetTree)
	{
		return;
	}

	UImage* Fill = Cast<UImage>(WidgetTree->FindWidget(FName(*(Prefix + TEXT("Fill")))));
	UImage* Track = Cast<UImage>(WidgetTree->FindWidget(FName(*(Prefix + TEXT("Track")))));
	if (!Fill || !Track)
	{
		return;
	}

	const UCanvasPanelSlot* TrackSlot = Cast<UCanvasPanelSlot>(Track->Slot);
	UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(Fill->Slot);
	if (TrackSlot && FillSlot)
	{
		const FVector2D Full = TrackSlot->GetSize();
		FillSlot->SetSize(FVector2D(Full.X * FMath::Clamp(Fraction, 0.0f, 1.0f), Full.Y));
	}
}

void UPlantStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceRefresh += InDeltaTime;
	if (TimeSinceRefresh < RefreshInterval)
	{
		return;
	}
	TimeSinceRefresh = 0.0f;

	const ACropPlant* Crop = GetCrop();
	if (!Crop || !WidgetTree)
	{
		return;
	}

	SetBar(TEXT("Growth"), Crop->GetGrowthFraction());
	SetBar(TEXT("Water"), Crop->GetWaterFraction());

	if (UTextBlock* Name = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CropName"))))
	{
		Name->SetText(Crop->GetCropName());
	}

	if (UTextBlock* Status = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StatusText"))))
	{
		// One line that tells the player what to do next, in priority order.
		FString Text;
		if (Crop->IsWithered())
		{
			Text = TEXT("Withered");
		}
		else if (Crop->GetWaterFraction() <= 0.0f)
		{
			Text = FString::Printf(TEXT("Dry - needs water  (%.0f%% health)"), Crop->GetVitalityFraction() * 100.0f);
		}
		else if (Crop->IsRipe())
		{
			Text = TEXT("Ready to harvest");
		}
		else
		{
			Text = FString::Printf(TEXT("Growing  %.0f%%"), Crop->GetGrowthFraction() * 100.0f);
		}
		Status->SetText(FText::FromString(Text));
	}
}
