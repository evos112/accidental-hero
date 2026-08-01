// Copyright Epic Games, Inc. All Rights Reserved.

#include "FurnaceInventoryComponent.h"
#include "Net/UnrealNetwork.h"

UFurnaceInventoryComponent::UFurnaceInventoryComponent()
{
	MaxSlots = 6;
}

void UFurnaceInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Deliberately skips UInventoryComponent::GetLifetimeReplicatedProps (which registers
	// InventoryList as COND_OwnerOnly) and instead goes straight to its parent, re-registering
	// InventoryList with no condition — furnace contents should be visible to every nearby
	// player, not just an owning connection (a furnace generally has no single "owner" anyway).
	UActorComponent::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFurnaceInventoryComponent, InventoryList);
}
