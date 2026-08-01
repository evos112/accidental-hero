// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_MeleeDamage.h"
#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"

UGE_MeleeDamage::UGE_MeleeDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = AccidentalHeroGameplayTags::Data_Damage;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UAccidentalHeroAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
	Modifiers.Add(DamageModifier);
}
