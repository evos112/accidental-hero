// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AccidentalHeroSaveGame.generated.h"

class UItemDefinition;
class ACropPlant;
class AFarmPlot;

/** One inventory entry, flattened. Durability rides along so a worn tool stays worn. */
USTRUCT()
struct FSavedItemStack
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UItemDefinition> ItemDef;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY()
	int32 Durability = 0;
};

/** A planted crop, including how far along it is and how thirsty. */
USTRUCT()
struct FSavedCrop
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<ACropPlant> CropClass;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	int32 GrowthStage = 0;

	UPROPERTY()
	float StageProgress = 0.0f;

	UPROPERTY()
	float WaterLevel = 0.0f;

	UPROPERTY()
	float Vitality = 100.0f;

	UPROPERTY()
	bool bWithered = false;

	/** Index into SavedPlots, or INDEX_NONE for a crop sown straight into open ground. Plots are
	 *  restored first so this index is always resolvable. */
	UPROPERTY()
	int32 PlotIndex = INDEX_NONE;

	UPROPERTY()
	int32 PlotSlot = INDEX_NONE;
};

/** A placed planting bed. Slot occupancy is rebuilt from the crops, not stored twice. */
USTRUCT()
struct FSavedFarmPlot
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<AFarmPlot> PlotClass;

	UPROPERTY()
	FTransform Transform;
};

/**
 * Everything that survives quitting.
 *
 * Deliberately does NOT record foliage. Felled trees regrow on their own (SPEC 5.12), so a reloaded
 * forest is indistinguishable from a correct one — which lets us skip the foliage-identity problem
 * entirely (SPEC 6.1).
 */
UCLASS()
class ACCIDENTALHERO_API UAccidentalHeroSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bumped whenever the layout below changes meaning. Loading a mismatched version is refused
	 *  rather than half-applied — a wrong load is worse than a lost one. */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	UPROPERTY()
	FDateTime SavedAtUtc;

	//~ Player
	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bIsFirstPerson = false;

	UPROPERTY()
	float Health = 0.0f;

	UPROPERTY()
	float Stamina = 0.0f;

	UPROPERTY()
	float Hunger = 0.0f;

	UPROPERTY()
	float Thirst = 0.0f;

	//~ Inventory
	UPROPERTY()
	TArray<FSavedItemStack> Items;

	/** One entry per quick-bar key; empty entries are null soft pointers. */
	UPROPERTY()
	TArray<TSoftObjectPtr<UItemDefinition>> HotbarSlots;

	//~ World
	UPROPERTY()
	TArray<FSavedFarmPlot> Plots;

	UPROPERTY()
	TArray<FSavedCrop> Crops;
};
