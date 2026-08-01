// Copyright Epic Games, Inc. All Rights Reserved.

#include "BiomeSubsystem.h"

#include "BiomeDefinition.h"
#include "BiomeRegion.h"

void UBiomeSubsystem::RegisterRegion(ABiomeRegion* Region)
{
	if (Region)
	{
		Regions.AddUnique(Region);
	}
}

void UBiomeSubsystem::UnregisterRegion(ABiomeRegion* Region)
{
	Regions.Remove(Region);
}

UBiomeDefinition* UBiomeSubsystem::GetBiomeAt(const FVector& WorldLocation) const
{
	UBiomeDefinition* Best = nullptr;
	float BestRadius = TNumericLimits<float>::Max();

	for (const TObjectPtr<ABiomeRegion>& Region : Regions)
	{
		if (!Region || !Region->Biome || !Region->ContainsLocation(WorldLocation))
		{
			continue;
		}

		// Smallest containing region wins, so a pocket inside a larger biome takes priority.
		if (Region->GetRadius() < BestRadius)
		{
			BestRadius = Region->GetRadius();
			Best = Region->Biome;
		}
	}

	return Best;
}
