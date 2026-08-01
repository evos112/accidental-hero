// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoalDefinition.h"

FPrimaryAssetId UGoalDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Goal"), GoalId);
}
