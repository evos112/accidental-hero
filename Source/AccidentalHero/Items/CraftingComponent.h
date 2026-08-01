// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UInventoryComponent;
class URecipeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingResult, URecipeDefinition*, Recipe, bool, bSuccess);

/**
 * Server-authoritative crafting transaction against the sibling UInventoryComponent on the same
 * PlayerState. Not a GAS ability: crafting is a plain inventory transaction (check -> remove ->
 * add), the same shape as UInventoryComponent's own mutators, not a spendable-resource combat
 * ability like Sprint/Attack. Lives on AAccidentalHeroPlayerState for the same reason
 * InventoryComponent does. See AccidentalHeroPlayerState::GetCraftingComponent().
 */
UCLASS(ClassGroup = (Crafting), meta = (BlueprintSpawnableComponent))
class ACCIDENTALHERO_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingComponent();

	/** Whether every ingredient (aggregated across any duplicate rows) is present and the
	 *  output would fit. Safe to call from anywhere; does not mutate the inventory. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crafting")
	bool CanCraft(URecipeDefinition* Recipe) const;

	/** Authoritative mutator — only call where HasAuthority() is true. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryCraft(URecipeDefinition* Recipe);

	/** Client-callable wrapper for player-initiated crafting. */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Crafting")
	void Server_TryCraft(URecipeDefinition* Recipe);

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingResult OnCraftingResult;

protected:
	virtual void BeginPlay() override;

private:
	UInventoryComponent* GetInventoryComponent() const;

	UPROPERTY(Transient)
	mutable TObjectPtr<UInventoryComponent> CachedInventoryComponent;
};
