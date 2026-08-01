// Copyright Epic Games, Inc. All Rights Reserved.

#include "GE_Starvation.h"
#include "AccidentalHeroAttributeSet.h"

namespace
{
	/** Both conditions are the same shape: periodic health loss for as long as the bar is empty.
	 *
	 *  Deliberately no UTargetTagsGameplayEffectComponent here — FindOrAddComponent calls NewObject,
	 *  which is illegal inside a UObject constructor and fatals on startup. The owning state tag is
	 *  applied as a loose tag by UAccidentalHeroAttributeSet instead, which also tracks the handle
	 *  so it can remove exactly this effect. */
	void BuildDrain(UGameplayEffect& Effect, float HealthPerSecond)
	{
		Effect.DurationPolicy = EGameplayEffectDurationType::Infinite;
		Effect.Period = FScalableFloat(1.0f);
		Effect.bExecutePeriodicEffectOnApplication = false;

		FGameplayModifierInfo HealthModifier;
		HealthModifier.Attribute = UAccidentalHeroAttributeSet::GetHealthAttribute();
		HealthModifier.ModifierOp = EGameplayModOp::Additive;
		HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-HealthPerSecond));
		Effect.Modifiers.Add(HealthModifier);
	}
}

UGE_Starvation::UGE_Starvation()
{
	// 0.5 hp/s: a full health bar takes ~3.3 minutes to burn off, which is time to find food.
	BuildDrain(*this, 0.5f);
}

UGE_Dehydration::UGE_Dehydration()
{
	BuildDrain(*this, 0.8f);
}
