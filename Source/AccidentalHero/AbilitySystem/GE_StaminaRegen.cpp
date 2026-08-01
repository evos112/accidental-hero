// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_StaminaRegen.h"
#include "AccidentalHeroAttributeSet.h"

UGE_StaminaRegen::UGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = false;

	FAttributeBasedFloat RegenMagnitude;
	RegenMagnitude.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UAccidentalHeroAttributeSet::GetStaminaRegenRateAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	RegenMagnitude.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
	RegenMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo RegenModifier;
	RegenModifier.Attribute = UAccidentalHeroAttributeSet::GetStaminaAttribute();
	RegenModifier.ModifierOp = EGameplayModOp::Additive;
	RegenModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenMagnitude);
	Modifiers.Add(RegenModifier);
}
