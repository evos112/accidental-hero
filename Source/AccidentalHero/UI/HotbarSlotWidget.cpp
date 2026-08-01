// Copyright Epic Games, Inc. All Rights Reserved.

#include "HotbarSlotWidget.h"
#include "ItemDragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Items/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Input/Reply.h"

UInventoryComponent* UHotbarSlotWidget::GetInventory() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	return PS ? PS->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UHotbarSlotWidget::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	// The printed number is derived from the index rather than authored per instance, so the eight
	// slots stay identical copies of one widget.
	if (WidgetTree)
	{
		if (UTextBlock* Key = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Key"))))
		{
			Key->SetText(SlotIndex >= 0 ? FText::AsNumber(SlotIndex + 1) : FText::GetEmpty());
		}
	}
}

void UHotbarSlotWidget::SetSlotContent(UItemDefinition* InItem, int32 HeldCount)
{
	Item = InItem;

	if (!WidgetTree)
	{
		return;
	}

	if (UTextBlock* Name = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Name"))))
	{
		Name->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	}

	if (UTextBlock* Qty = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Qty"))))
	{
		// A count only means anything on a stack of more than one.
		Qty->SetText(HeldCount > 1 ? FText::AsNumber(HeldCount) : FText::GetEmpty());
	}

	if (UImage* Bg = Cast<UImage>(WidgetTree->FindWidget(TEXT("Bg"))))
	{
		// Bound and stocked is brightest; bound but empty dims; unbound dims further.
		Bg->SetOpacity(Item ? (HeldCount > 0 ? 1.0f : 0.5f) : 0.28f);
	}

	// Wear bar, shown only for tools that actually wear. The value is the tool that would be used
	// next (lowest durability first), so the bar matches what a keypress would swing.
	UImage* DurFill = Cast<UImage>(WidgetTree->FindWidget(TEXT("DurFill")));
	UImage* DurTrack = Cast<UImage>(WidgetTree->FindWidget(TEXT("DurTrack")));
	if (!DurFill || !DurTrack)
	{
		return;
	}

	const UInventoryComponent* Inventory = GetInventory();
	const int32 Durability = (Item && Inventory) ? Inventory->GetItemDurability(Item) : -1;
	const bool bShow = Durability >= 0 && Item && Item->MaxDurability > 0;

	DurFill->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	DurTrack->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (bShow)
	{
		const float Ratio = FMath::Clamp(
			static_cast<float>(Durability) / static_cast<float>(Item->MaxDurability), 0.0f, 1.0f);

		if (UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(DurFill->Slot))
		{
			if (const UCanvasPanelSlot* TrackSlot = Cast<UCanvasPanelSlot>(DurTrack->Slot))
			{
				const FVector2D Full = TrackSlot->GetSize();
				FillSlot->SetSize(FVector2D(Full.X * Ratio, Full.Y));
			}
		}

		// Red under 20%, matching the warning threshold in SpendToolDurability.
		DurFill->SetColorAndOpacity(Ratio <= 0.2f
			? FLinearColor(0.95f, 0.35f, 0.30f, 1.0f)
			: FLinearColor(0.55f, 0.85f, 0.50f, 1.0f));
	}
}

FReply UHotbarSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// An empty slot has nothing to pick up, so let the click fall through.
	if (!Item || !InMouseEvent.GetEffectingButton().IsMouseButton())
	{
		return FReply::Unhandled();
	}

	// Returning DetectDrag rather than starting immediately means a plain click still behaves like
	// a click; the drag only begins once the cursor actually moves.
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton)
		.NativeReply;
}

void UHotbarSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!Item)
	{
		return;
	}

	UItemDragDropOperation* Operation = NewObject<UItemDragDropOperation>(GetTransientPackage(),
		UItemDragDropOperation::StaticClass());
	Operation->Item = Item;
	Operation->SourceHotbarIndex = SlotIndex;
	Operation->Pivot = EDragPivot::CenterCenter;

	if (DragVisualClass)
	{
		if (UUserWidget* Visual = CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass))
		{
			if (UHotbarSlotWidget* AsSlot = Cast<UHotbarSlotWidget>(Visual))
			{
				AsSlot->SetSlotContent(Item, 1);
			}
			Operation->DefaultDragVisual = Visual;
		}
	}

	OutOperation = Operation;
}

void UHotbarSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (bAcceptsDrops && Cast<UItemDragDropOperation>(InOperation))
	{
		SetHighlighted(true);
	}
}

void UHotbarSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	SetHighlighted(false);
}

void UHotbarSlotWidget::SetHighlighted(bool bHighlighted)
{
	if (!WidgetTree)
	{
		return;
	}

	// The accent strip doubles as the drop indicator, so there's no extra widget to maintain.
	if (UImage* Edge = Cast<UImage>(WidgetTree->FindWidget(TEXT("Edge"))))
	{
		Edge->SetColorAndOpacity(bHighlighted
			? FLinearColor(0.45f, 1.0f, 0.55f, 1.0f)
			: FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

bool UHotbarSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	SetHighlighted(false);

	UItemDragDropOperation* Operation = Cast<UItemDragDropOperation>(InOperation);
	UInventoryComponent* Inventory = GetInventory();
	if (!bAcceptsDrops || !Operation || !Operation->Item || !Inventory || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	// Dropping a key onto itself is a no-op rather than a swap with nothing.
	if (Operation->SourceHotbarIndex == SlotIndex)
	{
		return true;
	}

	if (Operation->SourceHotbarIndex != INDEX_NONE)
	{
		// Key-to-key: swap, so dragging onto an occupied key displaces rather than discards.
		// Read what's here first — AssignHotbarSlot de-duplicates and would clear the source.
		UItemDefinition* Displaced = Inventory->GetHotbarItem(SlotIndex);
		Inventory->AssignHotbarSlot(SlotIndex, Operation->Item);
		Inventory->AssignHotbarSlot(Operation->SourceHotbarIndex, Displaced);
	}
	else
	{
		// From the inventory grid: plain assign. AssignHotbarSlot vacates any other key holding it.
		Inventory->AssignHotbarSlot(SlotIndex, Operation->Item);
	}

	return true;
}
