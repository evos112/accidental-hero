// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Starvation.generated.h"

/**
 * Health drain applied while Hunger is empty, and removed once the player eats.
 *
 * Infinite rather than fixed-duration, so the condition lasts exactly as long as the empty stomach
 * does. Applied and removed from UAccidentalHeroAttributeSet::PostGameplayEffectExecute, which
 * already runs every time the survival drain ticks; that is also where State.Starving is added as
 * a loose tag for UI and ability queries.
 */
UCLASS()
class ACCIDENTALHERO_API UGE_Starvation : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Starvation();
};

/** Health drain applied while Thirst is empty. Bites harder than starvation — you last days
 *  without food and only a day or two without water. */
UCLASS()
class ACCIDENTALHERO_API UGE_Dehydration : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Dehydration();
};
