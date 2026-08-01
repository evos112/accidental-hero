// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarSlotWidget.generated.h"

class UItemDefinition;
class UInventoryComponent;

/**
 * One quick-bar key: shows what's bound to it, and is both a drag source and a drop target.
 *
 * Slots are their own widget rather than loose Images on a canvas because drag-and-drop needs a
 * widget that can take mouse input and own a hit-test area. The same class backs the inventory
 * grid's slots, where DropsAllowed is turned off so the grid can only be dragged *from*.
 */
UCLASS()
class ACCIDENTALHERO_API UHotbarSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which quick-bar key this represents, which also sets the printed key number. Inventory-grid
	 *  slots pass INDEX_NONE, which blanks the number. */
	UFUNCTION(BlueprintCallable, Category = "Hotbar Slot")
	void SetSlotIndex(int32 InSlotIndex);

	UFUNCTION(BlueprintPure, Category = "Hotbar Slot")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Pushes the item and count into this slot's child widgets. */
	UFUNCTION(BlueprintCallable, Category = "Hotbar Slot")
	void SetSlotContent(UItemDefinition* InItem, int32 HeldCount);

	UFUNCTION(BlueprintPure, Category = "Hotbar Slot")
	UItemDefinition* GetSlotItem() const { return Item; }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** False for inventory-grid slots, which are drag sources only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar Slot")
	bool bAcceptsDrops = true;

	/** Ghost shown under the cursor while dragging. Unset simply means no ghost. */
	UPROPERTY(EditDefaultsOnly, Category = "Hotbar Slot")
	TSubclassOf<UUserWidget> DragVisualClass;

private:
	UInventoryComponent* GetInventory() const;

	/** Restores the slot's normal tint after a drag passes over it. */
	void SetHighlighted(bool bHighlighted);

	UPROPERTY()
	TObjectPtr<UItemDefinition> Item;

	int32 SlotIndex = INDEX_NONE;
};
