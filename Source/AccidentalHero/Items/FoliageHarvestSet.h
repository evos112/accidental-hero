// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FoliageHarvestSet.generated.h"

class UStaticMesh;
class AResourceNode;

/** One "this foliage mesh is harvestable as this node" rule. */
USTRUCT(BlueprintType)
struct ACCIDENTALHERO_API FFoliageHarvestRule
{
	GENERATED_BODY()

	/** The mesh used by the foliage type, e.g. pine_tree_full_a or rock_a. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage Harvest")
	TObjectPtr<UStaticMesh> FoliageMesh = nullptr;

	/** Node spawned in its place when struck — ATreeNode, ARockNode, ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage Harvest")
	TSubclassOf<AResourceNode> NodeClass;
};

/**
 * Maps scattered foliage meshes to the harvestable node that replaces them.
 *
 * The world's trees and rocks are foliage instances, not actors — there is nothing for
 * UGA_Gather to call Harvest() on. Rather than hand-place hundreds of thousands of actors, a
 * struck instance is removed and a single node actor is spawned at its exact transform. Only the
 * instance actually being harvested ever becomes an actor, so the cost stays flat no matter how
 * dense the forest is.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API UFoliageHarvestSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage Harvest")
	TArray<FFoliageHarvestRule> Rules;

	/** Node class registered for Mesh, or null if that mesh isn't harvestable. */
	UFUNCTION(BlueprintPure, Category = "Foliage Harvest")
	TSubclassOf<AResourceNode> FindNodeClassForMesh(UStaticMesh* Mesh) const;
};
