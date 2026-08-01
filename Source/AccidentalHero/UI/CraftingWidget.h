// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "CraftingWidget.generated.h"

class URecipeDefinition;
class URecipeRowWidget;
class UCraftingComponent;
class UInventoryComponent;
class UScrollBox;

/**
 * Drives WBP_Crafting: category tabs, the recipe list, the ingredient breakdown and the craft
 * button. Until this existed the panel was pure mockup and nothing in the game could be crafted
 * except from script.
 *
 * Recipes come from AAccidentalHeroCharacter::GetCraftingRecipes (asset-manager driven), so adding
 * a recipe asset is enough to make it appear — there is no list to maintain here.
 */
UCLASS()
class ACCIDENTALHERO_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the list for the active category and refreshes affordability. */
	UFUNCTION(BlueprintCallable, Category = "Crafting UI")
	void RefreshRecipeList();

	/** Fills the right-hand panel from the selected recipe. */
	UFUNCTION(BlueprintCallable, Category = "Crafting UI")
	void ShowRecipeDetail(URecipeDefinition* Recipe);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleRowClicked(URecipeDefinition* Recipe);

	UFUNCTION()
	void HandleCraftClicked();

	UFUNCTION()
	void HandleQtyMinus();

	UFUNCTION()
	void HandleQtyPlus();

	UFUNCTION()
	void HandleCategoryAll();

	UFUNCTION()
	void HandleCategoryCrafting();

	UFUNCTION()
	void HandleCategorySmelting();

	UFUNCTION()
	void HandleInventoryChanged();

	/** Row widget spawned once per recipe. Unset means the list stays empty. */
	UPROPERTY(EditDefaultsOnly, Category = "Crafting UI")
	TSubclassOf<URecipeRowWidget> RecipeRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "Crafting UI", meta = (ClampMin = "1"))
	int32 MaxCraftQuantity = 20;

private:
	UCraftingComponent* GetCrafting() const;
	UInventoryComponent* GetInventory() const;

	/** Applies the active category filter and refreshes the craft button's enabled look. */
	void SetCategoryFilter(const FGameplayTag& Tag, int32 TabIndex);
	void UpdateCraftButtonState();

	/** Loaded once in NativeConstruct. Recipe lookup does a blocking asset load, so it must never
	 *  run per refresh — and refreshes happen on every inventory change. */
	UPROPERTY()
	TArray<TObjectPtr<URecipeDefinition>> AllRecipes;

	UPROPERTY()
	TObjectPtr<URecipeDefinition> SelectedRecipe;

	UPROPERTY()
	TArray<TObjectPtr<URecipeRowWidget>> Rows;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> BoundInventory;

	/** Empty tag means "All". */
	FGameplayTag CategoryFilter;
	int32 ActiveTab = 0;
	int32 CraftQuantity = 1;
};
