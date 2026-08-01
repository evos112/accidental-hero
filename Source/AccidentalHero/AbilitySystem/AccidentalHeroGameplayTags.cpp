// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroGameplayTags.h"

namespace AccidentalHeroGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Sprint, "Ability.Sprint", "Grants the sprint movement speed boost while active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "Applied to the owner while the sprint ability is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "Identifies the melee attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Attack, "Cooldown.Attack", "Applied while the melee attack ability is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller tag used to pass damage magnitude into GE_MeleeDamage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_RangedAttack, "Ability.RangedAttack", "Identifies the ranged attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_RangedDamage, "Data.RangedDamage", "SetByCaller tag used to pass ranged damage magnitude into GE_RangedDamage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_MagicBolt, "Ability.MagicBolt", "Identifies the magic bolt spell ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_MagicDamage, "Data.MagicDamage", "SetByCaller tag used to pass magic damage magnitude into GE_MagicDamage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_MagicBolt, "Cooldown.MagicBolt", "Applied while the magic bolt ability is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Gather, "Ability.Gather", "Identifies the resource-gathering ability (chopping/mining AResourceNode actors).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Gather, "Cooldown.Gather", "Applied while the gather ability is on cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Starving, "State.Starving", "Granted while Hunger is empty; drives the starvation health drain.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dehydrated, "State.Dehydrated", "Granted while Thirst is empty; drives the dehydration health drain.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Ore, "Item.Category.Ore", "Raw mined ore; smelting recipes will key off this tag.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Ingot, "Item.Category.Ingot", "Smelted metal ingot; crafting recipe input/output.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_BuildingMaterial, "Item.Category.BuildingMaterial", "Generic construction material; building system input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Tool, "Item.Category.Tool", "Tools used by crafting/gathering actions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Weapon, "Item.Category.Weapon", "Equippable weapon items.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool_Hatchet, "Item.Tool.Hatchet", "Chopping tool. Mining a rock node with a hatchet favours bulk stone over secondary minerals.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool_Pickaxe, "Item.Tool.Pickaxe", "Mining tool. Yields less raw stone than a hatchet but is the only way to recover ore/flint from a rock node.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool_Axe, "Item.Tool.Axe", "Felling tool. Chops trees far faster than bare hands; tier decides how many swings.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool_Knife, "Item.Tool.Knife", "Cutting tool. Improves fibre yield from grass and is required for skinning.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Tool_Hammer, "Item.Tool.Hammer", "Construction tool. Reserved for the building system.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Consumable, "Item.Category.Consumable", "Consumed on use (future potions/food).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Fuel, "Item.Category.Fuel", "Burnable fuel consumed as a smelting-recipe ingredient row (e.g. Coal); descriptive metadata only, not read by any matching logic.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Recipe_Category_Crafting, "Recipe.Category.Crafting", "Recipes that combine inventory items via direct crafting (not smelting/building).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Recipe_Category_Smelting, "Recipe.Category.Smelting", "Recipes that require a furnace station and run over ProcessDuration seconds, instead of resolving instantly like Recipe.Category.Crafting.");
}
