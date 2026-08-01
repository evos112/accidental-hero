// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UItemDefinition;
class UStaticMeshComponent;
class USphereComponent;
class UPrimitiveComponent;

/**
 * Minimal world pickup: an item stack that's added to the overlapping character's inventory
 * on contact. Auto-pickup on overlap (not interact/keypress) — fewer moving parts for now;
 * interact-based pickup can gate the same AddItem call behind a keypress later if wanted.
 *
 * SpawnItemPickup() is the single entry point other systems (e.g. the voxel digging/mining
 * system) can call from Blueprint to drop a mined resource without new C++ glue.
 */
UCLASS()
class ACCIDENTALHERO_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	UFUNCTION(BlueprintCallable, Category = "Item", meta = (WorldContext = "WorldContextObject"))
	static AItemPickup* SpawnItemPickup(UObject* WorldContextObject, UItemDefinition* ItemDef, int32 Count,
		const FTransform& SpawnTransform, int32 Durability = -1);

	/** Wear carried by a dropped tool. -1 means "full", which is right for anything freshly
	 *  harvested. Without this a worn axe would come back off the ground good as new. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	int32 Durability = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemDef, Category = "Item")
	TObjectPtr<UItemDefinition> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	int32 StackCount = 1;

	/** Swaps in ItemDef->WorldMesh if it has one, leaving the default placeholder otherwise.
	 *  Called on spawn (server) and via OnRep so clients show the right model too. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void ApplyItemVisual();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ItemDef();

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<USphereComponent> CollisionSphere;
};
