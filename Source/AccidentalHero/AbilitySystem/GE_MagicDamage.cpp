// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_MagicDamage.h"
#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"

UGE_MagicDamage::UGE_MagicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = AccidentalHeroGameplayTags::Data_MagicDamage;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UAccidentalHeroAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
	Modifiers.Add(DamageModifier);
}
