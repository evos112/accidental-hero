// Copyright Epic Games, Inc. All Rights Reserved.

#include "Furnace.h"
#include "FurnaceInventoryComponent.h"
#include "RecipeDefinition.h"
#include "RecipeUtils.h"
#include "InventoryComponent.h"
#include "AccidentalHeroCharacter.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

AFurnace::AFurnace()
{
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RootComponent = MeshComponent;

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(RootComponent);
	InteractionRange->SetSphereRadius(InteractionRadius);
	InteractionRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// No OnComponentBeginOverlap binding: unlike AItemPickup this isn't auto-triggered.
	// IsInRange() queries the engine's already-maintained overlap set at call time.

	FurnaceInventory = CreateDefaultSubobject<UFurnaceInventoryComponent>(TEXT("FurnaceInventory"));
}

void AFurnace::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFurnace, bIsSmelting);
	DOREPLIFETIME(AFurnace, ActiveRecipe);
	DOREPLIFETIME(AFurnace, SmeltEndTime);
}

void AFurnace::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		RefreshSmeltingRecipeCache();
	}
}

bool AFurnace::IsInRange(AAccidentalHeroCharacter* Player) const
{
	// Distance check rather than InteractionRange's tracked overlap set: this is called
	// synchronously from DepositItem/WithdrawItem (poll-based, not event-driven), and a physics
	// overlap set only updates while the physics scene is actually ticking (a running Play
	// session) — a direct distance check is accurate immediately in all contexts.
	return Player && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) <= InteractionRadius;
}

float AFurnace::GetSmeltTimeRemaining() const
{
	return bIsSmelting ? FMath::Max(0.0f, SmeltEndTime - GetWorld()->GetTimeSeconds()) : 0.0f;
}

bool AFurnace::DepositItem(AAccidentalHeroCharacter* Player, UItemDefinition* ItemDef, int32 Count)
{
	if (!HasAuthority() || !Player || !ItemDef || Count <= 0 || !IsInRange(Player))
	{
		return false;
	}

	UInventoryComponent* PlayerInventory = Player->GetInventoryComponent();
	if (!PlayerInventory || !PlayerInventory->HasItem(ItemDef, Count) || !FurnaceInventory->CanAddItem(ItemDef, Count))
	{
		return false;
	}

	const int32 Removed = PlayerInventory->RemoveItem(ItemDef, Count);
	const int32 Added = FurnaceInventory->AddItem(ItemDef, Removed);
	if (Added < Removed)
	{
		PlayerInventory->AddItem(ItemDef, Removed - Added);
	}

	if (Added > 0)
	{
		TryStartSmelt();
	}
	return Added > 0;
}

bool AFurnace::WithdrawItem(AAccidentalHeroCharacter* Player, UItemDefinition* ItemDef, int32 Count)
{
	if (!HasAuthority() || !Player || !ItemDef || Count <= 0 || !IsInRange(Player))
	{
		return false;
	}

	UInventoryComponent* PlayerInventory = Player->GetInventoryComponent();
	if (!PlayerInventory || !FurnaceInventory->HasItem(ItemDef, Count) || !PlayerInventory->CanAddItem(ItemDef, Count))
	{
		return false;
	}

	const int32 Removed = FurnaceInventory->RemoveItem(ItemDef, Count);
	const int32 Added = PlayerInventory->AddItem(ItemDef, Removed);
	if (Added < Removed)
	{
		FurnaceInventory->AddItem(ItemDef, Removed - Added);
	}
	return Added > 0;
}

bool AFurnace::TryStartSmelt()
{
	if (!HasAuthority() || bIsSmelting)
	{
		return false;
	}

	if (SmeltingRecipeCache.Num() == 0)
	{
		RefreshSmeltingRecipeCache();
	}

	for (URecipeDefinition* Recipe : SmeltingRecipeCache)
	{
		if (Recipe && RecipeUtils::CanAfford(Recipe, FurnaceInventory))
		{
			for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
			{
				FurnaceInventory->RemoveItem(Ingredient.Item, Ingredient.Count);
			}

			ActiveRecipe = Recipe;
			bIsSmelting = true;
			const float Duration = FMath::Max(Recipe->ProcessDuration, KINDA_SMALL_NUMBER);
			SmeltEndTime = GetWorld()->GetTimeSeconds() + Duration;
			GetWorldTimerManager().SetTimer(SmeltTimerHandle, this, &AFurnace::OnSmeltComplete, Duration, false);
			return true;
		}
	}
	return false;
}

void AFurnace::OnSmeltComplete()
{
	if (!HasAuthority() || !ActiveRecipe)
	{
		return;
	}

	FurnaceInventory->AddItem(ActiveRecipe->OutputItem, ActiveRecipe->OutputCount);
	bIsSmelting = false;
	ActiveRecipe = nullptr;
	SmeltEndTime = 0.0f;

	TryStartSmelt();
}

void AFurnace::RefreshSmeltingRecipeCache()
{
	UAssetManager& Manager = UAssetManager::Get();
	const FPrimaryAssetType RecipeType(TEXT("Recipe"));

	TArray<FPrimaryAssetId> RecipeIds;
	Manager.GetPrimaryAssetIdList(RecipeType, RecipeIds);
	if (RecipeIds.Num() == 0)
	{
		return;
	}

	// Small, one-time blocking load of every Recipe DataAsset. Acceptable at this project's tiny
	// recipe count; switch to the async LoadPrimaryAssets(..., FStreamableDelegate) overload
	// instead of blocking here if the recipe list grows large.
	TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAssets(RecipeIds);
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> RecipeObjects;
	Manager.GetPrimaryAssetObjectList(RecipeType, RecipeObjects);

	SmeltingRecipeCache.Reset();
	for (UObject* Object : RecipeObjects)
	{
		if (URecipeDefinition* Recipe = Cast<URecipeDefinition>(Object))
		{
			if (Recipe->RecipeTags.HasTag(AccidentalHeroGameplayTags::Recipe_Category_Smelting))
			{
				SmeltingRecipeCache.Add(Recipe);
			}
		}
	}
}
