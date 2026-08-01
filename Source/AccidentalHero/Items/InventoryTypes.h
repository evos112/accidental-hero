// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryTypes.generated.h"

class UInventoryComponent;
class UItemDefinition;

/** A single replicated inventory slot: an item reference plus its stack count. */
USTRUCT(BlueprintType)
struct ACCIDENTALHERO_API FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemDefinition> ItemDef = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 StackCount = 0;

	/** Uses remaining on this specific tool. Ignored entirely when ItemDef->MaxDurability is 0.
	 *  Per-entry rather than per-item-type because two axes wear independently — which works only
	 *  because damageable items never share an entry (see UInventoryComponent::IsDamageable). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Durability = 0;
};

/** Replicated list of inventory slots. Owned by UInventoryComponent. */
USTRUCT()
struct ACCIDENTALHERO_API FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryItemEntry> Items;

	/** Not replicated; set by the owning component so replication callbacks can notify it. */
	UPROPERTY(NotReplicated)
	TObjectPtr<UInventoryComponent> OwnerComponent = nullptr;

	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FInventoryItemEntry, FInventoryList>(Items, DeltaParms, *this);
	}

private:
	void BroadcastChanged() const;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};
