// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GoalDefinition.generated.h"

class UItemDefinition;

/**
 * One line on the checklist that ends the game (SPEC 5.7).
 *
 * Every goal is "obtain N of this item", which sounds narrow but covers the whole ladder: crafting a
 * stone axe, smelting an iron ingot, building a farm plot and harvesting a crop all end with an item
 * arriving in the pack. That keeps the completion test to a single rule with a single place to get
 * wrong, instead of a condition system that would need a new branch per goal type.
 *
 * Completion is sticky — see UGoalSubsystem. Spending the item afterwards must not un-tick the line,
 * or the checklist would punish players for using what they made.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API UGoalDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable identity written into the save. Renaming the asset is safe; changing this is not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
	FName GoalId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
	FText DisplayName;

	/** Shown under the line — say where to go or what to use, not what to click. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal", meta = (MultiLine = "true"))
	FText Hint;

	/** Lower sorts first, so the checklist reads in the order the game expects you to do it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
	int32 SortOrder = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal|Condition")
	TObjectPtr<UItemDefinition> RequiredItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal|Condition", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UObject Interface
};
