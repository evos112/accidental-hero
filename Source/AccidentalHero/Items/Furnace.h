// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Furnace.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UFurnaceInventoryComponent;
class URecipeDefinition;
class UItemDefinition;
class AAccidentalHeroCharacter;

/**
 * World-placeable smelting station. Owns its own small inventory (ore/fuel deposited, output
 * withdrawn) and runs recipes tagged Recipe.Category.Smelting over their ProcessDuration via a
 * timer, unlike UCraftingComponent's instant crafting. Not a PlayerState component — a furnace
 * doesn't belong to one player and needs its own timer/inventory, which doesn't fit
 * UCraftingComponent's "instant, against an already-known inventory" shape.
 */
UCLASS()
class ACCIDENTALHERO_API AFurnace : public AActor
{
	GENERATED_BODY()

public:
	AFurnace();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-authoritative. Moves Count of ItemDef from Player's inventory into the furnace, then
	 *  attempts to start smelting. Returns false if not server, out of range, or the transfer
	 *  can't be satisfied. */
	UFUNCTION(BlueprintCallable, Category = "Furnace")
	bool DepositItem(AAccidentalHeroCharacter* Player, UItemDefinition* ItemDef, int32 Count);

	/** Server-authoritative. Reverse transfer, furnace -> player. */
	UFUNCTION(BlueprintCallable, Category = "Furnace")
	bool WithdrawItem(AAccidentalHeroCharacter* Player, UItemDefinition* ItemDef, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Furnace")
	bool IsInRange(AAccidentalHeroCharacter* Player) const;

	UFUNCTION(BlueprintPure, Category = "Furnace")
	bool IsSmelting() const { return bIsSmelting; }

	UFUNCTION(BlueprintPure, Category = "Furnace")
	URecipeDefinition* GetActiveRecipe() const { return ActiveRecipe; }

	/** 0 if not smelting; otherwise seconds left, computed locally from the replicated absolute
	 *  SmeltEndTime — no per-tick replication needed for a progress readout. */
	UFUNCTION(BlueprintPure, Category = "Furnace")
	float GetSmeltTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Furnace")
	UFurnaceInventoryComponent* GetFurnaceInventory() const { return FurnaceInventory; }

protected:
	virtual void BeginPlay() override;

	/** Server-only. Lazily refreshes SmeltingRecipeCache if empty, so this also works when called
	 *  on an actor that never got BeginPlay (e.g. spawned directly in the editor world rather than
	 *  a running Play session). If not already smelting, scans the cache for a recipe
	 *  FurnaceInventory can afford; if found, removes ingredients immediately (no cancel-to-refund
	 *  exploit), sets ActiveRecipe/bIsSmelting/SmeltEndTime, and starts SmeltTimerHandle. Returns
	 *  whether it started. */
	bool TryStartSmelt();

	/** Timer callback. Adds ActiveRecipe's output, clears smelting state, and calls
	 *  TryStartSmelt() again so the furnace keeps consuming ingredients as long as they're
	 *  available. */
	void OnSmeltComplete();

	/** Server-only. Populates SmeltingRecipeCache from every Primary Asset of type "Recipe"
	 *  tagged Recipe.Category.Smelting. */
	void RefreshSmeltingRecipeCache();

	UPROPERTY(VisibleAnywhere, Category = "Furnace")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Furnace")
	TObjectPtr<USphereComponent> InteractionRange;

	UPROPERTY(VisibleAnywhere, Category = "Furnace")
	TObjectPtr<UFurnaceInventoryComponent> FurnaceInventory;

	UPROPERTY(EditDefaultsOnly, Category = "Furnace")
	float InteractionRadius = 150.0f;

	UPROPERTY(Replicated)
	bool bIsSmelting = false;

	UPROPERTY(Replicated)
	TObjectPtr<URecipeDefinition> ActiveRecipe = nullptr;

	/** Absolute world time (GetWorld()->GetTimeSeconds()) smelting finishes — not a replicated
	 *  countdown, so clients derive remaining time locally without needing a stream of updates. */
	UPROPERTY(Replicated)
	float SmeltEndTime = 0.0f;

	FTimerHandle SmeltTimerHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URecipeDefinition>> SmeltingRecipeCache;
};
