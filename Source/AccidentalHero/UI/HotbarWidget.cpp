// Copyright Epic Games, Inc. All Rights Reserved.

#include "HotbarWidget.h"
#include "HotbarSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UInventoryComponent* UHotbarWidget::GetInventory() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	// Inventory (and the hotbar with it) lives on the PlayerState so it survives respawn.
	return PS ? PS->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UInventoryComponent* Inventory = GetInventory())
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddDynamic(this, &UHotbarWidget::HandleInventoryChanged);
	}

	RefreshHotbar();
}

void UHotbarWidget::NativeDestruct()
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UHotbarWidget::HandleInventoryChanged);
		BoundInventory = nullptr;
	}

	Super::NativeDestruct();
}

void UHotbarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Once subscribed, updates come from the delegate and there is nothing to do here.
	if (BoundInventory)
	{
		return;
	}

	TimeSinceBindRetry += InDeltaTime;
	if (TimeSinceBindRetry >= 0.25f)
	{
		TimeSinceBindRetry = 0.0f;
		RefreshHotbar();
	}
}

void UHotbarWidget::HandleInventoryChanged()
{
	RefreshHotbar();
}

void UHotbarWidget::RefreshHotbar()
{
	if (!WidgetTree)
	{
		return;
	}

	// The PlayerState can arrive after the pawn, so NativeConstruct may have found no inventory to
	// subscribe to. Bind on the first refresh that does find one, otherwise the bar would draw
	// once and then never update again.
	UInventoryComponent* Inventory = GetInventory();
	if (Inventory && !BoundInventory)
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddDynamic(this, &UHotbarWidget::HandleInventoryChanged);
	}

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		// Each key is its own widget so it can take mouse input for drag-and-drop.
		UHotbarSlotWidget* KeySlot = Cast<UHotbarSlotWidget>(
			WidgetTree->FindWidget(FName(*FString::Printf(TEXT("Slot%d"), Index))));
		if (!KeySlot)
		{
			continue;
		}

		UItemDefinition* Bound = Inventory ? Inventory->GetHotbarItem(Index) : nullptr;
		const int32 Held = (Bound && Inventory) ? Inventory->GetItemCount(Bound) : 0;

		KeySlot->SetSlotIndex(Index);
		KeySlot->SetSlotContent(Bound, Held);
	}
}
