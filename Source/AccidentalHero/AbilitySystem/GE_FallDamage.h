// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_FallDamage.generated.h"

/** Instant health loss from a hard landing. Magnitude is set by caller (Data.Damage) from the
 *  impact speed — see AAccidentalHeroCharacter::Landed. */
UCLASS()
class ACCIDENTALHERO_API UGE_FallDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_FallDamage();
};
