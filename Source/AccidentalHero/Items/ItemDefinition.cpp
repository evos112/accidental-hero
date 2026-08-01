// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemDefinition.h"
#include "GameplayTagsManager.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Item"), ItemID);
}

FGameplayTag UItemDefinition::RequestGameplayTag(FName TagName)
{
	return UGameplayTagsManager::Get().RequestGameplayTag(TagName);
}
