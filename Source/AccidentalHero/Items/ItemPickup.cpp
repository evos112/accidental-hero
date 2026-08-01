// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemPickup.h"
#include "ItemDefinition.h"
#include "InventoryComponent.h"
#include "AccidentalHeroCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AItemPickup::AItemPickup()
{
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComponent;

	// Default visual so a dropped item is never invisible: previously MeshComponent had no mesh at
	// all unless hand-assigned per instance, so pickups spawned dynamically by
	// AResourceNode::Harvest (the actual gameplay drop path) rendered nothing.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPickupMeshFinder(
		TEXT("/Game/ProceduralNtr_vol2/pnp_v2_assets/Assets/rocks/rock_a/rock_a.rock_a"));
	if (DefaultPickupMeshFinder.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultPickupMeshFinder.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.15f));
	}

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(75.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnSphereBeginOverlap);
}

AItemPickup* AItemPickup::SpawnItemPickup(UObject* WorldContextObject, UItemDefinition* ItemDef, int32 Count,
	const FTransform& SpawnTransform, int32 Durability)
{
	if (!ItemDef || Count <= 0)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
	if (Pickup)
	{
		Pickup->ItemDef = ItemDef;
		Pickup->StackCount = Count;
		Pickup->Durability = Durability;
		Pickup->ApplyItemVisual();
	}
	return Pickup;
}

void AItemPickup::ApplyItemVisual()
{
	if (!MeshComponent || !ItemDef)
	{
		return;
	}

	// Synchronous load: the pickup exists now and needs something to draw this frame. The soft
	// pointer keeps item art out of memory until an item is actually dropped in the world.
	if (UStaticMesh* Mesh = ItemDef->WorldMesh.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetRelativeScale3D(FVector(FMath::Max(0.001f, ItemDef->WorldMeshScale)));
	}
	// No WorldMesh set: keep the constructor's placeholder so the drop is never invisible.
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyItemVisual();
}

void AItemPickup::OnRep_ItemDef()
{
	ApplyItemVisual();
}

void AItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemPickup, ItemDef);
	DOREPLIFETIME(AItemPickup, StackCount);
	DOREPLIFETIME(AItemPickup, Durability);
}

void AItemPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !ItemDef)
	{
		return;
	}

	AAccidentalHeroCharacter* Character = Cast<AAccidentalHeroCharacter>(OtherActor);
	UInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	if (!Inventory)
	{
		return;
	}

	// Carries the wear back in, so dropping and re-collecting a tool can't repair it.
	const int32 AddedCount = Inventory->AddItem(ItemDef, StackCount, Durability);
	StackCount -= AddedCount;
	if (StackCount <= 0)
	{
		Destroy();
	}
}
