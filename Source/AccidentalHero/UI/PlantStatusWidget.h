// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlantStatusWidget.generated.h"

class ACropPlant;

/**
 * Floating growth/water readout drawn above a crop by its UWidgetComponent.
 *
 * Reads the owning ACropPlant directly rather than being pushed values: the crop's state is
 * replicated, so both bars stay correct on clients with no extra plumbing. Widgets are found by
 * name, matching UInventoryWidget, so the layout stays editable in the designer.
 */
UCLASS()
class ACCIDENTALHERO_API UPlantStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Seconds between refreshes. Bars move slowly; per-frame updates would be waste across a field. */
	UPROPERTY(EditDefaultsOnly, Category = "Plant Status", meta = (ClampMin = "0.05"))
	float RefreshInterval = 0.2f;

private:
	/** The crop this widget is parented to, via the owning UWidgetComponent's actor. */
	ACropPlant* GetCrop() const;

	/** Scales a fill image's canvas width to Fraction of its track's width. */
	void SetBar(const FString& Prefix, float Fraction);

	float TimeSinceRefresh = 0.0f;
};
