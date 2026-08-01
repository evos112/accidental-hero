// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BiomeSubsystem.generated.h"

class ABiomeRegion;
class UBiomeDefinition;

/**
 * Answers "which biome is this point in?".
 *
 * Regions register themselves on BeginPlay rather than being gathered up front, because the world
 * is World Partition — an actor sweep at startup would miss every region that hasn't streamed in
 * yet, and would find them in a different order each run.
 */
UCLASS()
class ACCIDENTALHERO_API UBiomeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterRegion(ABiomeRegion* Region);
	void UnregisterRegion(ABiomeRegion* Region);

	/** Biome at a world location, or null out in the ordinary world. Where regions overlap the
	 *  smallest one wins, so a pocket biome can sit inside a larger one and still take priority. */
	UFUNCTION(BlueprintPure, Category = "Biome")
	UBiomeDefinition* GetBiomeAt(const FVector& WorldLocation) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<ABiomeRegion>> Regions;
};
