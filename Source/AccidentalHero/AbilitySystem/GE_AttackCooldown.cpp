// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_AttackCooldown.h"
#include "AccidentalHeroGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_AttackCooldown::UGE_AttackCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
}

void UGE_AttackCooldown::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FInheritedTagContainer GrantedTags;
		GrantedTags.Added.AddTag(AccidentalHeroGameplayTags::Cooldown_Attack);

		FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);
	}
}
