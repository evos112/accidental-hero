// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_BiomeDrain.h"

#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"

UGE_BiomeDrain::UGE_BiomeDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = false;

	// Signed as the caller passes them: UBiomeDefinition states drain as a positive number, and the
	// character negates it once when building the spec, so the sign convention lives in one place.
	FSetByCallerFloat HungerSetByCaller;
	HungerSetByCaller.DataTag = AccidentalHeroGameplayTags::Data_Biome_Hunger;

	FGameplayModifierInfo HungerModifier;
	HungerModifier.Attribute = UAccidentalHeroAttributeSet::GetHungerAttribute();
	HungerModifier.ModifierOp = EGameplayModOp::Additive;
	HungerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HungerSetByCaller);
	Modifiers.Add(HungerModifier);

	FSetByCallerFloat ThirstSetByCaller;
	ThirstSetByCaller.DataTag = AccidentalHeroGameplayTags::Data_Biome_Thirst;

	FGameplayModifierInfo ThirstModifier;
	ThirstModifier.Attribute = UAccidentalHeroAttributeSet::GetThirstAttribute();
	ThirstModifier.ModifierOp = EGameplayModOp::Additive;
	ThirstModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ThirstSetByCaller);
	Modifiers.Add(ThirstModifier);
}
