// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResourceNode.h"
#include "ForageNode.generated.h"

class UItemDefinition;

/**
 * Hand-gathered ground cover — tall grass, scrub, herb clumps.
 *
 * The bottom rung of the resource ladder: no tool, one pull, and it is gone. Yields a bulk fibre
 * item every time plus an occasional bonus (berries), which is what makes wandering through grass
 * worth doing rather than something you walk past.
 *
 * Unlike ATreeNode/ARockNode there is no tool tier here — a hatchet doesn't help you pull grass,
 * so GetStrikePower is left at the base 1 and MaxHits is 1.
 */
UCLASS()
class ACCIDENTALHERO_API AForageNode : public AResourceNode
{
	GENERATED_BODY()

public:
	AForageNode();

	virtual bool Harvest(AAccidentalHeroCharacter* Player) override;

protected:
	/** Always dropped — the reason to pull grass at all. Set to DA_Item_Fiber. */
	UPROPERTY(EditAnywhere, Category = "Forage")
	TObjectPtr<UItemDefinition> FibreItem;

	UPROPERTY(EditAnywhere, Category = "Forage", meta = (ClampMin = "1"))
	int32 FibreYieldMin = 1;

	UPROPERTY(EditAnywhere, Category = "Forage", meta = (ClampMin = "1"))
	int32 FibreYieldMax = 3;

	/** Occasional bonus find. Set to DA_Item_Berries. */
	UPROPERTY(EditAnywhere, Category = "Forage")
	TObjectPtr<UItemDefinition> BonusItem;

	UPROPERTY(EditAnywhere, Category = "Forage", meta = (ClampMin = "1"))
	int32 BonusYield = 1;

	/** Chance (0-1) the bonus item is found. Low enough that berries stay a small event. */
	UPROPERTY(EditAnywhere, Category = "Forage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BonusChance = 0.25f;

	/** Extra fibre per knife tier — a knife cuts grass instead of tearing it. */
	UPROPERTY(EditAnywhere, Category = "Forage", meta = (ClampMin = "0"))
	int32 KnifeBonusPerTier = 2;
};
