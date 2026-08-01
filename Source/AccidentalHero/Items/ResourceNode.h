// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "ResourceNode.generated.h"

class UStaticMeshComponent;
class UItemDefinition;
class AAccidentalHeroCharacter;

/**
 * World-placeable gatherable node (tree, rock, etc). Single data-driven class — different node
 * types are the same actor with a different OutputItem/mesh, the same way AItemPickup is
 * data-driven via ItemDef rather than subclassed per item. Harvested by UGA_Gather's forward
 * sweep, which calls Harvest() on whatever AResourceNode it hits; delivery to the player reuses
 * AItemPickup::SpawnItemPickup (spawns a pickup at the node, the player's existing auto-pickup
 * overlap collects it) instead of adding to inventory directly.
 */
UCLASS()
class ACCIDENTALHERO_API AResourceNode : public AActor
{
	GENERATED_BODY()

public:
	AResourceNode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-authoritative. Spawns YieldPerHit of OutputItem as a world pickup near this node and
	 *  decrements HitsRemaining. Returns false if not server, depleted, out of range, or
	 *  Player/OutputItem is invalid. Depletes the node once HitsRemaining reaches 0. */
	UFUNCTION(BlueprintCallable, Category = "ResourceNode")
	virtual bool Harvest(AAccidentalHeroCharacter* Player);

	UFUNCTION(BlueprintPure, Category = "ResourceNode")
	bool IsInRange(AAccidentalHeroCharacter* Player) const;

	UFUNCTION(BlueprintPure, Category = "ResourceNode")
	bool IsDepleted() const { return bDepleted; }

	UFUNCTION(BlueprintPure, Category = "ResourceNode")
	int32 GetHitsRemaining() const { return HitsRemaining; }

protected:
	virtual void BeginPlay() override;

	/** How many hits a single strike removes. Base is always 1; ATreeNode overrides it so better
	 *  axes fell a tree in fewer swings. */
	virtual int32 GetStrikePower(AAccidentalHeroCharacter* Player) const { return 1; }

	/** Highest ToolTier among carried items tagged ToolTag, or 0 if the player has none.
	 *  Tier lives on the item and purpose lives in the tag, so adding a Bronze Axe is data only.
	 *  Reads the held stacks rather than scanning every Item asset — this runs per swing. */
	int32 GetBestToolTier(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const;

	/** Spends one use of whichever tool backs ToolTag. Call once per *successful* harvest, so a
	 *  swing that missed or hit a depleted node costs nothing. Server-only; safe to call when the
	 *  player has no such tool (bare hands wear nothing). */
	void ConsumeToolDurability(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const;

	/** Server-only. Marks the node depleted and starts the respawn timer. */
	virtual void Deplete();

	/** Timer callback (server-only). Restores HitsRemaining and clears bDepleted. */
	virtual void Respawn();

	UFUNCTION()
	void OnRep_Depleted();

	/** Shared by the server (called directly from Deplete/Respawn) and clients (via OnRep) so the
	 *  mesh visually hides/reappears everywhere, not just on the authority. */
	void ApplyDepletedVisualState();

	UPROPERTY(VisibleAnywhere, Category = "ResourceNode")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category = "ResourceNode")
	TObjectPtr<UItemDefinition> OutputItem;

	UPROPERTY(EditAnywhere, Category = "ResourceNode", meta = (ClampMin = "1"))
	int32 YieldPerHit = 1;

	UPROPERTY(EditAnywhere, Category = "ResourceNode", meta = (ClampMin = "1"))
	int32 MaxHits = 3;

	UPROPERTY(EditAnywhere, Category = "ResourceNode")
	float GatherInteractRadius = 200.0f;

	UPROPERTY(EditAnywhere, Category = "ResourceNode")
	float RespawnSeconds = 45.0f;

	UPROPERTY(Replicated)
	int32 HitsRemaining = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Depleted)
	bool bDepleted = false;

	FTimerHandle RespawnTimerHandle;
};
