// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_ManaRegen.h"
#include "AccidentalHeroAttributeSet.h"

UGE_ManaRegen::UGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = false;

	FAttributeBasedFloat RegenMagnitude;
	RegenMagnitude.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UAccidentalHeroAttributeSet::GetManaRegenRateAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	RegenMagnitude.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
	RegenMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo RegenModifier;
	RegenModifier.Attribute = UAccidentalHeroAttributeSet::GetManaAttribute();
	RegenModifier.ModifierOp = EGameplayModOp::Additive;
	RegenModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenMagnitude);
	Modifiers.Add(RegenModifier);
}
