// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_SurvivalDrain.generated.h"

/**
 * Infinite periodic drain on Hunger and Thirst — the clock that makes food and water matter.
 *
 * Mirrors UGE_StaminaRegen's shape (infinite duration, 1s period) but with fixed negative
 * magnitudes rather than an attribute-backed rate, because there is no HungerDrainRate attribute
 * to capture. Thirst falls faster than hunger, the usual survival-game balance.
 *
 * PreAttributeChange clamps both at 0, so this parks at empty rather than going negative; the
 * consequences of being at 0 (health decay, movement penalty) are deliberately not modelled here.
 */
UCLASS()
class ACCIDENTALHERO_API UGE_SurvivalDrain : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_SurvivalDrain();
};
