// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmPlot.h"
#include "CropPlant.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AFarmPlot::AFarmPlot()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	// Walk-over, not walk-into: a low bed shouldn't block the player tending it.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = MeshComponent;
}

void AFarmPlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AFarmPlot::BeginPlay()
{
	Super::BeginPlay();
	SlotOccupants.SetNum(GetSlotCount());
}

FTransform AFarmPlot::GetSlotTransform(int32 SlotIndex) const
{
	const int32 Count = GetSlotCount();
	if (SlotIndex < 0 || SlotIndex >= Count)
	{
		return GetActorTransform();
	}

	const int32 Row = SlotIndex / FMath::Max(1, Columns);
	const int32 Column = SlotIndex % FMath::Max(1, Columns);

	// Centre the grid on the plot so the bed mesh and its rows share an origin.
	const float OffsetX = (Row - (Rows - 1) * 0.5f) * SlotSpacing;
	const float OffsetY = (Column - (Columns - 1) * 0.5f) * SlotSpacing;

	const FTransform PlotTransform = GetActorTransform();
	const FVector Local(OffsetX, OffsetY, 0.0f);
	return FTransform(PlotTransform.GetRotation(), PlotTransform.TransformPosition(Local));
}

int32 AFarmPlot::FindFreeSlot() const
{
	const int32 Count = GetSlotCount();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// A slot whose crop has been destroyed is free again, so a dead plant doesn't waste a bed.
		if (!SlotOccupants.IsValidIndex(Index) || !IsValid(SlotOccupants[Index]))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 AFarmPlot::FindSlotOf(const ACropPlant* Crop) const
{
	return Crop ? SlotOccupants.IndexOfByKey(Crop) : INDEX_NONE;
}

void AFarmPlot::OccupySlot(int32 SlotIndex, ACropPlant* Crop)
{
	if (SlotIndex < 0 || SlotIndex >= GetSlotCount() || !Crop)
	{
		return;
	}

	if (SlotOccupants.Num() < GetSlotCount())
	{
		SlotOccupants.SetNum(GetSlotCount());
	}

	SlotOccupants[SlotIndex] = Crop;
	Crop->SetPlantedInPlot(this);
}
