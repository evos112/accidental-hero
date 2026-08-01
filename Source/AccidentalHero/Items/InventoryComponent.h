// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * Server-authoritative replicated inventory. Lives on AAccidentalHeroPlayerState (not the
 * Character) so items survive Character destroy/respawn, mirroring this project's own
 * AbilitySystemComponent placement. See AccidentalHeroPlayerState::GetInventoryComponent().
 *
 * Adds originate server-side only (pickup overlap, crafting output, loot) — there is no
 * client-facing Add RPC by design, since there's no client-trust surface to cover yet.
 * Player-initiated removal (e.g. "drop" from UI) goes through Server_RemoveItem.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class ACCIDENTALHERO_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Authoritative mutator — only call where HasAuthority() is true. Returns the quantity
	 *  actually added (may be less than Count if MaxSlots/MaxStackSize limits are hit).
	 *  StartingDurability < 0 means "full"; pass a value to restore a part-worn tool, e.g. when
	 *  picking one back up off the ground. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDefinition* ItemDef, int32 Count = 1, int32 StartingDurability = -1);

	/** True when this item wears out, and therefore may never share an entry with another copy. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	static bool IsDamageable(const UItemDefinition* ItemDef);

	/** Entry index of the tool that should be used for ToolTag: highest tier first, then the
	 *  *lowest* durability within that tier, so a worn tool is finished before a fresh one is
	 *  started. INDEX_NONE when the player carries no such tool. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 FindBestToolEntry(const FGameplayTag& ToolTag) const;

	/** Spends one use of the tool at EntryIndex, removing it if it breaks. Returns true if it
	 *  broke. Authoritative — call only where HasAuthority() is true. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SpendToolDurability(int32 EntryIndex);

	/** Tops a refillable container back up to full. Returns false when nothing needed filling, so
	 *  callers can tell whether the interaction did anything. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RefillContainer(UItemDefinition* Item);

	/** Spends one swig from a container. Unlike SpendToolDurability this never destroys the entry
	 *  at zero — an empty waterskin is still a waterskin, and the whole point is refilling it.
	 *  Returns false when every copy is already empty. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeContainerCharge(UItemDefinition* Item);

	/** Durability of the copy of Item that would be used next, or -1 when it doesn't wear.
	 *  This is what the UI should show: it matches the tool the player would actually swing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetItemDurability(UItemDefinition* Item) const;

	/** Puts Item in hand. Passing null, or an item you don't hold, empties the hand. Equipping the
	 *  item already equipped takes it off again, so one hotbar key toggles. Returns what is now
	 *  equipped. Authoritative — only call where HasAuthority() is true. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	UItemDefinition* EquipItem(UItemDefinition* Item);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	UItemDefinition* GetEquippedItem() const { return EquippedItem; }

	/** Tier of the equipped item if it carries ToolTag, else 0 — which is bare hands. Harvest nodes
	 *  ask this instead of scanning the pack, so what you are holding is what does the work. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	int32 GetEquippedToolTier(const FGameplayTag& ToolTag) const;

	/** Entry index of the equipped item, lowest durability first so a worn copy is finished before
	 *  a fresh one is started. INDEX_NONE when the hand is empty. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	int32 FindEquippedEntry() const;

	/** Authoritative mutator — only call where HasAuthority() is true. Returns the quantity
	 *  actually removed (may be less than Count if the inventory doesn't hold that many). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(UItemDefinition* ItemDef, int32 Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool HasItem(UItemDefinition* ItemDef, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetItemCount(UItemDefinition* ItemDef) const;

	/** Non-mutating capacity check: would AddItem(ItemDef, Count) fit in full? Checks headroom
	 *  in existing understacked entries plus remaining MaxSlots for new slots. Callers that need
	 *  to guarantee no partial add (e.g. crafting output) should check this before mutating. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool CanAddItem(UItemDefinition* ItemDef, int32 Count) const;

	/** Snapshot copy of every held stack (not a live view). Shared by UFurnaceInventoryComponent
	 *  since it derives from this class, so both the player and furnace panels can use it. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	TArray<FInventoryItemEntry> GetAllItems() const;

	/** Client-callable wrapper for player-initiated removal (e.g. "drop" from UI). */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RemoveItem(UItemDefinition* ItemDef, int32 Count);

	/** Number of quick-bar slots. Fixed rather than configurable because the bar's slot widgets are
	 *  laid out in WBP_Inventory; changing this means changing the layout too. */
	static constexpr int32 HotbarSlotCount = 8;

	/** Puts an item on the quick bar. Passing null clears the slot. Assignment is by item type, not
	 *  by stack — the bar points at "Berries", and follows however many you happen to hold. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void AssignHotbarSlot(int32 SlotIndex, UItemDefinition* Item);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Hotbar")
	UItemDefinition* GetHotbarItem(int32 SlotIndex) const;

	/** True when the item already sits somewhere on the bar. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Hotbar")
	bool IsOnHotbar(UItemDefinition* Item) const;

	/** Slot capacity, so UI can draw the right number of usable slots rather than hardcoding it. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	/** Sum of Weight x StackCount across every held stack. Nothing enforces a limit yet — this is
	 *  for display, so the carry meter shows a real number instead of a placeholder. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** Called by FInventoryList's replication callbacks (clients) and directly after
	 *  server-side mutation (the server doesn't get its own OnRep/fast-array callback). */
	void NotifyInventoryChanged();

protected:
	UPROPERTY(Replicated)
	FInventoryList InventoryList;

	/** One entry per quick-bar slot; null means empty. Replicated to the owner only, same as the
	 *  inventory itself — nobody else needs to know what's on your bar. */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UItemDefinition>> HotbarSlots;

	/** What's in hand, held as an item type rather than an entry index so it survives the entry
	 *  array reshuffling when other stacks are added or removed. */
	UPROPERTY(Replicated)
	TObjectPtr<UItemDefinition> EquippedItem;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 MaxSlots = 20;
};
