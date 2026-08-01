// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class URecipeDefinition;
class UInventoryComponent;

/**
 * Shared "can this inventory afford this recipe" check, used by both UCraftingComponent (instant
 * crafting from a player's own inventory) and AFurnace (timed smelting from a furnace's own
 * inventory) so the ingredient-aggregation logic lives in exactly one place. Aggregation matters:
 * a naive per-row check is fooled if a recipe (accidentally) lists the same item across multiple
 * ingredient rows, since each row's check would pass independently against the current total even
 * if the combined requirement exceeds what's actually held.
 */
namespace RecipeUtils
{
	ACCIDENTALHERO_API bool CanAfford(const URecipeDefinition* Recipe, const UInventoryComponent* Inventory);
}
