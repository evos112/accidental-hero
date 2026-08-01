// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDefinition.generated.h"

class UGA_WeaponAttackBase;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Melee,
	Ranged,
	Magic
};

/**
 * Data-only description of an equippable weapon: which attack ability the Attack button should
 * fire while it's equipped, and the damage/range/radius that ability should use instead of its
 * own class defaults. See AAccidentalHeroCharacter::EquipWeapon.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API UWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGA_WeaponAttackBase> AttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float Damage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float Range = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float Radius = 75.0f;
};
