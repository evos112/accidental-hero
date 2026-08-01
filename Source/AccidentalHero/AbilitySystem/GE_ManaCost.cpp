// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_ManaCost.h"
#include "AccidentalHeroAttributeSet.h"

UGE_ManaCost::UGE_ManaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ManaModifier;
	ManaModifier.Attribute = UAccidentalHeroAttributeSet::GetManaAttribute();
	ManaModifier.ModifierOp = EGameplayModOp::Additive;
	ManaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-30.0f));
	Modifiers.Add(ManaModifier);
}
