// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDefinition.generated.h"

/**
 * Data-only description of an inventory item: identity, display info, stacking limit, and
 * category tags. Category tags (Item.Category.*, see AccidentalHeroGameplayTags) are the
 * extension point crafting/smelting/building recipes key off of (e.g. "requires an item
 * tagged Item.Category.Ore"). See UInventoryComponent for how these are held/replicated.
 *
 * Deliberately separate from UWeaponDefinition for now — weapons aren't routed through the
 * inventory yet, so unifying them isn't worth touching working combat code today.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Stable designer-assigned ID used for save/load and recipe cross-referencing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Model shown when the item lies on the ground as an AItemPickup, and the mesh a future
	 *  equip system would attach to the hand. Soft so an item's art isn't loaded until something
	 *  actually needs to draw it — the inventory only ever touches Icon. Falls back to the
	 *  pickup's default mesh when unset, so an item without art is still visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Uniform scale applied to WorldMesh on a ground pickup. Two jobs: correcting art authored
	 *  in the wrong units (a lot of marketplace meshes come in at 100x), and making a dropped
	 *  item read smaller than the thing it came from — a stone pickup shouldn't be boulder-sized. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.001"))
	float WorldMeshScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** Category membership, e.g. Item.Category.Ore. Crafting/smelting/building recipes key off these. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (Categories = "Item.Category"))
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float Weight = 1.0f;

	/** Tool quality: 0 = not a tool, 1 = stone, 2 = iron, and upward. Harvest nodes multiply their
	 *  work rate by this, so tier is data on the item rather than a hardcoded list of item
	 *  pointers per node type. What the tool is *for* comes from its Item.Tool.* tag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "0"))
	int32 ToolTier = 0;

	/** Uses before the tool breaks. 0 = indestructible, which is every non-tool and anything we
	 *  simply don't want to wear out. Only meaningful on items that occupy a slot each, i.e.
	 *  MaxStackSize 1 — UInventoryComponent treats anything with durability as unstackable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "0"))
	int32 MaxDurability = 0;

	/** Hunger restored when this item is consumed. Zero means it isn't food. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float HungerRestore = 0.0f;

	/** Thirst restored when this item is consumed. Zero means it isn't a drink. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float ThirstRestore = 0.0f;

	/** Health restored when consumed — berries heal a little, cooked meals more. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float HealthRestore = 0.0f;

	/** True when the item restores anything, i.e. the inventory "Use" action should be offered. */
	UFUNCTION(BlueprintPure, Category = "Item|Consumable")
	bool IsConsumable() const { return HungerRestore > 0.0f || ThirstRestore > 0.0f || HealthRestore > 0.0f; }

	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UObject Interface

	/** Resolves an already-registered gameplay tag by its dotted name (e.g. "Item.Category.Ore").
	 *  Exists because the engine deliberately doesn't expose a runtime string->tag conversion to
	 *  Blueprint/Python (tag literals are normally compile-time-picked) — useful for editor
	 *  automation and any future data-driven recipe/tag resolution. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
	static FGameplayTag RequestGameplayTag(FName TagName);
};
