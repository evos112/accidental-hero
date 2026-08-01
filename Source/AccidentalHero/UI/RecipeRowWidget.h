// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RecipeRowWidget.generated.h"

class URecipeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeRowClicked, URecipeDefinition*, Recipe);

/**
 * One selectable line in the crafting list.
 *
 * Rows are created at runtime, one per recipe, rather than being four fixed widgets on the canvas:
 * there are already fourteen recipes and adding a fifteenth shouldn't mean editing the layout.
 */
UCLASS()
class ACCIDENTALHERO_API URecipeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the row in. bAffordable tints it so the player can see what they can make at a glance. */
	UFUNCTION(BlueprintCallable, Category = "Recipe Row")
	void SetRecipe(URecipeDefinition* InRecipe, bool bAffordable);

	UFUNCTION(BlueprintPure, Category = "Recipe Row")
	URecipeDefinition* GetRecipe() const { return Recipe; }

	/** Draws the selection highlight on exactly one row. */
	UFUNCTION(BlueprintCallable, Category = "Recipe Row")
	void SetSelected(bool bInSelected);

	UPROPERTY(BlueprintAssignable, Category = "Recipe Row")
	FOnRecipeRowClicked OnRecipeRowClicked;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	TObjectPtr<URecipeDefinition> Recipe;

	bool bSelected = false;
	bool bCanAfford = false;
};
