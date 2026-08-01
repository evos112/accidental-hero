// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResourceNode.h"
#include "CropPlant.generated.h"

class UStaticMesh;
class UWidgetComponent;
class AFarmPlot;

/**
 * A planted crop that needs tending: it drinks, it grows, and it dies if neglected.
 *
 * Deliberately an AResourceNode subclass rather than a new system: harvesting is already a solved
 * problem here (forward sweep on G, yield spawns as a pickup, replicated depletion), so a crop is
 * a node that refuses to be harvested until it has finished growing, and reverts to seedling
 * instead of vanishing.
 *
 * Everything runs off a single one-second timer rather than Tick, so a field of crops costs one
 * timer each no matter how many are planted. Growth only advances on seconds where the plant had
 * water; dry seconds cost Vitality instead, and a plant that runs out withers.
 */
UCLASS()
class ACCIDENTALHERO_API ACropPlant : public AResourceNode
{
	GENERATED_BODY()

public:
	ACropPlant();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Blocked until the final growth stage; harvesting resets the crop to a seedling. */
	virtual bool Harvest(AAccidentalHeroCharacter* Player) override;

	UFUNCTION(BlueprintPure, Category = "Crop")
	bool IsRipe() const { return GrowthStage >= GrowthStageMeshes.Num() - 1; }

	UFUNCTION(BlueprintPure, Category = "Crop")
	int32 GetGrowthStage() const { return GrowthStage; }

	/** 0-1 across the whole plant, seedling to ripe, including progress within the current stage. */
	UFUNCTION(BlueprintPure, Category = "Crop")
	float GetGrowthFraction() const;

	UFUNCTION(BlueprintPure, Category = "Crop")
	float GetWaterFraction() const { return MaxWater > 0.0f ? FMath::Clamp(WaterLevel / MaxWater, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Crop")
	float GetVitalityFraction() const { return FMath::Clamp(Vitality / 100.0f, 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "Crop")
	bool IsWithered() const { return bWithered; }

	UFUNCTION(BlueprintPure, Category = "Crop")
	bool NeedsWater() const { return bRequiresWater && GetWaterFraction() < 0.999f; }

	/** Display name for the interaction prompt ("Harvest Berry Bush"). */
	UFUNCTION(BlueprintPure, Category = "Crop")
	FText GetCropName() const;

	/** Tops the plant up. Returns false when it's already full or already dead. */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	bool AddWater(float Amount);

	UFUNCTION(BlueprintPure, Category = "Crop")
	float GetWaterPerPour() const { return WaterPerPour; }

	/** Reads out everything a save needs. Kept as one call so the save code can't quietly miss a
	 *  field when a new one is added here. */
	UFUNCTION(BlueprintCallable, Category = "Crop|Save")
	void GetSaveState(int32& OutGrowthStage, float& OutStageProgress, float& OutWaterLevel,
		float& OutVitality, bool& bOutWithered) const;

	/** Restores state loaded from disk. Call immediately after spawning; BeginPlay's seeding of
	 *  water and ripeness is overwritten by design. */
	UFUNCTION(BlueprintCallable, Category = "Crop|Save")
	void ApplySaveState(int32 InGrowthStage, float InStageProgress, float InWaterLevel,
		float InVitality, bool bInWithered);

	/** Records that this crop is growing in a bed, which slows its water drain. */
	UFUNCTION(BlueprintCallable, Category = "Crop")
	void SetPlantedInPlot(AFarmPlot* Plot) { HomePlot = Plot; }

	UFUNCTION(BlueprintPure, Category = "Crop")
	AFarmPlot* GetHomePlot() const { return HomePlot; }

protected:
	virtual void BeginPlay() override;

	/** One-second tick: drink, grow or decay. */
	void TendTick();

	UFUNCTION()
	void OnRep_GrowthStage();

	UFUNCTION()
	void OnRep_CropState();

	/** Pushes the current stage's mesh onto MeshComponent. Runs on server and clients. */
	void ApplyGrowthVisual();

	/** Seedling first, ripe last. The number of entries defines how many stages there are. */
	UPROPERTY(EditAnywhere, Category = "Crop")
	TArray<TObjectPtr<UStaticMesh>> GrowthStageMeshes;

	/** Watered seconds needed per stage. Total grow time is this multiplied by the stage count —
	 *  longer in practice, since dry seconds don't count. */
	UPROPERTY(EditAnywhere, Category = "Crop", meta = (ClampMin = "1.0"))
	float SecondsPerStage = 30.0f;

	/** Seeds returned on harvest so a field is self-sustaining — the replanting loop. */
	UPROPERTY(EditAnywhere, Category = "Crop")
	TObjectPtr<UItemDefinition> SeedItem;

	UPROPERTY(EditAnywhere, Category = "Crop", meta = (ClampMin = "0"))
	int32 SeedYield = 1;

	/** Spawn already ripe instead of as a seedling. Wild bushes converted from foliage are grown
	 *  plants the player is walking up to — starting them at stage 0 would swap a full bush for a
	 *  sprout and refuse the harvest. Player-planted seeds leave this false and grow normally. */
	UPROPERTY(EditAnywhere, Category = "Crop")
	bool bStartRipe = false;

	/** Wild plants look after themselves; only sown crops need tending. */
	UPROPERTY(EditAnywhere, Category = "Crop|Water")
	bool bRequiresWater = true;

	UPROPERTY(EditAnywhere, Category = "Crop|Water", meta = (ClampMin = "1.0"))
	float MaxWater = 100.0f;

	/** Empties a full plant in ~100 s, so a crop needs revisiting a few times per stage. */
	UPROPERTY(EditAnywhere, Category = "Crop|Water", meta = (ClampMin = "0.0"))
	float WaterDrainPerSecond = 1.0f;

	/** How much one pour from a watering can restores. */
	UPROPERTY(EditAnywhere, Category = "Crop|Water", meta = (ClampMin = "1.0"))
	float WaterPerPour = 60.0f;

	/** Vitality lost per second while bone dry. At 2/s a forgotten crop dies in 50 seconds of
	 *  drought, which is recoverable if you come back but fatal if you don't. */
	UPROPERTY(EditAnywhere, Category = "Crop|Water", meta = (ClampMin = "0.0"))
	float DecayPerSecond = 2.0f;

	/** Vitality regained per watered second, so a wilting plant recovers if you tend it. */
	UPROPERTY(EditAnywhere, Category = "Crop|Water", meta = (ClampMin = "0.0"))
	float RecoveryPerSecond = 1.0f;

	/** Floating growth/water readout. Optional — a null widget class just means no readout. */
	UPROPERTY(VisibleAnywhere, Category = "Crop")
	TObjectPtr<UWidgetComponent> StatusWidget;

	UPROPERTY(ReplicatedUsing = OnRep_GrowthStage)
	int32 GrowthStage = 0;

	/** Watered seconds banked toward the next stage. */
	UPROPERTY(ReplicatedUsing = OnRep_CropState)
	float StageProgress = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CropState)
	float WaterLevel = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CropState)
	float Vitality = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CropState)
	bool bWithered = false;

	/** Bed this crop was sown into, if any. Slows water drain by the plot's retention factor. */
	UPROPERTY()
	TObjectPtr<AFarmPlot> HomePlot;

	FTimerHandle TendTimerHandle;
};
