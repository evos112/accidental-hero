// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BiomeRegion.generated.h"

class UBiomeDefinition;

/**
 * Marks a circular stretch of the map as belonging to a biome.
 *
 * A circle rather than a volume because the world is built by script: placing one actor with a
 * radius is a single call, where authoring a shaped volume is not. Containment ignores Z, since a
 * biome is a region of the map — standing on a hill inside the rainforest is still inside it, and
 * a box would have to be tall enough to cover the terrain anyway.
 */
UCLASS()
class ACCIDENTALHERO_API ABiomeRegion : public AActor
{
	GENERATED_BODY()

public:
	ABiomeRegion();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	TObjectPtr<UBiomeDefinition> Biome;

	/** Horizontal radius in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome", meta = (ClampMin = "0"))
	float Radius = 30000.0f;

	UFUNCTION(BlueprintPure, Category = "Biome")
	bool ContainsLocation(const FVector& WorldLocation) const;

	float GetRadius() const { return Radius; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
