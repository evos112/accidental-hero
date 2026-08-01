// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaterSource.h"
#include "AccidentalHeroCharacter.h"
#include "InventoryComponent.h"
#include "InventoryTypes.h"
#include "ItemDefinition.h"
#include "AbilitySystem/AccidentalHeroAttributeSet.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

AWaterSource::AWaterSource()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	// Walk-through, not walk-into: wading into a shallow pond shouldn't be blocked, but the
	// surface still needs to be visible to a trace so the player can aim at it.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = MeshComponent;
}

bool AWaterSource::IsInRange(const AAccidentalHeroCharacter* Player) const
{
	if (!Player)
	{
		return false;
	}

	// Horizontal only. A pond sits at its own surface height and the player stands on the bank
	// above it, so counting the drop would make the water unreachable from dry land.
	return FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation())
		<= FMath::Square(DrinkRadius);
}

bool AWaterSource::Drink(AAccidentalHeroCharacter* Player)
{
	if (!HasAuthority() || !Player || !IsInRange(Player))
	{
		return false;
	}

	bool bDidSomething = false;

	if (UAccidentalHeroAttributeSet* Attributes = Player->GetMutableAttributeSet())
	{
		if (Attributes->GetThirst() < Attributes->GetMaxThirst())
		{
			Attributes->SetThirst(Attributes->GetMaxThirst());
			bDidSomething = true;
		}
	}

	// Same action refills what you carry, so nobody has to discover a separate "fill" verb.
	if (UInventoryComponent* Inventory = Player->GetInventoryComponent())
	{
		for (const FInventoryItemEntry& Entry : Inventory->GetAllItems())
		{
			if (Entry.ItemDef && Entry.ItemDef->ContainerThirstPerUse > 0.0f)
			{
				if (Inventory->RefillContainer(Entry.ItemDef))
				{
					bDidSomething = true;
				}
			}
		}
	}

	if (bDidSomething && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("You drink your fill."));
	}

	return bDidSomething;
}
