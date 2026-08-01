// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_RangedAttack.h"
#include "AccidentalHeroGameplayTags.h"
#include "GE_RangedDamage.h"

UGA_RangedAttack::UGA_RangedAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(AccidentalHeroGameplayTags::Ability_RangedAttack));

	DamageEffect = UGE_RangedDamage::StaticClass();
	DamageDataTag = AccidentalHeroGameplayTags::Data_RangedDamage;

	AttackDamage = 20.0f;
	AttackRange = 2000.0f;
	AttackRadius = 50.0f;
}
