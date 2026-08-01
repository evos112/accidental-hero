// Copyright Epic Games, Inc. All Rights Reserved.

#include "BiomeRegion.h"

#include "BiomeSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ABiomeRegion::ABiomeRegion()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	// An AActor with no RootComponent cannot be placed: SetActorLocation silently does nothing and
	// GetActorLocation always returns the origin. Without this the region sits at (0,0,0) however it
	// is spawned, and anything driven off its location lands in the corner of the map.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

bool ABiomeRegion::ContainsLocation(const FVector& WorldLocation) const
{
	return FVector::DistSquared2D(GetActorLocation(), WorldLocation) <= FMath::Square(Radius);
}

void ABiomeRegion::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UBiomeSubsystem* Subsystem = World->GetSubsystem<UBiomeSubsystem>())
		{
			Subsystem->RegisterRegion(this);
		}
	}
}

void ABiomeRegion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UBiomeSubsystem* Subsystem = World->GetSubsystem<UBiomeSubsystem>())
		{
			Subsystem->UnregisterRegion(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
