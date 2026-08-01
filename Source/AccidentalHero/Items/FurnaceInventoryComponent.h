// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"
#include "FurnaceInventoryComponent.generated.h"

/**
 * An UInventoryComponent variant for world stations (furnaces): replicates to every nearby
 * client, not just an owning connection, since a furnace's contents are meant to be visible to
 * any player standing near it. Smaller MaxSlots than a player backpack — a furnace is a small
 * hopper (ore/fuel in, output out), not a second inventory.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class ACCIDENTALHERO_API UFurnaceInventoryComponent : public UInventoryComponent
{
	GENERATED_BODY()

public:
	UFurnaceInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
