// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_GatherCooldown.h"
#include "AccidentalHeroGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_GatherCooldown::UGE_GatherCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
}

void UGE_GatherCooldown::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FInheritedTagContainer GrantedTags;
		GrantedTags.Added.AddTag(AccidentalHeroGameplayTags::Cooldown_Gather);

		FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);
	}
}
