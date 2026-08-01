// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_SprintStaminaDrain.h"
#include "AccidentalHeroAttributeSet.h"

UGE_SprintStaminaDrain::UGE_SprintStaminaDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = true;

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = UAccidentalHeroAttributeSet::GetStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-25.0f));
	Modifiers.Add(StaminaModifier);
}
