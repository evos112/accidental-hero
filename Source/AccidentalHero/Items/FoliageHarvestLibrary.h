// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FoliageHarvestLibrary.generated.h"

class AResourceNode;
class UFoliageHarvestSet;
struct FHitResult;

/**
 * Turns a struck foliage instance into a real harvestable actor.
 *
 * Foliage instances live inside an InstancedStaticMeshComponent and have no actor of their own,
 * so a sweep hit on one reports the AInstancedFoliageActor with the instance index in
 * FHitResult::Item. Converting means: read that instance's world transform, delete the instance,
 * and spawn the mapped node there. From that point the normal AResourceNode flow (chop damage,
 * falling trunk, respawn) applies unchanged.
 */
UCLASS()
class ACCIDENTALHERO_API UFoliageHarvestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * If Hit landed on a harvestable foliage instance, remove that instance and spawn its node.
	 * Server-only; returns the spawned node, or null when the hit wasn't convertible.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage Harvest")
	static AResourceNode* ConvertFoliageHitToNode(const FHitResult& Hit, const UFoliageHarvestSet* HarvestSet);

	/**
	 * Converts the harvestable foliage instance nearest to where the player is looking, without
	 * needing a trace hit.
	 *
	 * Ground cover (grass, shrubs, berry bushes) deliberately has no collision — you should walk
	 * through grass, not bump into it — so a sweep can never find it. This searches the instance
	 * trees directly instead: instances within Range of Origin, scored by how close they are to the
	 * aim ray, best one converted. Cost is bounded by Range rather than by how much foliage the
	 * world contains, so it stays cheap in a 216k-instance level.
	 *
	 * Server-only; returns the spawned node, or null when nothing harvestable is in reach.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage Harvest", meta = (WorldContext = "WorldContextObject"))
	static AResourceNode* HarvestNearestFoliage(UObject* WorldContextObject, const FVector& Origin,
		const FVector& AimDirection, float Range, const UFoliageHarvestSet* HarvestSet);
};
