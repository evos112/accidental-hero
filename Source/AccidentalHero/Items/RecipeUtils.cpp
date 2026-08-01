// Copyright Epic Games, Inc. All Rights Reserved.

#include "RecipeUtils.h"
#include "RecipeDefinition.h"
#include "InventoryComponent.h"

bool RecipeUtils::CanAfford(const URecipeDefinition* Recipe, const UInventoryComponent* Inventory)
{
	if (!Recipe || !Inventory || Recipe->Ingredients.Num() == 0 || !Recipe->OutputItem || Recipe->OutputCount <= 0)
	{
		return false;
	}

	TMap<UItemDefinition*, int32> RequiredTotals;
	for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Ingredient.Item || Ingredient.Count <= 0)
		{
			return false;
		}
		RequiredTotals.FindOrAdd(Ingredient.Item) += Ingredient.Count;
	}

	for (const TPair<UItemDefinition*, int32>& Pair : RequiredTotals)
	{
		if (!Inventory->HasItem(Pair.Key, Pair.Value))
		{
			return false;
		}
	}

	return Inventory->CanAddItem(Recipe->OutputItem, Recipe->OutputCount);
}
