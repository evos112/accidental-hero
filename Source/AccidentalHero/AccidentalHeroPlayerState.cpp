// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AccidentalHeroAttributeSet.h"
#include "Items/InventoryComponent.h"
#include "Items/CraftingComponent.h"

AAccidentalHeroPlayerState::AAccidentalHeroPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UAccidentalHeroAttributeSet>(TEXT("AttributeSet"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));

	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AAccidentalHeroPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
