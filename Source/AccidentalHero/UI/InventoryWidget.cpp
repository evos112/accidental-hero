// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryWidget.h"
#include "HotbarSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Items/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "Items/InventoryTypes.h"
#include "AbilitySystem/AccidentalHeroAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

template <typename T>
T* UInventoryWidget::Find(const FString& Name) const
{
	return WidgetTree ? Cast<T>(WidgetTree->FindWidget(FName(*Name))) : nullptr;
}

UInventoryComponent* UInventoryWidget::GetInventory() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	// Inventory lives on the PlayerState, not the pawn, so it survives respawn.
	return PS ? PS->FindComponentByClass<UInventoryComponent>() : nullptr;
}

const UAccidentalHeroAttributeSet* UInventoryWidget::GetAttributes() const
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	const IAbilitySystemInterface* AsInterface = Cast<IAbilitySystemInterface>(PS);
	const UAbilitySystemComponent* ASC = AsInterface ? AsInterface->GetAbilitySystemComponent() : nullptr;
	return ASC ? ASC->GetSet<UAccidentalHeroAttributeSet>() : nullptr;
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UInventoryComponent* Inventory = GetInventory())
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	RefreshAll();
}

void UInventoryWidget::NativeDestruct()
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
		BoundInventory = nullptr;
	}

	Super::NativeDestruct();
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceMeterRefresh += InDeltaTime;
	if (TimeSinceMeterRefresh >= MeterRefreshInterval)
	{
		TimeSinceMeterRefresh = 0.0f;
		RefreshMeters();
	}
}

void UInventoryWidget::HandleInventoryChanged()
{
	RefreshSlots();
	RefreshMeters();
}

void UInventoryWidget::RefreshAll()
{
	RefreshSlots();
	RefreshMeters();
	RefreshEquipment();
	ShowItemDetail(nullptr);
}

void UInventoryWidget::RefreshEquipment()
{
	// Only the Tool slot is real. The doll's other seven (Head/Chest/Legs/Feet/Hands/Back/Primary)
	// stay as drawn placeholders because the game has no armour or weapons to put in them — wiring
	// them to nothing would just be a mockup that looks functional.
	if (UTextBlock* ToolText = Find<UTextBlock>(TEXT("EqToolTx")))
	{
		const UInventoryComponent* Inventory = GetInventory();
		UItemDefinition* Equipped = Inventory ? Inventory->GetEquippedItem() : nullptr;
		ToolText->SetText(Equipped ? Equipped->DisplayName : FText::FromString(TEXT("Tool")));
	}
}

void UInventoryWidget::RefreshSlots()
{
	const UInventoryComponent* Inventory = GetInventory();
	const TArray<FInventoryItemEntry> Items = Inventory ? Inventory->GetAllItems() : TArray<FInventoryItemEntry>();
	const int32 MaxSlots = Inventory ? Inventory->GetMaxSlots() : SlotCount;

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		const FString Suffix = FString::Printf(TEXT("%02d"), Index);
		UTextBlock* NameText = Find<UTextBlock>(TEXT("SlotName") + Suffix);
		UTextBlock* QtyText = Find<UTextBlock>(TEXT("SlotQty") + Suffix);
		UImage* IconImage = Find<UImage>(TEXT("SlotIcon") + Suffix);
		UImage* SlotBg = Find<UImage>(TEXT("Slot") + Suffix);

		const bool bUsable = Index < MaxSlots;
		const FInventoryItemEntry* Entry = Items.IsValidIndex(Index) ? &Items[Index] : nullptr;
		const UItemDefinition* ItemDef = Entry ? Entry->ItemDef : nullptr;

		if (NameText)
		{
			NameText->SetText(ItemDef ? ItemDef->DisplayName : FText::GetEmpty());
		}

		if (QtyText)
		{
			// A count is only meaningful on a stack of more than one.
			QtyText->SetText(Entry && Entry->StackCount > 1
				? FText::AsNumber(Entry->StackCount)
				: FText::GetEmpty());
		}

		if (IconImage)
		{
			// Items have no authored icons yet; the slot reads as name + count until they do.
			UTexture2D* Icon = ItemDef ? ItemDef->Icon.LoadSynchronous() : nullptr;
			if (Icon)
			{
				IconImage->SetBrushFromTexture(Icon, /*bMatchSize=*/false);
			}
			IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (SlotBg)
		{
			// Slots past the inventory's capacity are dimmed rather than hidden, so the grid keeps
			// its shape and the player can see what more capacity would buy them.
			SlotBg->SetOpacity(bUsable ? (ItemDef ? 1.0f : 0.55f) : 0.15f);
		}

		// Invisible drag handle laid over each grid cell, so a stack can be dragged onto a quick-bar
		// key. Drop-target behaviour is off — the grid is a source only.
		if (UHotbarSlotWidget* Handle = Find<UHotbarSlotWidget>(TEXT("SlotDrag") + Suffix))
		{
			Handle->SetSlotIndex(INDEX_NONE);
			Handle->SetSlotContent(const_cast<UItemDefinition*>(ItemDef), Entry ? Entry->StackCount : 0);
		}
	}

	// Quick bar shows its actual assignments, with the live count of whatever each slot points at.
	for (int32 Index = 0; Index < HotbarCount; ++Index)
	{
		UItemDefinition* Bound = Inventory ? Inventory->GetHotbarItem(Index) : nullptr;
		const int32 Held = (Bound && Inventory) ? Inventory->GetItemCount(Bound) : 0;

		if (UTextBlock* HotName = Find<UTextBlock>(FString::Printf(TEXT("Hot%dName"), Index)))
		{
			HotName->SetText(Bound
				? (Held > 1
					? FText::FromString(FString::Printf(TEXT("%s  %d"), *Bound->DisplayName.ToString(), Held))
					: Bound->DisplayName)
				: FText::GetEmpty());
		}

		// An empty slot, or one whose item you've run out of, reads dimmer than a usable key.
		if (UImage* HotBg = Find<UImage>(FString::Printf(TEXT("Hot%dBg"), Index)))
		{
			HotBg->SetOpacity(Bound ? (Held > 0 ? 1.0f : 0.45f) : 0.35f);
		}
	}

	if (UTextBlock* Sub = Find<UTextBlock>(TEXT("Sub")))
	{
		Sub->SetText(FText::FromString(FString::Printf(TEXT("%d / %d slots used"), Items.Num(), MaxSlots)));
	}
}

void UInventoryWidget::ApplyMeter(const FString& Prefix, float Current, float Max, bool bShowAsWeight)
{
	const float Ratio = Max > KINDA_SMALL_NUMBER ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f;

	if (UTextBlock* Value = Find<UTextBlock>(Prefix + TEXT("V")))
	{
		Value->SetText(FText::FromString(bShowAsWeight
			? FString::Printf(TEXT("%.1f / %.0f kg"), Current, Max)
			: FString::Printf(TEXT("%.0f / %.0f"), Current, Max)));
	}

	// The fill bar is a plain Image sitting on the track image; scaling its canvas slot width is
	// what makes it read as a bar. Track width is the authored full-width reference.
	UImage* Fill = Find<UImage>(Prefix + TEXT("F"));
	UImage* Track = Find<UImage>(Prefix + TEXT("T"));
	if (!Fill || !Track)
	{
		return;
	}

	const UCanvasPanelSlot* TrackSlot = Cast<UCanvasPanelSlot>(Track->Slot);
	UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(Fill->Slot);
	if (TrackSlot && FillSlot)
	{
		const FVector2D FullSize = TrackSlot->GetSize();
		FillSlot->SetSize(FVector2D(FullSize.X * Ratio, FullSize.Y));
	}
}

void UInventoryWidget::RefreshMeters()
{
	if (const UAccidentalHeroAttributeSet* Attributes = GetAttributes())
	{
		ApplyMeter(TEXT("MHealth"), Attributes->GetHealth(), Attributes->GetMaxHealth());
		ApplyMeter(TEXT("MStamina"), Attributes->GetStamina(), Attributes->GetMaxStamina());
		ApplyMeter(TEXT("MHunger"), Attributes->GetHunger(), Attributes->GetMaxHunger());
		ApplyMeter(TEXT("MThirst"), Attributes->GetThirst(), Attributes->GetMaxThirst());
	}

	if (const UInventoryComponent* Inventory = GetInventory())
	{
		ApplyMeter(TEXT("MCarry"), Inventory->GetTotalWeight(), CarryCapacity, /*bShowAsWeight=*/true);
	}
}

void UInventoryWidget::ShowItemDetail(UItemDefinition* Item)
{
	if (UTextBlock* Name = Find<UTextBlock>(TEXT("DetName")))
	{
		Name->SetText(Item ? Item->DisplayName : FText::FromString(TEXT("Select an item")));
	}

	if (UTextBlock* Desc = Find<UTextBlock>(TEXT("DetDesc")))
	{
		Desc->SetText(Item ? Item->Description : FText::GetEmpty());
	}

	if (UTextBlock* Category = Find<UTextBlock>(TEXT("DetCat")))
	{
		FString CategoryText;
		if (Item)
		{
			// First category tag reads better than the whole container; tools show their tier.
			TArray<FGameplayTag> Tags;
			Item->ItemTags.GetGameplayTagArray(Tags);
			CategoryText = Tags.Num() > 0 ? Tags[0].ToString() : TEXT("Item");
			if (Item->ToolTier > 0)
			{
				CategoryText += FString::Printf(TEXT("  (Tier %d)"), Item->ToolTier);
			}
		}
		Category->SetText(FText::FromString(CategoryText));
	}

	// Three generic stat rows, filled with whatever this item actually has.
	TArray<TPair<FString, FString>> Stats;
	if (Item)
	{
		// Durability leads for tools — it's the thing you check before heading out.
		if (Item->MaxDurability > 0)
		{
			const UInventoryComponent* Inventory = GetInventory();
			const int32 Current = Inventory ? Inventory->GetItemDurability(Item) : -1;
			Stats.Emplace(TEXT("Durability"), Current >= 0
				? FString::Printf(TEXT("%d / %d"), Current, Item->MaxDurability)
				: FString::Printf(TEXT("%d"), Item->MaxDurability));
		}

		Stats.Emplace(TEXT("Weight"), FString::Printf(TEXT("%.2f kg"), Item->Weight));
		Stats.Emplace(TEXT("Stack"), FString::Printf(TEXT("%d"), Item->MaxStackSize));
		if (Item->IsConsumable())
		{
			Stats.Emplace(TEXT("Restores"), FString::Printf(TEXT("%.0f hunger / %.0f thirst"),
				Item->HungerRestore, Item->ThirstRestore));
		}
		else if (Item->ToolTier > 0)
		{
			Stats.Emplace(TEXT("Tool tier"), FString::Printf(TEXT("%d"), Item->ToolTier));
		}
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UTextBlock* Label = Find<UTextBlock>(FString::Printf(TEXT("DS%dL"), Index));
		UTextBlock* Value = Find<UTextBlock>(FString::Printf(TEXT("DS%dV"), Index));
		const bool bHasStat = Stats.IsValidIndex(Index);
		if (Label)
		{
			Label->SetText(bHasStat ? FText::FromString(Stats[Index].Key) : FText::GetEmpty());
		}
		if (Value)
		{
			Value->SetText(bHasStat ? FText::FromString(Stats[Index].Value) : FText::GetEmpty());
		}
	}

	if (UImage* Icon = Find<UImage>(TEXT("DetIcon")))
	{
		UTexture2D* Texture = Item ? Item->Icon.LoadSynchronous() : nullptr;
		if (Texture)
		{
			Icon->SetBrushFromTexture(Texture, /*bMatchSize=*/false);
		}
		Icon->SetOpacity(Texture ? 1.0f : 0.25f);
	}
}
