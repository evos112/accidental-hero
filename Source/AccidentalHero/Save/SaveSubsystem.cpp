// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSubsystem.h"
#include "AccidentalHeroSaveGame.h"
#include "AccidentalHeroCharacter.h"
#include "AbilitySystem/AccidentalHeroAttributeSet.h"
#include "Items/CropPlant.h"
#include "Items/FarmPlot.h"
#include "Items/InventoryComponent.h"
#include "Items/CraftingComponent.h"
#include "Items/ItemDefinition.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

const TCHAR* USaveSubsystem::SlotName = TEXT("AccidentalHero");

namespace
{
	constexpr float AutosaveIntervalSeconds = 120.0f;
}

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadGame();
}

void USaveSubsystem::Deinitialize()
{
	// Last line of defence: quitting the game, ending PIE, or travelling all land here.
	SaveGame();

	if (UWorld* World = TimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(AutosaveHandle);
	}

	Super::Deinitialize();
}

bool USaveSubsystem::LoadGame()
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	UAccidentalHeroSaveGame* Loaded =
		Cast<UAccidentalHeroSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));

	if (!Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("Save slot '%s' could not be read; starting fresh."), SlotName);
		return false;
	}

	// A save from a different layout is refused outright rather than half-applied. Silently
	// loading mismatched data is worse than losing it, because the damage isn't obvious.
	if (Loaded->SaveVersion != UAccidentalHeroSaveGame::CurrentVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Save is version %d but this build expects %d; ignoring it."),
			Loaded->SaveVersion, UAccidentalHeroSaveGame::CurrentVersion);
		return false;
	}

	LoadedSave = Loaded;
	return true;
}

void USaveSubsystem::DeleteSave()
{
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	LoadedSave = nullptr;
	bWorldRestored = false;
}

bool USaveSubsystem::SaveGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AAccidentalHeroCharacter* Character =
		Cast<AAccidentalHeroCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));

	// Nothing to record before the player exists; writing now would blank a good save.
	if (!Character)
	{
		return false;
	}

	UAccidentalHeroSaveGame* Save = NewObject<UAccidentalHeroSaveGame>(this);
	Save->SaveVersion = UAccidentalHeroSaveGame::CurrentVersion;
	Save->SavedAtUtc = FDateTime::UtcNow();

	Save->PlayerLocation = Character->GetActorLocation();
	Save->PlayerRotation = Character->GetActorRotation();
	Save->bIsFirstPerson = Character->IsFirstPerson();

	if (const UAccidentalHeroAttributeSet* Attributes = Character->GetAttributeSet())
	{
		Save->Health = Attributes->GetHealth();
		Save->Stamina = Attributes->GetStamina();
		Save->Hunger = Attributes->GetHunger();
		Save->Thirst = Attributes->GetThirst();
	}

	if (const UInventoryComponent* Inventory = Character->GetInventoryComponent())
	{
		for (const FInventoryItemEntry& Entry : Inventory->GetAllItems())
		{
			if (!Entry.ItemDef || Entry.StackCount <= 0)
			{
				continue;
			}
			FSavedItemStack& Stack = Save->Items.AddDefaulted_GetRef();
			Stack.ItemDef = Entry.ItemDef;
			Stack.StackCount = Entry.StackCount;
			Stack.Durability = Entry.Durability;
		}

		for (int32 Index = 0; Index < UInventoryComponent::HotbarSlotCount; ++Index)
		{
			Save->HotbarSlots.Add(Inventory->GetHotbarItem(Index));
		}
	}

	// Plots first, so each crop can point at one by index.
	TMap<AFarmPlot*, int32> PlotIndices;
	for (TActorIterator<AFarmPlot> It(World); It; ++It)
	{
		AFarmPlot* Plot = *It;
		if (!IsValid(Plot))
		{
			continue;
		}
		FSavedFarmPlot& Saved = Save->Plots.AddDefaulted_GetRef();
		Saved.PlotClass = Plot->GetClass();
		Saved.Transform = Plot->GetActorTransform();
		PlotIndices.Add(Plot, Save->Plots.Num() - 1);
	}

	for (TActorIterator<ACropPlant> It(World); It; ++It)
	{
		ACropPlant* Crop = *It;
		if (!IsValid(Crop))
		{
			continue;
		}

		FSavedCrop& Saved = Save->Crops.AddDefaulted_GetRef();
		Saved.CropClass = Crop->GetClass();
		Saved.Transform = Crop->GetActorTransform();
		Crop->GetSaveState(Saved.GrowthStage, Saved.StageProgress, Saved.WaterLevel,
			Saved.Vitality, Saved.bWithered);

		if (AFarmPlot* Home = Crop->GetHomePlot())
		{
			if (const int32* Found = PlotIndices.Find(Home))
			{
				Saved.PlotIndex = *Found;
				Saved.PlotSlot = Home->FindSlotOf(Crop);
			}
		}
	}

	const bool bWritten = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	if (bWritten)
	{
		LoadedSave = Save;
	}
	return bWritten;
}

void USaveSubsystem::RestoreWorld(UWorld* World)
{
	if (!World || !LoadedSave || bWorldRestored)
	{
		return;
	}
	bWorldRestored = true;

	// Start the autosave clock here, once there's a level to save.
	TimerWorld = World;
	World->GetTimerManager().SetTimer(AutosaveHandle, this, &USaveSubsystem::HandleAutosaveTimer,
		AutosaveIntervalSeconds, true);

	// Everything in the save replaces what the level shipped with, rather than adding to it —
	// otherwise the tutorial patch's own bed and bushes would duplicate on every load.
	TArray<AActor*> Existing;
	for (TActorIterator<ACropPlant> It(World); It; ++It) { Existing.Add(*It); }
	for (TActorIterator<AFarmPlot> It(World); It; ++It) { Existing.Add(*It); }
	for (AActor* Actor : Existing)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AFarmPlot*> SpawnedPlots;
	SpawnedPlots.Reserve(LoadedSave->Plots.Num());
	for (const FSavedFarmPlot& Saved : LoadedSave->Plots)
	{
		UClass* PlotClass = Saved.PlotClass.LoadSynchronous();
		AFarmPlot* Plot = PlotClass
			? World->SpawnActor<AFarmPlot>(PlotClass, Saved.Transform, SpawnParams)
			: nullptr;
		SpawnedPlots.Add(Plot);
	}

	for (const FSavedCrop& Saved : LoadedSave->Crops)
	{
		UClass* CropClass = Saved.CropClass.LoadSynchronous();
		if (!CropClass)
		{
			continue;
		}

		ACropPlant* Crop = World->SpawnActor<ACropPlant>(CropClass, Saved.Transform, SpawnParams);
		if (!Crop)
		{
			continue;
		}

		// After spawn, so this overwrites BeginPlay's starting water and ripeness.
		Crop->ApplySaveState(Saved.GrowthStage, Saved.StageProgress, Saved.WaterLevel,
			Saved.Vitality, Saved.bWithered);

		if (SpawnedPlots.IsValidIndex(Saved.PlotIndex) && Saved.PlotSlot != INDEX_NONE)
		{
			if (AFarmPlot* Plot = SpawnedPlots[Saved.PlotIndex])
			{
				Plot->OccupySlot(Saved.PlotSlot, Crop);
			}
		}
	}
}

void USaveSubsystem::RestorePlayer(AAccidentalHeroCharacter* Character)
{
	if (!LoadedSave || !Character || !Character->HasAuthority())
	{
		return;
	}

	Character->SetActorLocation(LoadedSave->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Character->SetActorRotation(LoadedSave->PlayerRotation);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(LoadedSave->PlayerRotation);
	}

	if (UAccidentalHeroAttributeSet* Attributes = Character->GetMutableAttributeSet())
	{
		// Never load in dead. Direct setters bypass PostGameplayEffectExecute, so a saved zero
		// would leave the player alive at 0 HP with no death ever triggering — a soft-lock where
		// the next scratch kills you instantly. A save can legitimately contain zero if the game
		// was quit mid-death, so this has to be handled rather than assumed away.
		Attributes->SetHealth(LoadedSave->Health > 0.0f
			? LoadedSave->Health
			: Attributes->GetMaxHealth());
		Attributes->SetStamina(LoadedSave->Stamina);
		Attributes->SetHunger(LoadedSave->Hunger);
		Attributes->SetThirst(LoadedSave->Thirst);
	}

	if (UInventoryComponent* Inventory = Character->GetInventoryComponent())
	{
		// The character may already have been given starting items; clear before restoring so a
		// load never stacks on top of a fresh loadout.
		for (const FInventoryItemEntry& Entry : Inventory->GetAllItems())
		{
			if (Entry.ItemDef)
			{
				Inventory->RemoveItem(Entry.ItemDef, Entry.StackCount);
			}
		}

		for (const FSavedItemStack& Stack : LoadedSave->Items)
		{
			if (UItemDefinition* ItemDef = Stack.ItemDef.LoadSynchronous())
			{
				Inventory->AddItem(ItemDef, Stack.StackCount, Stack.Durability);
			}
		}

		// After the items, because AddItem auto-binds new item types to free keys and would
		// otherwise fight the saved layout.
		for (int32 Index = 0; Index < LoadedSave->HotbarSlots.Num(); ++Index)
		{
			Inventory->AssignHotbarSlot(Index, LoadedSave->HotbarSlots[Index].LoadSynchronous());
		}
	}

	if (LoadedSave->bIsFirstPerson != Character->IsFirstPerson())
	{
		Character->SetFirstPerson(LoadedSave->bIsFirstPerson);
	}

	// Saving after a craft means a successful craft is never lost to a crash.
	if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		if (const APlayerState* PS = PC->PlayerState)
		{
			if (UCraftingComponent* Crafting = PS->FindComponentByClass<UCraftingComponent>())
			{
				Crafting->OnCraftingResult.RemoveDynamic(this, &USaveSubsystem::HandleCraftingResult);
				Crafting->OnCraftingResult.AddDynamic(this, &USaveSubsystem::HandleCraftingResult);
			}
		}
	}
}

void USaveSubsystem::HandleCraftingResult(URecipeDefinition* Recipe, bool bSuccess)
{
	if (bSuccess)
	{
		SaveGame();
	}
}

void USaveSubsystem::HandleAutosaveTimer()
{
	SaveGame();
}
