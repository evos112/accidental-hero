// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_BiomeDrain.generated.h"

/**
 * The survival cost of standing in a biome, stacked on top of UGE_SurvivalDrain rather than
 * replacing it — so the baseline clock stays in one place and a biome only ever says "and also".
 *
 * One effect class serves every biome: the per-second amounts arrive as SetByCaller magnitudes
 * from UBiomeDefinition, so adding a biome is a data asset, not another UGameplayEffect subclass.
 * Callers must set both magnitudes; an unset SetByCaller resolves to 0 and logs.
 */
UCLASS()
class ACCIDENTALHERO_API UGE_BiomeDrain : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_BiomeDrain();
};
