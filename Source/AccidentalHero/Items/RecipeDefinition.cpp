// Copyright Epic Games, Inc. All Rights Reserved.

#include "RecipeDefinition.h"

FPrimaryAssetId URecipeDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Recipe"), RecipeID);
}
