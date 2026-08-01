// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterSource.generated.h"

class UStaticMeshComponent;
class AAccidentalHeroCharacter;

/**
 * Somewhere you can drink: a pond, a stream, or a well you built yourself.
 *
 * The world ships with no water at all, so every drinkable spot is one of these. Natural water and
 * a crafted well are the same actor with a different mesh and a different way of arriving — a pond
 * is placed in the level, a well is built from the inventory.
 *
 * Drinking is free and unlimited; the cost is having to be here. Carrying water away is what the
 * waterskin is for, and refilling it happens through the same interaction.
 */
UCLASS()
class ACCIDENTALHERO_API AWaterSource : public AActor
{
	GENERATED_BODY()

public:
	AWaterSource();

	/** True when Player is close enough to reach the water. */
	UFUNCTION(BlueprintPure, Category = "Water")
	bool IsInRange(const AAccidentalHeroCharacter* Player) const;

	/** Fills the player's thirst and tops up any refillable containers they carry.
	 *  Returns false when out of range, or when there was nothing to gain. */
	UFUNCTION(BlueprintCallable, Category = "Water")
	bool Drink(AAccidentalHeroCharacter* Player);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Water")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Reach from the actor's origin. Generous, because the surface is wide and the player should
	 *  not have to hunt for the exact spot. */
	UPROPERTY(EditAnywhere, Category = "Water", meta = (ClampMin = "0.0"))
	float DrinkRadius = 400.0f;
};
