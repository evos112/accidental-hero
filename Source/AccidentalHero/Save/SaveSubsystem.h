// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSubsystem.generated.h"

class AAccidentalHeroCharacter;
class UAccidentalHeroSaveGame;
class URecipeDefinition;

/**
 * Owns the single save slot: writing it, reading it, and putting the world back.
 *
 * Lives on the GameInstance rather than the GameMode or PlayerController because it must outlive
 * both — the player pawn is destroyed on travel and respawn, and the save has to survive that.
 *
 * Restore happens in two halves, because the world and the player become ready at different times:
 * RestoreWorld() runs from the GameMode once the level is up, and RestorePlayer() runs from the
 * character once its PlayerState (and therefore its inventory) exists.
 */
UCLASS()
class ACCIDENTALHERO_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Writes everything to the slot. Server-only; safe to call at any time. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveGame();

	/** Reads the slot into memory. Does not touch the world — call the Restore functions for that.
	 *  Returns false when there's no save or the version doesn't match. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadGame();

	/** Recreates crops and farm plots. Runs once per level; safe to call when no save is loaded. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void RestoreWorld(UWorld* World);

	/** Puts a character's inventory, hotbar, attributes and position back. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void RestorePlayer(AAccidentalHeroCharacter* Character);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save")
	bool HasSave() const { return LoadedSave != nullptr; }

	/** Clears the slot and the in-memory copy. Used by "new game". */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void DeleteSave();

	/** Slot name. One save per installation — SPEC decided against manual slots. */
	static const TCHAR* SlotName;

protected:
	/** Bound to the crafting component so a successful craft is never lost. */
	UFUNCTION()
	void HandleCraftingResult(URecipeDefinition* Recipe, bool bSuccess);

	UFUNCTION()
	void HandleAutosaveTimer();

private:
	/** The world the autosave timer is running against, so it can be cleaned up. */
	TWeakObjectPtr<UWorld> TimerWorld;

	UPROPERTY()
	TObjectPtr<UAccidentalHeroSaveGame> LoadedSave;

	/** Guards RestoreWorld against running twice for one level load. */
	bool bWorldRestored = false;

	FTimerHandle AutosaveHandle;
};
