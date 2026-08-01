// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AccidentalHeroPlayerState.generated.h"

class UAbilitySystemComponent;
class UAccidentalHeroAttributeSet;
class UInventoryComponent;
class UCraftingComponent;

UCLASS()
class ACCIDENTALHERO_API AAccidentalHeroPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAccidentalHeroPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAccidentalHeroAttributeSet* GetAttributeSet() const { return AttributeSet; }
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	UCraftingComponent* GetCraftingComponent() const { return CraftingComponent; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAccidentalHeroAttributeSet> AttributeSet;

	/** Lives here (not on the Character) so items survive Character destroy/respawn. */
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, Category = "Crafting")
	TObjectPtr<UCraftingComponent> CraftingComponent;
};
