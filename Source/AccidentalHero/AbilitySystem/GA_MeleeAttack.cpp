// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_MeleeAttack.h"
#include "AccidentalHeroGameplayTags.h"
#include "GE_MeleeDamage.h"
#include "GE_AttackCooldown.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(AccidentalHeroGameplayTags::Ability_Attack));

	DamageEffect = UGE_MeleeDamage::StaticClass();
	DamageDataTag = AccidentalHeroGameplayTags::Data_Damage;
	CooldownGameplayEffectClass = UGE_AttackCooldown::StaticClass();
}
