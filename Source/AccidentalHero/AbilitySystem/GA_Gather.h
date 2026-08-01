// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Gather.generated.h"

class UFoliageHarvestSet;

/**
 * Instant forward sphere-sweep on activation
 * but sweeping ECC_WorldStatic (resource nodes are static props, not pawns) and calling
 * AResourceNode::Harvest() on the first hit instead of applying a GameplayEffect — resource nodes
 * don't have their own AbilitySystemComponent, so there's no ASC-to-ASC damage step here.
 */
UCLASS()
class ACCIDENTALHERO_API UGA_Gather : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Gather();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Gather")
	float GatherRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Gather")
	float GatherRadius = 75.0f;

	/** Maps scattered foliage meshes (trees, boulders) to the node spawned when one is struck.
	 *  Without this the world's foliage is scenery — only hand-placed AResourceNode actors are
	 *  harvestable. Point at DA_FoliageHarvest. */
	UPROPERTY(EditDefaultsOnly, Category = "Gather")
	TObjectPtr<UFoliageHarvestSet> FoliageHarvestSet;
};
