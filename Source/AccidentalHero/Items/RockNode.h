// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ResourceNode.h"
#include "RockNode.generated.h"

class UItemDefinition;

/**
 * A mineable boulder. Extends AResourceNode with the tool-choice rule survival games use for
 * stone: a hatchet smashes the rock apart for bulk stone, a pickaxe works it more carefully and
 * is the only way to recover the secondary mineral (ore/flint). Bare hands get a token amount,
 * so a fresh spawn is never hard-blocked — you can always scrape enough stone for a first tool.
 *
 * Tool tier is read from what the player is carrying, by gameplay tag, so any future item tagged
 * Item.Tool.Pickaxe works without touching this class.
 */
UCLASS()
class ACCIDENTALHERO_API ARockNode : public AResourceNode
{
	GENERATED_BODY()

public:
	ARockNode();

	virtual bool Harvest(AAccidentalHeroCharacter* Player) override;

protected:
	virtual int32 GetStrikePower(AAccidentalHeroCharacter* Player) const override;

	/** True if the player carries any item tagged with ToolTag. */
	bool PlayerHasTool(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const;

	/** Bulk material — Stone. */
	UPROPERTY(EditAnywhere, Category = "Rock|Yield")
	TObjectPtr<UItemDefinition> StoneItem;

	/** Pickaxe-only drop — the project's stand-in for flint. Set to DA_Item_IronOre or Coal. */
	UPROPERTY(EditAnywhere, Category = "Rock|Yield")
	TObjectPtr<UItemDefinition> MineralItem;

	UPROPERTY(EditAnywhere, Category = "Rock|Yield", meta = (ClampMin = "1"))
	int32 HatchetStoneYield = 4;

	UPROPERTY(EditAnywhere, Category = "Rock|Yield", meta = (ClampMin = "1"))
	int32 PickaxeStoneYield = 2;

	UPROPERTY(EditAnywhere, Category = "Rock|Yield", meta = (ClampMin = "1"))
	int32 BareHandStoneYield = 1;

	/** Mineral units dropped per pickaxe strike. */
	UPROPERTY(EditAnywhere, Category = "Rock|Yield", meta = (ClampMin = "1"))
	int32 PickaxeMineralYield = 1;

	/** Chance (0-1) a pickaxe strike also yields MineralItem. */
	UPROPERTY(EditAnywhere, Category = "Rock|Yield", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PickaxeMineralChance = 0.5f;

	/** Strikes-per-hit with a tool. Bare hands stay at 1 so mining by hand is slow but possible. */
	UPROPERTY(EditAnywhere, Category = "Rock|Tools", meta = (ClampMin = "1"))
	int32 ToolStrikePower = 2;
};
