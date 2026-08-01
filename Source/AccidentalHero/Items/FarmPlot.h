// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmPlot.generated.h"

class UStaticMeshComponent;
class ACropPlant;

/**
 * A placed planting bed: tilled soil that holds a fixed grid of crops.
 *
 * Exists to make farming deliberate rather than "sow anywhere the ground is flat". A plot gives
 * crops tidy rows, keeps them from being scattered across the landscape, and holds moisture — a
 * crop in a bed drinks slower than one stuck in raw ground, so building beds is what makes a farm
 * maintainable instead of a chore.
 *
 * Slots are computed from Rows/Columns rather than hand-placed components, so a bigger bed is a
 * data change. Occupancy is tracked by pointer, and a slot frees itself when its crop dies.
 */
UCLASS()
class ACCIDENTALHERO_API AFarmPlot : public AActor
{
	GENERATED_BODY()

public:
	AFarmPlot();

	/** World transform of a slot, whether or not it's occupied. */
	UFUNCTION(BlueprintPure, Category = "Farm Plot")
	FTransform GetSlotTransform(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Farm Plot")
	int32 GetSlotCount() const { return FMath::Max(0, Rows) * FMath::Max(0, Columns); }

	/** First slot with nothing growing in it, or INDEX_NONE when the bed is full. */
	UFUNCTION(BlueprintPure, Category = "Farm Plot")
	int32 FindFreeSlot() const;

	/** Records a crop as occupying a slot, and tells the crop it's in a bed. */
	UFUNCTION(BlueprintCallable, Category = "Farm Plot")
	void OccupySlot(int32 SlotIndex, ACropPlant* Crop);

	/** Which slot a crop sits in, or INDEX_NONE. Used when saving, so a bed's layout survives. */
	UFUNCTION(BlueprintPure, Category = "Farm Plot")
	int32 FindSlotOf(const ACropPlant* Crop) const;

	/** Multiplier applied to a planted crop's water drain. Below 1 means the bed holds moisture. */
	UFUNCTION(BlueprintPure, Category = "Farm Plot")
	float GetMoistureRetention() const { return MoistureRetention; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Farm Plot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category = "Farm Plot", meta = (ClampMin = "1"))
	int32 Rows = 2;

	UPROPERTY(EditAnywhere, Category = "Farm Plot", meta = (ClampMin = "1"))
	int32 Columns = 2;

	/** Centre-to-centre spacing between slots, in cm. */
	UPROPERTY(EditAnywhere, Category = "Farm Plot", meta = (ClampMin = "10.0"))
	float SlotSpacing = 90.0f;

	/** Tilled soil stays damp: 0.5 means a crop in a bed drinks at half the open-ground rate. */
	UPROPERTY(EditAnywhere, Category = "Farm Plot", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MoistureRetention = 0.5f;

	/** One entry per slot; null means free. Not replicated — placement is server-authoritative and
	 *  clients only need to see the crops themselves, which replicate on their own. */
	UPROPERTY()
	TArray<TObjectPtr<ACropPlant>> SlotOccupants;
};
