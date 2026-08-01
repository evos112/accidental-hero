// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResourceNode.h"
#include "ItemPickup.h"
#include "ItemDefinition.h"
#include "InventoryComponent.h"
#include "InventoryTypes.h"
#include "AccidentalHeroCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AResourceNode::AResourceNode()
{
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RootComponent = MeshComponent;
}

void AResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AResourceNode, HitsRemaining);
	DOREPLIFETIME(AResourceNode, bDepleted);
}

void AResourceNode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		HitsRemaining = MaxHits;
	}
}

bool AResourceNode::IsInRange(AAccidentalHeroCharacter* Player) const
{
	// Distance check rather than a tracked overlap set, same reasoning as AFurnace::IsInRange:
	// Harvest() is called synchronously from UGA_Gather's sweep, not an overlap event.
	return Player && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) <= GatherInteractRadius;
}

void AResourceNode::ConsumeToolDurability(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const
{
	if (!HasAuthority() || !Player)
	{
		return;
	}

	UInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// Only the tool that did the work wears, and that is whatever is in hand — matching
	// GetEquippedToolTier, so a tool can never grant its tier without also taking the wear.
	if (Inventory->GetEquippedToolTier(ToolTag) <= 0)
	{
		return;
	}

	const int32 EntryIndex = Inventory->FindEquippedEntry();
	if (EntryIndex != INDEX_NONE)
	{
		Inventory->SpendToolDurability(EntryIndex);
	}
}

int32 AResourceNode::GetEquippedToolTier(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag) const
{
	if (!Player || !ToolTag.IsValid())
	{
		return 0;
	}

	const UInventoryComponent* Inventory = Player->GetInventoryComponent();
	return Inventory ? Inventory->GetEquippedToolTier(ToolTag) : 0;
}

bool AResourceNode::Harvest(AAccidentalHeroCharacter* Player)
{
	if (!HasAuthority() || bDepleted || !Player || !OutputItem || !IsInRange(Player))
	{
		return false;
	}

	AItemPickup::SpawnItemPickup(this, OutputItem, YieldPerHit, FTransform(GetActorLocation() + FVector(0.0f, 0.0f, 20.0f)));

	HitsRemaining -= FMath::Max(1, GetStrikePower(Player));
	if (HitsRemaining <= 0)
	{
		Deplete();
	}
	return true;
}

void AResourceNode::SetFoliageOrigin(UInstancedStaticMeshComponent* SourceComponent, const FTransform& SourceTransform)
{
	SourceFoliageComponent = SourceComponent;
	SourceFoliageTransform = SourceTransform;
	bFromFoliage = SourceComponent != nullptr;
}

void AResourceNode::Deplete()
{
	bDepleted = true;
	ApplyDepletedVisualState();

	// A node carved out of the foliage goes back into the foliage rather than respawning as an
	// actor — otherwise every tree ever felled would accumulate as a permanent actor.
	if (bFromFoliage)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AResourceNode::RegrowAsFoliage,
			FoliageRegrowSeconds, false);
		return;
	}

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AResourceNode::Respawn, RespawnSeconds, false);
}

void AResourceNode::RegrowAsFoliage()
{
	if (!HasAuthority())
	{
		return;
	}

	// The foliage actor can be unloaded by World Partition while this counts down. Falling back to
	// an ordinary actor respawn would leave a tree standing in an unloaded cell, so just remove
	// this node — the instance is gone either way, and the world reloads from disk on next play.
	if (UInstancedStaticMeshComponent* Component = SourceFoliageComponent.Get())
	{
		Component->AddInstance(SourceFoliageTransform, /*bWorldSpace=*/true);
	}

	Destroy();
}

void AResourceNode::Respawn()
{
	HitsRemaining = MaxHits;
	bDepleted = false;
	ApplyDepletedVisualState();
}

void AResourceNode::OnRep_Depleted()
{
	ApplyDepletedVisualState();
}

void AResourceNode::ApplyDepletedVisualState()
{
	MeshComponent->SetVisibility(!bDepleted);
	MeshComponent->SetCollisionEnabled(bDepleted ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}
