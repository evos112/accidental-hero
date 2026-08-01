// Copyright Epic Games, Inc. All Rights Reserved.

#include "CropPlant.h"
#include "ItemDefinition.h"
#include "ItemPickup.h"
#include "AccidentalHeroCharacter.h"
#include "FarmPlot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

ACropPlant::ACropPlant()
{
	// One swing takes a ripe crop; the work is the tending, not the hitting.
	MaxHits = 1;

	StatusWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidget"));
	StatusWidget->SetupAttachment(RootComponent);
	StatusWidget->SetWidgetSpace(EWidgetSpace::Screen);
	StatusWidget->SetDrawSize(FVector2D(150.0f, 44.0f));
	StatusWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	// Only readable up close, so a field of crops isn't a wall of floating bars.
	StatusWidget->SetCullDistance(1500.0f);
	StatusWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACropPlant::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACropPlant, GrowthStage);
	DOREPLIFETIME(ACropPlant, StageProgress);
	DOREPLIFETIME(ACropPlant, WaterLevel);
	DOREPLIFETIME(ACropPlant, Vitality);
	DOREPLIFETIME(ACropPlant, bWithered);
}

void ACropPlant::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (bStartRipe)
		{
			GrowthStage = FMath::Max(0, GrowthStageMeshes.Num() - 1);
		}

		// A sown seed goes in damp; a wild plant is simply never thirsty.
		WaterLevel = bRequiresWater ? MaxWater * 0.5f : MaxWater;

		GetWorldTimerManager().SetTimer(TendTimerHandle, this, &ACropPlant::TendTick, 1.0f, true);
	}

	ApplyGrowthVisual();
}

float ACropPlant::GetGrowthFraction() const
{
	const int32 FinalStage = FMath::Max(1, GrowthStageMeshes.Num() - 1);
	const float Within = SecondsPerStage > 0.0f ? FMath::Clamp(StageProgress / SecondsPerStage, 0.0f, 1.0f) : 0.0f;
	return FMath::Clamp((GrowthStage + Within) / FinalStage, 0.0f, 1.0f);
}

FText ACropPlant::GetCropName() const
{
	// Named after what it produces, which is what the player actually cares about.
	return OutputItem ? OutputItem->DisplayName : FText::FromString(TEXT("Plant"));
}

void ACropPlant::GetSaveState(int32& OutGrowthStage, float& OutStageProgress, float& OutWaterLevel,
	float& OutVitality, bool& bOutWithered) const
{
	OutGrowthStage = GrowthStage;
	OutStageProgress = StageProgress;
	OutWaterLevel = WaterLevel;
	OutVitality = Vitality;
	bOutWithered = bWithered;
}

void ACropPlant::ApplySaveState(int32 InGrowthStage, float InStageProgress, float InWaterLevel,
	float InVitality, bool bInWithered)
{
	GrowthStage = FMath::Clamp(InGrowthStage, 0, FMath::Max(0, GrowthStageMeshes.Num() - 1));
	StageProgress = FMath::Max(0.0f, InStageProgress);
	WaterLevel = FMath::Clamp(InWaterLevel, 0.0f, MaxWater);
	Vitality = FMath::Clamp(InVitality, 0.0f, 100.0f);
	bWithered = bInWithered;

	if (bWithered)
	{
		// A dead plant stays dead: stop the tick that BeginPlay started and make it unharvestable.
		HitsRemaining = 0;
		GetWorldTimerManager().ClearTimer(TendTimerHandle);
	}

	ApplyGrowthVisual();
}

bool ACropPlant::AddWater(float Amount)
{
	if (!HasAuthority() || bWithered || Amount <= 0.0f)
	{
		return false;
	}

	if (WaterLevel >= MaxWater - KINDA_SMALL_NUMBER)
	{
		return false;
	}

	WaterLevel = FMath::Clamp(WaterLevel + Amount, 0.0f, MaxWater);
	return true;
}

void ACropPlant::TendTick()
{
	if (!HasAuthority() || bWithered)
	{
		return;
	}

	// Wild plants don't need tending; they just grow.
	const bool bHasWater = !bRequiresWater || WaterLevel > 0.0f;

	if (bRequiresWater)
	{
		// Tilled soil holds moisture, so a crop in a bed needs revisiting far less often. That
		// difference is the whole reason to spend planks on beds.
		const float Retention = HomePlot ? HomePlot->GetMoistureRetention() : 1.0f;
		WaterLevel = FMath::Max(0.0f, WaterLevel - WaterDrainPerSecond * Retention);
	}

	if (bHasWater)
	{
		Vitality = FMath::Min(100.0f, Vitality + RecoveryPerSecond);

		// Only watered seconds count toward growth — that is the whole reason to come back.
		if (!IsRipe())
		{
			StageProgress += 1.0f;
			if (StageProgress >= SecondsPerStage)
			{
				StageProgress = 0.0f;
				++GrowthStage;
				ApplyGrowthVisual();
			}
		}
	}
	else
	{
		Vitality -= DecayPerSecond;
		if (Vitality <= 0.0f)
		{
			Vitality = 0.0f;
			bWithered = true;
			// A dead crop stops being harvestable but stays in the world as a visible mistake.
			HitsRemaining = 0;
			GetWorldTimerManager().ClearTimer(TendTimerHandle);
		}
	}
}

void ACropPlant::OnRep_GrowthStage()
{
	ApplyGrowthVisual();
}

void ACropPlant::OnRep_CropState()
{
	// Water/vitality drive the floating readout, which reads them straight off this actor.
}

void ACropPlant::ApplyGrowthVisual()
{
	if (!MeshComponent || GrowthStageMeshes.Num() == 0)
	{
		return;
	}

	const int32 Index = FMath::Clamp(GrowthStage, 0, GrowthStageMeshes.Num() - 1);
	if (UStaticMesh* StageMesh = GrowthStageMeshes[Index])
	{
		MeshComponent->SetStaticMesh(StageMesh);
	}
}

bool ACropPlant::Harvest(AAccidentalHeroCharacter* Player)
{
	if (!HasAuthority() || !Player || !IsInRange(Player))
	{
		return false;
	}

	if (bWithered)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("This plant has withered."));
		}
		return false;
	}

	if (!IsRipe())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Not ready to harvest."));
		}
		return false;
	}

	// Produce comes from the base implementation (OutputItem x YieldPerHit).
	const bool bHarvested = Super::Harvest(Player);

	if (bHarvested && SeedItem && SeedYield > 0)
	{
		// Seeds alongside the produce, so harvesting funds the next planting.
		AItemPickup::SpawnItemPickup(this, SeedItem, SeedYield,
			FTransform(GetActorLocation() + FVector(0.0f, 0.0f, 20.0f)));
	}

	if (bHarvested)
	{
		// Back to a seedling and regrow, rather than the base class's hide-and-respawn.
		GrowthStage = 0;
		StageProgress = 0.0f;
		ApplyGrowthVisual();
	}
	return bHarvested;
}
