// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryTypes.h"
#include "InventoryComponent.h"

void FInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	BroadcastChanged();
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	BroadcastChanged();
}

void FInventoryList::PostReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	BroadcastChanged();
}

void FInventoryList::BroadcastChanged() const
{
	if (OwnerComponent)
	{
		OwnerComponent->NotifyInventoryChanged();
	}
}
