// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_MagicCooldown.h"
#include "AccidentalHeroGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_MagicCooldown::UGE_MagicCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));
}

void UGE_MagicCooldown::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FInheritedTagContainer GrantedTags;
		GrantedTags.Added.AddTag(AccidentalHeroGameplayTags::Cooldown_MagicBolt);

		FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);
	}
}
