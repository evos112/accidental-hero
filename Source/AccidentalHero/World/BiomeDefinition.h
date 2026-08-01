// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BiomeDefinition.generated.h"

class UStaticMesh;
class AResourceNode;

/** One plant layer in a biome: which mesh, how thickly, and what it yields when struck. */
USTRUCT(BlueprintType)
struct ACCIDENTALHERO_API FBiomeFoliageLayer
{
	GENERATED_BODY()

	/** Soft so authoring a biome doesn't drag every mesh into memory — only the editor-side
	 *  scatter step resolves these, and it does so one layer at a time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Foliage")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Instances per square kilometre. Expressed per-area rather than as a total so the same
	 *  biome reads the same whether it's applied to a 300 m pocket or a 3 km valley. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Foliage", meta = (ClampMin = "0"))
	float DensityPerSqKm = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Foliage", meta = (ClampMin = "0.01"))
	float MinScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Foliage", meta = (ClampMin = "0.01"))
	float MaxScale = 1.2f;

	/** Node spawned when this plant is harvested. Null means scenery — it can't be gathered.
	 *  Ground cover deliberately stays null; only canopy and mid-storey give resources. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Foliage")
	TSubclassOf<AResourceNode> HarvestNodeClass;
};

/**
 * Everything that makes one stretch of the map feel different from another: what grows there,
 * what the ground is, and what it costs you to be there.
 *
 * This is data, not code, on purpose. The plant list is the part most likely to be replaced
 * wholesale — the project's foliage today is temperate (pine, elderberry, grass), so a tropical
 * biome built from it is an approximation. Dropping a real rainforest pack in later means editing
 * FoliageLayers on this asset; nothing in C++ names a specific mesh.
 *
 * Regions are placed with ABiomeRegion and answered by UBiomeSubsystem::GetBiomeAt.
 */
UCLASS(BlueprintType)
class ACCIDENTALHERO_API UBiomeDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome")
	FText DisplayName;

	/** Identity for queries and future recipe/quest gating ("craftable only in the rainforest"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome")
	FGameplayTag BiomeTag;

	/** Added to UGE_SurvivalDrain's baseline while the player is inside, so a biome makes the
	 *  existing clock faster or slower rather than replacing it. Positive drains. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome|Survival")
	float ExtraHungerPerSecond = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome|Survival")
	float ExtraThirstPerSecond = 0.0f;

	/** Walk speed scale inside the biome. Undergrowth you have to push through is most of what
	 *  makes a jungle feel like a jungle rather than a green forest. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome|Movement", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MoveSpeedMultiplier = 1.0f;

	/** Landscape paint layer the editor-side applier weights up across the region. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome|Terrain")
	FName GroundLayerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biome|Foliage")
	TArray<FBiomeFoliageLayer> FoliageLayers;

	/** True when standing here costs anything; lets the character skip applying an empty effect. */
	UFUNCTION(BlueprintPure, Category = "Biome")
	bool HasSurvivalDrain() const
	{
		return !FMath::IsNearlyZero(ExtraHungerPerSecond) || !FMath::IsNearlyZero(ExtraThirstPerSecond);
	}
};
