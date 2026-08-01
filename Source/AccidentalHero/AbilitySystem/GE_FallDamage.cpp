// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_FallDamage.h"
#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"

UGE_FallDamage::UGE_FallDamage()
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
