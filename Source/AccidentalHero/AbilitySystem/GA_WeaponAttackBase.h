// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GA_WeaponAttackBase.generated.h"

class UWeaponDefinition;

/**
 * Shared implementation for the melee/ranged/magic attack abilities: sphere-sweep from the avatar
 * and apply SetByCaller damage to whatever's hit. Subclasses just configure DamageEffect,
 * DamageDataTag, and default stats in their constructor.
 */
UCLASS(Abstract)
class ACCIDENTALHERO_API UGA_WeaponAttackBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Overrides this instance's damage/range/radius from an equipped weapon's stats. */
	virtual void ConfigureFromWeapon(const UWeaponDefinition* Weapon);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** SetByCaller tag the damage magnitude is passed through as; set by each subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag DamageDataTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackDamage = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRadius = 75.0f;
};
