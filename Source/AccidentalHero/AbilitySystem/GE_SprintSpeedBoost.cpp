// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_SprintSpeedBoost.h"
#include "AccidentalHeroAttributeSet.h"

UGE_SprintSpeedBoost::UGE_SprintSpeedBoost()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo SpeedModifier;
	SpeedModifier.Attribute = UAccidentalHeroAttributeSet::GetMoveSpeedAttribute();
	SpeedModifier.ModifierOp = EGameplayModOp::Additive;
	// Takes the 340 uu/s jog to 600 uu/s (21.6 km/h) — a genuine sprint a person can hold briefly,
	// which is what the stamina drain is there to limit.
	SpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(260.0f));
	Modifiers.Add(SpeedModifier);
}
