// Copyright Epic Games, Inc. All Rights Reserved.

#include "CraftingComponent.h"
#include "InventoryComponent.h"
#include "RecipeDefinition.h"
#include "RecipeUtils.h"

UCraftingComponent::UCraftingComponent()
{
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	GetInventoryComponent(); // warm the cache
}

UInventoryComponent* UCraftingComponent::GetInventoryComponent() const
{
	if (!CachedInventoryComponent && GetOwner())
	{
		CachedInventoryComponent = GetOwner()->FindComponentByClass<UInventoryComponent>();
	}
	return CachedInventoryComponent;
}

bool UCraftingComponent::CanCraft(URecipeDefinition* Recipe) const
{
	return RecipeUtils::CanAfford(Recipe, GetInventoryComponent());
}

bool UCraftingComponent::TryCraft(URecipeDefinition* Recipe)
{
	if (!CanCraft(Recipe))
	{
		OnCraftingResult.Broadcast(Recipe, false);
		return false;
	}

	UInventoryComponent* Inventory = GetInventoryComponent();
	for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
	{
		Inventory->RemoveItem(Ingredient.Item, Ingredient.Count);
	}
	Inventory->AddItem(Recipe->OutputItem, Recipe->OutputCount);

	OnCraftingResult.Broadcast(Recipe, true);
	return true;
}

void UCraftingComponent::Server_TryCraft_Implementation(URecipeDefinition* Recipe)
{
	TryCraft(Recipe);
}

bool UCraftingComponent::Server_TryCraft_Validate(URecipeDefinition* Recipe)
{
	return Recipe != nullptr;
}
