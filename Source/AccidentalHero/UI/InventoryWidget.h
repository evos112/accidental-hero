// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UItemDefinition;
class UImage;
class UTextBlock;
class UAccidentalHeroAttributeSet;

/**
 * Drives WBP_Inventory, which is a hand-laid-out canvas of plain Images and TextBlocks.
 *
 * Widgets are found by naming convention (SlotName00, SlotQty00, ...) rather than BindWidget,
 * because the layout has 30 slots plus 8 hotbar entries and declaring ~100 BindWidget properties
 * would be worse than one FindWidget helper. A missing widget is simply skipped, so the layout can
 * be re-arranged in the designer without breaking the C++.
 *
 * Refreshes are event-driven (UInventoryComponent::OnInventoryChanged) plus a poll for the
 * attribute meters, which have no change delegate wired up here.
 */
UCLASS()
class ACCIDENTALHERO_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Repopulates slots, meters and the footer. Safe to call from Blueprint on open. */
	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void RefreshAll();

	/** Fills the detail panel from an item, or clears it when null. */
	UFUNCTION(BlueprintCallable, Category = "Inventory UI")
	void ShowItemDetail(UItemDefinition* Item);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleInventoryChanged();

	/** Number of slot widgets present in the layout (SlotName00..SlotName29). */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory UI", meta = (ClampMin = "1"))
	int32 SlotCount = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory UI", meta = (ClampMin = "1"))
	int32 HotbarCount = 8;

	/** Weight the carry meter fills against. Display only — nothing enforces a carry limit yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory UI", meta = (ClampMin = "1.0"))
	float CarryCapacity = 150.0f;

	/** Seconds between attribute-meter polls. The meters drift continuously (survival drain), so
	 *  they can't be event-driven off the inventory, but they also don't need per-frame updates. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory UI", meta = (ClampMin = "0.05"))
	float MeterRefreshInterval = 0.25f;

private:
	UInventoryComponent* GetInventory() const;
	const UAccidentalHeroAttributeSet* GetAttributes() const;

	void RefreshSlots();
	void RefreshMeters();

	/** Sets one meter's label text and scales its fill bar to Current/Max. */
	void ApplyMeter(const FString& Prefix, float Current, float Max, bool bShowAsWeight = false);

	template <typename T>
	T* Find(const FString& Name) const;

	/** Cached so the delegate can be unbound in NativeDestruct even if the component is gone. */
	UPROPERTY()
	TObjectPtr<UInventoryComponent> BoundInventory;

	float TimeSinceMeterRefresh = 0.0f;
};
