// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RecipeDefinition.generated.h"

class UItemDefinition;

/** One ingredient row: an item and how many of it a recipe consumes. */
USTRUCT(BlueprintType)
struct FCraftingIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ingredient")
	TObjectPtr<UItemDefinition> Item = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ingredient", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * Data-only description of a crafting recipe: ingredients consumed, item produced. See
 * UCraftingComponent for how these are validated/executed. RecipeTags is currently unread by
 * UCraftingComponent — it exists so a future smelting/building pass can filter "recipe type"
 * (e.g. Recipe.Category.Smelting) without a data migration.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API URecipeDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Stable designer-assigned ID used for save/load and cross-referencing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FName RecipeID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftingIngredient> Ingredients;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TObjectPtr<UItemDefinition> OutputItem = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "1"))
	int32 OutputCount = 1;

	/** Seconds this recipe takes to resolve once started. 0 (default) = instant, i.e. every
	 *  existing crafting recipe is unaffected. >0 = timed (furnace smelting today; a future
	 *  timed building-construction step could reuse this same field). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0.0"))
	float ProcessDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (Categories = "Recipe.Category"))
	FGameplayTagContainer RecipeTags;

	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UObject Interface
};
