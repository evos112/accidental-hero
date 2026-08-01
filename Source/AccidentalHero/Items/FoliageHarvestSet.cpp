// Copyright Epic Games, Inc. All Rights Reserved.

#include "FoliageHarvestSet.h"

TSubclassOf<AResourceNode> UFoliageHarvestSet::FindNodeClassForMesh(UStaticMesh* Mesh) const
{
	if (!Mesh)
	{
		return nullptr;
	}

	for (const FFoliageHarvestRule& Rule : Rules)
	{
		if (Rule.FoliageMesh == Mesh)
		{
			return Rule.NodeClass;
		}
	}
	return nullptr;
}
