// Copyright Epic Games, Inc. All Rights Reserved.

#include "RockNode.h"
#include "ItemDefinition.h"
#include "ItemPickup.h"
#include "InventoryComponent.h"
#include "InventoryTypes.h"
#include "AccidentalHeroCharacter.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "Components/StaticMeshComponent.h"

ARockNode::ARockNode()
{
	// Rocks take more work than a tree; the base default of 3 makes a boulder pop in one swing.
	MaxHits = 8;
}

bool ARockNode::PlayerHasTool(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const
{
	if (!Player || !ToolTag.IsValid())
	{
		return false;
	}

	const UInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	// The equipped item only: carrying a pick is not the same as swinging one, and the yield rule
	// below is what makes choosing between hatchet and pick a decision rather than a formality.
	return Inventory->GetEquippedToolTier(ToolTag) > 0;
}

int32 ARockNode::GetStrikePower(AAccidentalHeroCharacter* Player) const
{
	// Either tool breaks rock; the better one sets the pace.
	const int32 Tier = FMath::Max(
		GetEquippedToolTier(Player, AccidentalHeroGameplayTags::Item_Tool_Pickaxe),
		GetEquippedToolTier(Player, AccidentalHeroGameplayTags::Item_Tool_Axe));

	return Tier > 0 ? Tier * ToolStrikePower : 1;
}

bool ARockNode::Harvest(AAccidentalHeroCharacter* Player)
{
	// Yield here is tool-dependent, so this replaces the base's fixed YieldPerHit drop rather
	// than calling Super::Harvest. Depletion/respawn still runs through the base helpers.
	if (!HasAuthority() || IsDepleted() || !Player || !IsInRange(Player))
	{
		return false;
	}

	const bool bHasPickaxe = PlayerHasTool(Player, AccidentalHeroGameplayTags::Item_Tool_Pickaxe);
	// An axe smashes bulk stone out of a rock; only a pick recovers the mineral.
	const bool bHasHatchet = PlayerHasTool(Player, AccidentalHeroGameplayTags::Item_Tool_Axe);

	int32 StoneAmount = BareHandStoneYield;
	if (bHasHatchet)
	{
		// Hatchet favours bulk stone — the reason to carry one alongside a pick.
		StoneAmount = HatchetStoneYield;
	}
	else if (bHasPickaxe)
	{
		StoneAmount = PickaxeStoneYield;
	}

	const FTransform DropAt(GetActorLocation() + FVector(0.0f, 0.0f, 20.0f));

	if (StoneItem && StoneAmount > 0)
	{
		AItemPickup::SpawnItemPickup(this, StoneItem, StoneAmount, DropAt);
	}

	// Only a pickaxe recovers the secondary mineral.
	if (bHasPickaxe && MineralItem && FMath::FRand() <= PickaxeMineralChance)
	{
		AItemPickup::SpawnItemPickup(this, MineralItem, PickaxeMineralYield, DropAt);
	}

	// The tool that set the stone yield is the one doing the smashing, so that's the one that wears.
	// Carrying both means the axe wears first; the pickaxe takes over once it breaks.
	ConsumeToolDurability(Player, bHasHatchet
		? AccidentalHeroGameplayTags::Item_Tool_Axe
		: AccidentalHeroGameplayTags::Item_Tool_Pickaxe);

	HitsRemaining -= FMath::Max(1, GetStrikePower(Player));
	if (HitsRemaining <= 0)
	{
		Deplete();
	}
	return true;
}
