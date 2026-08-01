// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarWidget.generated.h"

class UInventoryComponent;

/**
 * Always-on-screen quick bar, showing what each number key is bound to and how much of it you hold.
 *
 * Separate widget from WBP_Inventory rather than a shared one: this is HUD that lives for the whole
 * session, while the inventory panel is opened and closed. Both read the same
 * UInventoryComponent hotbar assignments, so they can never disagree.
 *
 * Widgets are found by name (Slot0Bg, Slot0Name, ...), matching UInventoryWidget, so the layout
 * stays editable in the designer without touching C++.
 */
UCLASS()
class ACCIDENTALHERO_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Repopulates every slot from the player's current bindings. */
	UFUNCTION(BlueprintCallable, Category = "Hotbar UI")
	void RefreshHotbar();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Only does work until the inventory is found and subscribed to; after that the bar is purely
	 *  event-driven and this costs a float compare per frame. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleInventoryChanged();

	UPROPERTY(EditDefaultsOnly, Category = "Hotbar UI", meta = (ClampMin = "1"))
	int32 SlotCount = 8;

private:
	UInventoryComponent* GetInventory() const;

	/** Cached so the delegate can be unbound even if the component is gone by then. Also doubles as
	 *  the "am I subscribed yet" flag for the startup retry. */
	UPROPERTY()
	TObjectPtr<UInventoryComponent> BoundInventory;

	float TimeSinceBindRetry = 0.0f;
};
