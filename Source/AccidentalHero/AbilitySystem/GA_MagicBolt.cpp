// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_MagicBolt.h"
#include "AccidentalHeroGameplayTags.h"
#include "GE_MagicDamage.h"
#include "GE_ManaCost.h"
#include "GE_MagicCooldown.h"

UGA_MagicBolt::UGA_MagicBolt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(AccidentalHeroGameplayTags::Ability_MagicBolt));

	DamageEffect = UGE_MagicDamage::StaticClass();
	DamageDataTag = AccidentalHeroGameplayTags::Data_MagicDamage;
	CostGameplayEffectClass = UGE_ManaCost::StaticClass();
	CooldownGameplayEffectClass = UGE_MagicCooldown::StaticClass();

	AttackDamage = 30.0f;
	AttackRange = 2500.0f;
	AttackRadius = 60.0f;
}
