// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_SurvivalDrain.h"
#include "AccidentalHeroAttributeSet.h"

UGE_SurvivalDrain::UGE_SurvivalDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = false;

	// Tuned for the 15-30 minute sessions the game is built around (SPEC 5.8/5.9): a full bar
	// empties in ~28 minutes of hunger and ~18 of thirst, which works out at roughly one meal and
	// one drink per sitting. The previous rates (0.12 / 0.18) emptied in 14 and 9 minutes — three
	// drinks and a starvation inside a single short session, which crowded out actually playing.
	FGameplayModifierInfo HungerModifier;
	HungerModifier.Attribute = UAccidentalHeroAttributeSet::GetHungerAttribute();
	HungerModifier.ModifierOp = EGameplayModOp::Additive;
	HungerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-0.06f));
	Modifiers.Add(HungerModifier);

	FGameplayModifierInfo ThirstModifier;
	ThirstModifier.Attribute = UAccidentalHeroAttributeSet::GetThirstAttribute();
	ThirstModifier.ModifierOp = EGameplayModOp::Additive;
	ThirstModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-0.09f));
	Modifiers.Add(ThirstModifier);
}
