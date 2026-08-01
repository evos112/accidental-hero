// Copyright Epic Games, Inc. All Rights Reserved.

#include "RecipeRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/RecipeDefinition.h"
#include "Items/ItemDefinition.h"
#include "Input/Reply.h"

void URecipeRowWidget::SetRecipe(URecipeDefinition* InRecipe, bool bAffordable)
{
	Recipe = InRecipe;
	bCanAfford = bAffordable;

	if (!WidgetTree)
	{
		return;
	}

	if (UTextBlock* Name = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Name"))))
	{
		Name->SetText(Recipe ? Recipe->DisplayName : FText::GetEmpty());
		// Greyed out when you can't afford it, rather than hidden — the player should still see
		// what exists and work out what to gather.
		Name->SetColorAndOpacity(bCanAfford
			? FSlateColor(FLinearColor(0.93f, 0.92f, 0.86f, 1.0f))
			: FSlateColor(FLinearColor(0.50f, 0.49f, 0.47f, 1.0f)));
	}

	if (UTextBlock* Sub = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Sub"))))
	{
		FString Line;
		if (Recipe)
		{
			// Compact ingredient summary, e.g. "5x Plant Fiber".
			TArray<FString> Parts;
			for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
			{
				if (Ingredient.Item)
				{
					Parts.Add(FString::Printf(TEXT("%dx %s"), Ingredient.Count,
						*Ingredient.Item->DisplayName.ToString()));
				}
			}
			Line = FString::Join(Parts, TEXT("  "));
		}
		Sub->SetText(FText::FromString(Line));
	}

	SetSelected(bSelected);
}

void URecipeRowWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;

	if (!WidgetTree)
	{
		return;
	}

	if (UImage* Bg = Cast<UImage>(WidgetTree->FindWidget(TEXT("Bg"))))
	{
		Bg->SetOpacity(bSelected ? 1.0f : 0.35f);
	}
}

FReply URecipeRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Recipe && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnRecipeRowClicked.Broadcast(Recipe);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
