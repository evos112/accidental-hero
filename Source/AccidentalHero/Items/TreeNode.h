// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResourceNode.h"
#include "TreeNode.generated.h"

class UItemDefinition;

/**
 * A choppable tree. Extends AResourceNode with the three things a survival tree needs beyond a
 * generic node:
 *
 *  - Tool progression. Bare hands barely scratch it; a stone axe is better; an iron axe fells it
 *    in a few swings. Tier is read from what the player is carrying (see GetStrikePower) rather
 *    than an equipment slot, because the project has no equip system yet.
 *  - Falling physics. On depletion the standing mesh is swapped for a simulating trunk that is
 *    kicked away from whoever chopped it, so it topples and rolls instead of vanishing.
 *  - Regrowth. A sapling is left behind and the node respawns on the base class's timer, so
 *    clear-cutting an area leaves it visibly bare until it grows back.
 */
UCLASS()
class ACCIDENTALHERO_API ATreeNode : public AResourceNode
{
	GENERATED_BODY()

public:
	ATreeNode();

	/** Records who swung (so the trunk falls away from them), then defers to the base. */
	virtual bool Harvest(AAccidentalHeroCharacter* Player) override;

protected:
	virtual int32 GetStrikePower(AAccidentalHeroCharacter* Player) const override;
	virtual void Deplete() override;

	/** Trunk that simulates once the tree is felled. Hidden and collisionless while standing. */
	UPROPERTY(VisibleAnywhere, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	/** Left in the ground after felling so a cleared area reads as stumps, not empty ground. */
	UPROPERTY(VisibleAnywhere, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> SaplingMesh;

	/** Hits removed per swing per axe tier — stone (tier 1) gives 2, iron (tier 2) gives 4.
	 *  Any new axe is data: tag it Item.Tool.Axe and set its ToolTier. */
	UPROPERTY(EditAnywhere, Category = "Tree|Tools", meta = (ClampMin = "1"))
	int32 PowerPerAxeTier = 2;

	/** Bare hands. Deliberately 1 so an unarmed player can still fell a tree, just slowly. */
	UPROPERTY(EditAnywhere, Category = "Tree|Tools", meta = (ClampMin = "1"))
	int32 BareHandPower = 1;

	/** Sideways kick applied to the trunk when it falls, away from the chopper. */
	UPROPERTY(EditAnywhere, Category = "Tree|Physics")
	float FallImpulse = 42000.0f;

	/** Damage dealt to a character the falling trunk lands on. */
	UPROPERTY(EditAnywhere, Category = "Tree|Physics")
	float CrushDamage = 35.0f;

	/** Impact speed below which the trunk is considered settled and stops hurting anyone. */
	UPROPERTY(EditAnywhere, Category = "Tree|Physics")
	float CrushMinSpeed = 180.0f;

	/** Seconds the felled trunk lies around before it is cleaned up. */
	UPROPERTY(EditAnywhere, Category = "Tree|Physics")
	float TrunkLifetime = 30.0f;

	UFUNCTION()
	void OnTrunkHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** Restores the standing tree and hides the sapling/trunk again. */
	virtual void Respawn();

	void ResetTrunk();

private:
	/** Who swung last — the trunk falls away from them. */
	TWeakObjectPtr<AAccidentalHeroCharacter> LastChopper;

	FTimerHandle TrunkCleanupHandle;
};
