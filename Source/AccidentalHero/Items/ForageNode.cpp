// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForageNode.h"
#include "ItemDefinition.h"
#include "ItemPickup.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "AccidentalHeroCharacter.h"

AForageNode::AForageNode()
{
	// One pull and the clump is gone.
	MaxHits = 1;
	RespawnSeconds = 120.0f;
}

bool AForageNode::Harvest(AAccidentalHeroCharacter* Player)
{
	// Yield is randomised and can include a bonus drop, so this replaces the base's fixed
	// OutputItem x YieldPerHit rather than calling Super::Harvest. Depletion still uses the base.
	if (!HasAuthority() || IsDepleted() || !Player || !IsInRange(Player))
	{
		return false;
	}

	const FTransform DropAt(GetActorLocation() + FVector(0.0f, 0.0f, 20.0f));

	if (FibreItem)
	{
		int32 Amount = FMath::RandRange(FMath::Min(FibreYieldMin, FibreYieldMax),
			FMath::Max(FibreYieldMin, FibreYieldMax));

		// A knife cuts grass rather than tearing it — the reason to carry one early.
		const int32 KnifeTier = GetEquippedToolTier(Player, AccidentalHeroGameplayTags::Item_Tool_Knife);
		if (KnifeTier > 0)
		{
			Amount += KnifeTier * KnifeBonusPerTier;
			// Only wears when the knife actually did something — tearing grass by hand costs nothing.
			ConsumeToolDurability(Player, AccidentalHeroGameplayTags::Item_Tool_Knife);
		}
		AItemPickup::SpawnItemPickup(this, FibreItem, Amount, DropAt);
	}

	if (BonusItem && FMath::FRand() <= BonusChance)
	{
		AItemPickup::SpawnItemPickup(this, BonusItem, BonusYield, DropAt);
	}

	HitsRemaining -= FMath::Max(1, GetStrikePower(Player));
	if (HitsRemaining <= 0)
	{
		Deplete();
	}
	return true;
}
