// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryComponent.h"
#include "ItemDefinition.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	InventoryList.OwnerComponent = this;
	HotbarSlots.SetNum(HotbarSlotCount);
}

void UInventoryComponent::AssignHotbarSlot(int32 SlotIndex, UItemDefinition* Item)
{
	if (!HotbarSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	// An item lives in exactly one slot; assigning it somewhere new vacates the old spot rather
	// than leaving the same thing bound to two keys.
	if (Item)
	{
		for (int32 Index = 0; Index < HotbarSlots.Num(); ++Index)
		{
			if (Index != SlotIndex && HotbarSlots[Index] == Item)
			{
				HotbarSlots[Index] = nullptr;
			}
		}
	}

	HotbarSlots[SlotIndex] = Item;
	NotifyInventoryChanged();
}

UItemDefinition* UInventoryComponent::GetHotbarItem(int32 SlotIndex) const
{
	return HotbarSlots.IsValidIndex(SlotIndex) ? HotbarSlots[SlotIndex] : nullptr;
}

bool UInventoryComponent::IsOnHotbar(UItemDefinition* Item) const
{
	return Item && HotbarSlots.Contains(Item);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Private per-player inventory: only the owning connection needs to see the contents.
	// UFurnaceInventoryComponent overrides this function to register InventoryList with a
	// different (still compile-time-constant, as DOREPLIFETIME_CONDITION requires) condition.
	DOREPLIFETIME_CONDITION(UInventoryComponent, InventoryList, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComponent, HotbarSlots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComponent, EquippedItem, COND_OwnerOnly);
}

UItemDefinition* UInventoryComponent::EquipItem(UItemDefinition* Item)
{
	// Re-equipping what's already in hand puts it away, so one hotbar key is both equip and unequip.
	EquippedItem = (Item && Item != EquippedItem && HasItem(Item, 1)) ? Item : nullptr;
	NotifyInventoryChanged();
	return EquippedItem;
}

int32 UInventoryComponent::GetEquippedToolTier(const FGameplayTag& ToolTag) const
{
	if (!EquippedItem || !ToolTag.IsValid() || !EquippedItem->ItemTags.HasTag(ToolTag))
	{
		return 0;
	}
	return EquippedItem->ToolTier;
}

int32 UInventoryComponent::FindEquippedEntry() const
{
	if (!EquippedItem)
	{
		return INDEX_NONE;
	}

	int32 Best = INDEX_NONE;
	for (int32 Index = 0; Index < InventoryList.Items.Num(); ++Index)
	{
		const FInventoryItemEntry& Entry = InventoryList.Items[Index];
		if (Entry.ItemDef != EquippedItem || Entry.StackCount <= 0)
		{
			continue;
		}
		// Lowest durability first, matching FindBestToolEntry: finish a worn copy before starting
		// a fresh one, so the player isn't left with a pack full of half-used tools.
		if (Best == INDEX_NONE || Entry.Durability < InventoryList.Items[Best].Durability)
		{
			Best = Index;
		}
	}
	return Best;
}

bool UInventoryComponent::IsDamageable(const UItemDefinition* ItemDef)
{
	return ItemDef && ItemDef->MaxDurability > 0;
}

int32 UInventoryComponent::AddItem(UItemDefinition* ItemDef, int32 Count, int32 StartingDurability)
{
	if (!ItemDef || Count <= 0)
	{
		return 0;
	}

	// A worn tool must never be merged into a fresh one — one of the two durabilities would just
	// vanish. Damageable items are unstackable regardless of what MaxStackSize says, and
	// CanAddItem applies the identical rule so the two can't disagree.
	const bool bDamageable = IsDamageable(ItemDef);

	int32 Remaining = Count;

	// Top up existing understacked entries first.
	if (!bDamageable)
	{
		for (FInventoryItemEntry& Entry : InventoryList.Items)
		{
			if (Entry.ItemDef != ItemDef || Remaining <= 0)
			{
				continue;
			}

			const int32 SpaceInStack = ItemDef->MaxStackSize - Entry.StackCount;
			if (SpaceInStack <= 0)
			{
				continue;
			}

			const int32 AmountToAdd = FMath::Min(SpaceInStack, Remaining);
			Entry.StackCount += AmountToAdd;
			Remaining -= AmountToAdd;
			InventoryList.MarkItemDirty(Entry);
		}
	}

	// Create new slots for whatever didn't fit into existing stacks.
	while (Remaining > 0 && InventoryList.Items.Num() < MaxSlots)
	{
		FInventoryItemEntry& NewEntry = InventoryList.Items.AddDefaulted_GetRef();
		NewEntry.ItemDef = ItemDef;
		NewEntry.StackCount = bDamageable ? 1 : FMath::Min(ItemDef->MaxStackSize, Remaining);
		NewEntry.Durability = bDamageable
			? (StartingDurability >= 0 ? FMath::Min(StartingDurability, ItemDef->MaxDurability)
									   : ItemDef->MaxDurability)
			: 0;
		Remaining -= NewEntry.StackCount;
		InventoryList.MarkArrayDirty();
	}

	const int32 AddedCount = Count - Remaining;
	if (AddedCount > 0)
	{
		// First time you pick something up it claims a free quick-bar slot. There's no drag-and-drop
		// yet, so without this the bar would stay empty; AssignHotbarSlot still lets it be overridden.
		if (!IsOnHotbar(ItemDef))
		{
			const int32 FreeSlot = HotbarSlots.IndexOfByPredicate(
				[](const TObjectPtr<UItemDefinition>& Slot) { return Slot == nullptr; });
			if (FreeSlot != INDEX_NONE)
			{
				HotbarSlots[FreeSlot] = ItemDef;
			}
		}

		NotifyInventoryChanged();
	}
	return AddedCount;
}

int32 UInventoryComponent::RemoveItem(UItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;

	for (int32 Index = InventoryList.Items.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FInventoryItemEntry& Entry = InventoryList.Items[Index];
		if (Entry.ItemDef != ItemDef)
		{
			continue;
		}

		const int32 AmountToRemove = FMath::Min(Entry.StackCount, Remaining);
		Entry.StackCount -= AmountToRemove;
		Remaining -= AmountToRemove;

		if (Entry.StackCount <= 0)
		{
			// Ordered removal (not RemoveAtSwap) to keep slot order stable for UI binding;
			// inventories are small, so the shift cost is negligible.
			InventoryList.Items.RemoveAt(Index);
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(Entry);
		}
	}

	const int32 RemovedCount = Count - Remaining;
	if (RemovedCount > 0)
	{
		NotifyInventoryChanged();
	}
	return RemovedCount;
}

bool UInventoryComponent::HasItem(UItemDefinition* ItemDef, int32 Count) const
{
	return GetItemCount(ItemDef) >= Count;
}

bool UInventoryComponent::CanAddItem(UItemDefinition* ItemDef, int32 Count) const
{
	if (!ItemDef || Count <= 0)
	{
		return false;
	}

	// Must mirror AddItem exactly, including the damageable-is-unstackable rule — if these two
	// disagree, crafting's "will the output fit?" check stops predicting what AddItem does.
	const bool bDamageable = IsDamageable(ItemDef);
	const int32 SlotsAvailable = MaxSlots - InventoryList.Items.Num();

	if (bDamageable)
	{
		// One slot each, no top-up possible.
		return Count <= SlotsAvailable;
	}

	int32 Remaining = Count;

	// Headroom in existing understacked entries, same as AddItem's top-up pass.
	for (const FInventoryItemEntry& Entry : InventoryList.Items)
	{
		if (Entry.ItemDef != ItemDef || Remaining <= 0)
		{
			continue;
		}
		const int32 SpaceInStack = ItemDef->MaxStackSize - Entry.StackCount;
		Remaining -= FMath::Max(SpaceInStack, 0);
	}

	if (Remaining <= 0)
	{
		return true;
	}

	// Whatever's left needs new slots, same sizing as AddItem's slot-creation pass.
	const int32 SlotsNeeded = FMath::DivideAndRoundUp(Remaining, ItemDef->MaxStackSize);
	return SlotsNeeded <= SlotsAvailable;
}

int32 UInventoryComponent::FindBestToolEntry(const FGameplayTag& ToolTag) const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestTier = 0;
	int32 BestDurability = MAX_int32;

	for (int32 Index = 0; Index < InventoryList.Items.Num(); ++Index)
	{
		const FInventoryItemEntry& Entry = InventoryList.Items[Index];
		if (Entry.StackCount <= 0 || !Entry.ItemDef || !Entry.ItemDef->ItemTags.HasTag(ToolTag))
		{
			continue;
		}

		const int32 Tier = Entry.ItemDef->ToolTier;
		if (Tier <= 0)
		{
			continue;
		}

		// An indestructible tool sorts as infinitely durable, so it's only picked when there's no
		// worn one of the same tier to finish off first.
		const int32 Durability = IsDamageable(Entry.ItemDef) ? Entry.Durability : MAX_int32;

		if (Tier > BestTier || (Tier == BestTier && Durability < BestDurability))
		{
			BestTier = Tier;
			BestDurability = Durability;
			BestIndex = Index;
		}
	}

	return BestIndex;
}

bool UInventoryComponent::SpendToolDurability(int32 EntryIndex)
{
	if (!InventoryList.Items.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FInventoryItemEntry& Entry = InventoryList.Items[EntryIndex];
	if (!IsDamageable(Entry.ItemDef))
	{
		return false;
	}

	const UItemDefinition* ItemDef = Entry.ItemDef;
	const int32 Before = Entry.Durability;
	Entry.Durability = FMath::Max(0, Entry.Durability - 1);

	// Warn exactly on the crossing, which makes it once per tool without tracking any extra state.
	const int32 WarnAt = FMath::Max(1, FMath::RoundToInt(ItemDef->MaxDurability * 0.2f));
	if (Before > WarnAt && Entry.Durability <= WarnAt && Entry.Durability > 0 && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange,
			FString::Printf(TEXT("Your %s is worn out"), *ItemDef->DisplayName.ToString()));
	}

	const bool bBroke = Entry.Durability <= 0;
	if (bBroke)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
				FString::Printf(TEXT("Your %s broke!"), *ItemDef->DisplayName.ToString()));
		}
		InventoryList.Items.RemoveAt(EntryIndex);
		InventoryList.MarkArrayDirty();
	}
	else
	{
		InventoryList.MarkItemDirty(Entry);
	}

	NotifyInventoryChanged();
	return bBroke;
}

bool UInventoryComponent::RefillContainer(UItemDefinition* Item)
{
	if (!Item || !Item->IsWaterContainer())
	{
		return false;
	}

	bool bFilledAny = false;
	for (FInventoryItemEntry& Entry : InventoryList.Items)
	{
		if (Entry.ItemDef == Item && Entry.Durability < Item->MaxDurability)
		{
			Entry.Durability = Item->MaxDurability;
			InventoryList.MarkItemDirty(Entry);
			bFilledAny = true;
		}
	}

	if (bFilledAny)
	{
		NotifyInventoryChanged();
	}
	return bFilledAny;
}

bool UInventoryComponent::ConsumeContainerCharge(UItemDefinition* Item)
{
	if (!Item || !Item->IsWaterContainer())
	{
		return false;
	}

	// Fullest first, so a part-used skin isn't drained to nothing while a full one sits beside it.
	int32 BestIndex = INDEX_NONE;
	int32 BestCharges = 0;
	for (int32 Index = 0; Index < InventoryList.Items.Num(); ++Index)
	{
		const FInventoryItemEntry& Entry = InventoryList.Items[Index];
		if (Entry.ItemDef == Item && Entry.Durability > BestCharges)
		{
			BestCharges = Entry.Durability;
			BestIndex = Index;
		}
	}

	if (BestIndex == INDEX_NONE)
	{
		return false;
	}

	FInventoryItemEntry& Entry = InventoryList.Items[BestIndex];
	Entry.Durability = FMath::Max(0, Entry.Durability - 1);
	InventoryList.MarkItemDirty(Entry);
	NotifyInventoryChanged();
	return true;
}

int32 UInventoryComponent::GetItemDurability(UItemDefinition* Item) const
{
	if (!IsDamageable(Item))
	{
		return -1;
	}

	// Lowest first, matching FindBestToolEntry — the UI should show the tool you'd actually swing.
	int32 Lowest = -1;
	for (const FInventoryItemEntry& Entry : InventoryList.Items)
	{
		if (Entry.ItemDef == Item && Entry.StackCount > 0)
		{
			Lowest = (Lowest < 0) ? Entry.Durability : FMath::Min(Lowest, Entry.Durability);
		}
	}
	return Lowest;
}

TArray<FInventoryItemEntry> UInventoryComponent::GetAllItems() const
{
	return InventoryList.Items;
}

float UInventoryComponent::GetTotalWeight() const
{
	float Total = 0.0f;
	for (const FInventoryItemEntry& Entry : InventoryList.Items)
	{
		if (Entry.ItemDef)
		{
			Total += Entry.ItemDef->Weight * Entry.StackCount;
		}
	}
	return Total;
}

int32 UInventoryComponent::GetItemCount(UItemDefinition* ItemDef) const
{
	int32 Total = 0;
	for (const FInventoryItemEntry& Entry : InventoryList.Items)
	{
		if (Entry.ItemDef == ItemDef)
		{
			Total += Entry.StackCount;
		}
	}
	return Total;
}

void UInventoryComponent::Server_RemoveItem_Implementation(UItemDefinition* ItemDef, int32 Count)
{
	RemoveItem(ItemDef, Count);
}

bool UInventoryComponent::Server_RemoveItem_Validate(UItemDefinition* ItemDef, int32 Count)
{
	return Count > 0;
}

void UInventoryComponent::NotifyInventoryChanged()
{
	// One place to catch the hand holding something that is no longer in the pack — a tool that
	// broke, a stack that was eaten, an item dropped. Every mutation path ends here. Server-only:
	// on clients EquippedItem arrives by replication and must not be second-guessed, since their
	// copy of InventoryList may not have caught up yet.
	if (EquippedItem && GetOwnerRole() == ROLE_Authority && !HasItem(EquippedItem, 1))
	{
		EquippedItem = nullptr;
	}

	OnInventoryChanged.Broadcast();
}
