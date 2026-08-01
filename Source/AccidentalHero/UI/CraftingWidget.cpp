// Copyright Epic Games, Inc. All Rights Reserved.

#include "CraftingWidget.h"
#include "RecipeRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "AccidentalHeroCharacter.h"
#include "Items/CraftingComponent.h"
#include "Items/InventoryComponent.h"
#include "Items/RecipeDefinition.h"
#include "Items/ItemDefinition.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
	/** Small helper so every lookup below stays one line. */
	template <typename T>
	T* FindIn(UWidgetTree* Tree, const TCHAR* Name)
	{
		return Tree ? Cast<T>(Tree->FindWidget(FName(Name))) : nullptr;
	}
}

UCraftingComponent* UCraftingWidget::GetCrafting() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	return PS ? PS->FindComponentByClass<UCraftingComponent>() : nullptr;
}

UInventoryComponent* UCraftingWidget::GetInventory() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	return PS ? PS->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Buttons are transparent overlays sitting on the mockup's painted panels, so the art stays
	// untouched and only the click handling is new.
	if (UButton* Craft = FindIn<UButton>(WidgetTree, TEXT("CraftBtn")))
	{
		Craft->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCraftClicked);
	}
	if (UButton* Minus = FindIn<UButton>(WidgetTree, TEXT("QtyMinusBtn")))
	{
		Minus->OnClicked.AddDynamic(this, &UCraftingWidget::HandleQtyMinus);
	}
	if (UButton* Plus = FindIn<UButton>(WidgetTree, TEXT("QtyPlusBtn")))
	{
		Plus->OnClicked.AddDynamic(this, &UCraftingWidget::HandleQtyPlus);
	}
	if (UButton* Tab = FindIn<UButton>(WidgetTree, TEXT("CatBtn0")))
	{
		Tab->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCategoryAll);
	}
	if (UButton* Tab = FindIn<UButton>(WidgetTree, TEXT("CatBtn1")))
	{
		Tab->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCategoryCrafting);
	}
	if (UButton* Tab = FindIn<UButton>(WidgetTree, TEXT("CatBtn2")))
	{
		Tab->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCategorySmelting);
	}

	// Affordability changes whenever the inventory does, so the greying-out stays honest.
	if (UInventoryComponent* Inventory = GetInventory())
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddDynamic(this, &UCraftingWidget::HandleInventoryChanged);
	}

	// Both categories are gathered once here. GetCraftingRecipes covers Recipe.Category.Crafting;
	// smelting is fetched separately so the third tab isn't permanently empty.
	if (AAccidentalHeroCharacter* Character = Cast<AAccidentalHeroCharacter>(GetOwningPlayerPawn()))
	{
		AllRecipes.Reset();
		for (URecipeDefinition* Recipe : Character->GetCraftingRecipes())
		{
			AllRecipes.AddUnique(Recipe);
		}
		for (URecipeDefinition* Recipe : Character->GetSmeltingRecipes())
		{
			AllRecipes.AddUnique(Recipe);
		}
	}

	RefreshRecipeList();
	ShowRecipeDetail(nullptr);
}

void UCraftingWidget::NativeDestruct()
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UCraftingWidget::HandleInventoryChanged);
		BoundInventory = nullptr;
	}
	Super::NativeDestruct();
}

void UCraftingWidget::HandleInventoryChanged()
{
	RefreshRecipeList();
	ShowRecipeDetail(SelectedRecipe);
}

void UCraftingWidget::SetCategoryFilter(const FGameplayTag& Tag, int32 TabIndex)
{
	CategoryFilter = Tag;
	ActiveTab = TabIndex;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (UImage* Bg = FindIn<UImage>(WidgetTree, *FString::Printf(TEXT("CatBg%d"), Index)))
		{
			Bg->SetOpacity(Index == ActiveTab ? 1.0f : 0.4f);
		}
	}

	RefreshRecipeList();
}

void UCraftingWidget::HandleCategoryAll()      { SetCategoryFilter(FGameplayTag(), 0); }
void UCraftingWidget::HandleCategoryCrafting() { SetCategoryFilter(AccidentalHeroGameplayTags::Recipe_Category_Crafting, 1); }
void UCraftingWidget::HandleCategorySmelting() { SetCategoryFilter(AccidentalHeroGameplayTags::Recipe_Category_Smelting, 2); }

void UCraftingWidget::RefreshRecipeList()
{
	UScrollBox* List = FindIn<UScrollBox>(WidgetTree, TEXT("RecipeScroll"));
	if (!List || !RecipeRowClass)
	{
		return;
	}

	List->ClearChildren();
	Rows.Reset();

	const UCraftingComponent* Crafting = GetCrafting();
	TArray<URecipeDefinition*> Recipes;
	for (const TObjectPtr<URecipeDefinition>& Cached : AllRecipes)
	{
		Recipes.Add(Cached);
	}

	// Affordable first, then alphabetical: the things you can actually make right now rise to the
	// top instead of being buried among things you can't.
	Recipes.Sort([Crafting](const URecipeDefinition& A, const URecipeDefinition& B)
	{
		const bool bA = Crafting && Crafting->CanCraft(const_cast<URecipeDefinition*>(&A));
		const bool bB = Crafting && Crafting->CanCraft(const_cast<URecipeDefinition*>(&B));
		if (bA != bB)
		{
			return bA;
		}
		return A.DisplayName.ToString() < B.DisplayName.ToString();
	});

	int32 Shown = 0;
	for (URecipeDefinition* Recipe : Recipes)
	{
		if (!Recipe)
		{
			continue;
		}
		if (CategoryFilter.IsValid() && !Recipe->RecipeTags.HasTag(CategoryFilter))
		{
			continue;
		}

		URecipeRowWidget* Row = CreateWidget<URecipeRowWidget>(GetOwningPlayer(), RecipeRowClass);
		if (!Row)
		{
			continue;
		}

		Row->SetRecipe(Recipe, Crafting && Crafting->CanCraft(Recipe));
		Row->SetSelected(Recipe == SelectedRecipe);
		Row->OnRecipeRowClicked.AddDynamic(this, &UCraftingWidget::HandleRowClicked);
		List->AddChild(Row);
		Rows.Add(Row);
		++Shown;
	}

	if (UTextBlock* Count = FindIn<UTextBlock>(WidgetTree, TEXT("RecipeCount")))
	{
		Count->SetText(FText::FromString(FString::Printf(TEXT("%d recipes"), Shown)));
	}
}

void UCraftingWidget::HandleRowClicked(URecipeDefinition* Recipe)
{
	SelectedRecipe = Recipe;
	CraftQuantity = 1;

	for (URecipeRowWidget* Row : Rows)
	{
		if (Row)
		{
			Row->SetSelected(Row->GetRecipe() == Recipe);
		}
	}

	ShowRecipeDetail(Recipe);
}

void UCraftingWidget::ShowRecipeDetail(URecipeDefinition* Recipe)
{
	SelectedRecipe = Recipe;

	const UInventoryComponent* Inventory = GetInventory();

	if (UTextBlock* Title = FindIn<UTextBlock>(WidgetTree, TEXT("DetTitle")))
	{
		Title->SetText(Recipe ? Recipe->DisplayName : FText::FromString(TEXT("Select a recipe")));
	}

	if (UTextBlock* Out = FindIn<UTextBlock>(WidgetTree, TEXT("DetOut")))
	{
		Out->SetText(Recipe && Recipe->OutputItem
			? FText::FromString(FString::Printf(TEXT("Makes %d x %s"), Recipe->OutputCount,
				*Recipe->OutputItem->DisplayName.ToString()))
			: FText::GetEmpty());
	}

	if (UTextBlock* Dur = FindIn<UTextBlock>(WidgetTree, TEXT("DetDur")))
	{
		Dur->SetText(Recipe
			? (Recipe->ProcessDuration > 0.0f
				? FText::FromString(FString::Printf(TEXT("Takes %.0f s"), Recipe->ProcessDuration))
				: FText::FromString(TEXT("Instant")))
			: FText::GetEmpty());
	}

	// Ingredient rows show held/needed so a shortfall is obvious without doing the sums yourself.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FCraftingIngredient* Ingredient =
			(Recipe && Recipe->Ingredients.IsValidIndex(Index)) ? &Recipe->Ingredients[Index] : nullptr;

		UTextBlock* NameText = FindIn<UTextBlock>(WidgetTree, *FString::Printf(TEXT("IngName%d"), Index));
		UTextBlock* CountText = FindIn<UTextBlock>(WidgetTree, *FString::Printf(TEXT("IngCnt%d"), Index));

		if (NameText)
		{
			NameText->SetText(Ingredient && Ingredient->Item ? Ingredient->Item->DisplayName : FText::GetEmpty());
		}
		if (CountText)
		{
			if (Ingredient && Ingredient->Item)
			{
				const int32 Held = Inventory ? Inventory->GetItemCount(Ingredient->Item) : 0;
				CountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Held, Ingredient->Count)));
				CountText->SetColorAndOpacity(Held >= Ingredient->Count
					? FSlateColor(FLinearColor(0.55f, 0.90f, 0.50f, 1.0f))
					: FSlateColor(FLinearColor(0.95f, 0.45f, 0.40f, 1.0f)));
			}
			else
			{
				CountText->SetText(FText::GetEmpty());
			}
		}
	}

	// The mockup only has two ingredient rows; several recipes need three. Say so rather than
	// silently hiding a requirement the player would then fail on.
	if (UTextBlock* Warn = FindIn<UTextBlock>(WidgetTree, TEXT("WarnTxt")))
	{
		const int32 Extra = Recipe ? FMath::Max(0, Recipe->Ingredients.Num() - 2) : 0;
		Warn->SetText(Extra > 0
			? FText::FromString(FString::Printf(TEXT("+ %d more ingredient(s)"), Extra))
			: FText::GetEmpty());
	}

	UpdateCraftButtonState();
}

void UCraftingWidget::UpdateCraftButtonState()
{
	const UCraftingComponent* Crafting = GetCrafting();
	const bool bCanCraft = SelectedRecipe && Crafting && Crafting->CanCraft(SelectedRecipe);

	if (UTextBlock* Qty = FindIn<UTextBlock>(WidgetTree, TEXT("QtyVal")))
	{
		Qty->SetText(FText::AsNumber(CraftQuantity));
	}

	if (UImage* Bg = FindIn<UImage>(WidgetTree, TEXT("CraftBtnBg")))
	{
		Bg->SetOpacity(bCanCraft ? 1.0f : 0.35f);
	}

	if (UTextBlock* Label = FindIn<UTextBlock>(WidgetTree, TEXT("CraftLbl")))
	{
		Label->SetText(FText::FromString(
			!SelectedRecipe ? TEXT("SELECT A RECIPE") : (bCanCraft ? TEXT("CRAFT") : TEXT("MISSING MATERIALS"))));
	}
}

void UCraftingWidget::HandleQtyMinus()
{
	CraftQuantity = FMath::Max(1, CraftQuantity - 1);
	UpdateCraftButtonState();
}

void UCraftingWidget::HandleQtyPlus()
{
	CraftQuantity = FMath::Min(MaxCraftQuantity, CraftQuantity + 1);
	UpdateCraftButtonState();
}

void UCraftingWidget::HandleCraftClicked()
{
	UCraftingComponent* Crafting = GetCrafting();
	if (!SelectedRecipe || !Crafting)
	{
		return;
	}

	// Craft one at a time and stop the moment the materials run out, so a quantity larger than the
	// player can afford makes as many as it can instead of failing outright.
	int32 Made = 0;
	for (int32 Attempt = 0; Attempt < CraftQuantity; ++Attempt)
	{
		if (!Crafting->CanCraft(SelectedRecipe))
		{
			break;
		}
		Crafting->Server_TryCraft(SelectedRecipe);
		++Made;
	}

	if (UTextBlock* Warn = FindIn<UTextBlock>(WidgetTree, TEXT("WarnTxt")))
	{
		Warn->SetText(FText::FromString(Made > 0
			? FString::Printf(TEXT("Crafted %d"), Made)
			: TEXT("Not enough materials")));
	}

	RefreshRecipeList();
	ShowRecipeDetail(SelectedRecipe);
}
