// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_RangedDamage.h"
#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"

UGE_RangedDamage::UGE_RangedDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = AccidentalHeroGameplayTags::Data_RangedDamage;

	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = UAccidentalHeroAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
	Modifiers.Add(HealthModifier);
}
