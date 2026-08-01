// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreeNode.h"
#include "ItemDefinition.h"
#include "AccidentalHeroCharacter.h"
#include "InventoryComponent.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ATreeNode::ATreeNode()
{
	// Felling is a one-off event, so the trunk is a component that starts inert and is switched
	// to simulating on Deplete() rather than a separate actor spawned at runtime.
	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(RootComponent);
	TrunkMesh->SetSimulatePhysics(false);
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrunkMesh->SetHiddenInGame(true);
	TrunkMesh->SetNotifyRigidBodyCollision(true);

	SaplingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SaplingMesh"));
	SaplingMesh->SetupAttachment(RootComponent);
	SaplingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SaplingMesh->SetHiddenInGame(true);
}

bool ATreeNode::Harvest(AAccidentalHeroCharacter* Player)
{
	// Captured before Super may call Deplete(), which needs to know which way to topple.
	if (Player)
	{
		LastChopper = Player;
	}

	const bool bHarvested = Super::Harvest(Player);
	if (bHarvested)
	{
		// Only a landed chop wears the axe. Bare hands wear nothing, so this is a no-op then.
		ConsumeToolDurability(Player, AccidentalHeroGameplayTags::Item_Tool_Axe);
	}
	return bHarvested;
}

int32 ATreeNode::GetStrikePower(AAccidentalHeroCharacter* Player) const
{
	// No equipment system yet, so this is "best axe carried" rather than "axe equipped" — a
	// crafted axe takes effect the moment it lands in the bag.
	const int32 AxeTier = GetEquippedToolTier(Player, AccidentalHeroGameplayTags::Item_Tool_Axe);
	return AxeTier > 0 ? AxeTier * PowerPerAxeTier : BareHandPower;
}

void ATreeNode::Deplete()
{
	// Standing tree disappears; the trunk takes its place and topples.
	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (SaplingMesh)
	{
		SaplingMesh->SetHiddenInGame(false);
	}

	if (TrunkMesh)
	{
		TrunkMesh->SetHiddenInGame(false);
		TrunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		TrunkMesh->SetSimulatePhysics(true);
		TrunkMesh->OnComponentHit.AddDynamic(this, &ATreeNode::OnTrunkHit);

		// Push it away from whoever swung, with the impulse applied high up so it rotates about
		// the base and falls rather than sliding.
		FVector FallDirection = GetActorForwardVector();
		if (const AAccidentalHeroCharacter* Chopper = LastChopper.Get())
		{
			const FVector Away = GetActorLocation() - Chopper->GetActorLocation();
			if (!Away.IsNearlyZero())
			{
				FallDirection = Away.GetSafeNormal2D();
			}
		}

		const FVector TopOfTrunk = TrunkMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 300.0f);
		TrunkMesh->AddImpulseAtLocation(FallDirection * FallImpulse, TopOfTrunk);

		GetWorldTimerManager().SetTimer(TrunkCleanupHandle, this, &ATreeNode::ResetTrunk,
			TrunkLifetime, false);
	}

	Super::Deplete();
}

void ATreeNode::OnTrunkHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Only a moving trunk crushes — once it has settled it is scenery, not a hazard.
	if (!TrunkMesh || TrunkMesh->GetComponentVelocity().Size() < CrushMinSpeed)
	{
		return;
	}

	if (AAccidentalHeroCharacter* Victim = Cast<AAccidentalHeroCharacter>(OtherActor))
	{
		UGameplayStatics::ApplyDamage(Victim, CrushDamage, nullptr, this, nullptr);
	}
}

void ATreeNode::ResetTrunk()
{
	if (TrunkMesh)
	{
		TrunkMesh->OnComponentHit.RemoveDynamic(this, &ATreeNode::OnTrunkHit);
		TrunkMesh->SetSimulatePhysics(false);
		TrunkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TrunkMesh->SetHiddenInGame(true);
		TrunkMesh->SetRelativeTransform(FTransform::Identity);
	}
}

void ATreeNode::Respawn()
{
	ResetTrunk();

	if (SaplingMesh)
	{
		SaplingMesh->SetHiddenInGame(true);
	}
	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	Super::Respawn();
}
