// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroWorldSetupCommandlet.h"

#if WITH_EDITOR

#include "Editor.h"
#include "LevelEditorSubsystem.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogAccidentalHeroWorldSetup, Log, All);

#endif // WITH_EDITOR

int32 UAccidentalHeroWorldSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	static const FString MapAssetPath = TEXT("/Game/Maps/L_World");

	if (FPackageName::DoesPackageExist(MapAssetPath))
	{
		UE_LOG(LogAccidentalHeroWorldSetup, Log, TEXT("%s already exists, nothing to do."), *MapAssetPath);
		return 0;
	}

	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		UE_LOG(LogAccidentalHeroWorldSetup, Error, TEXT("No ULevelEditorSubsystem available."));
		return 1;
	}

	if (!LevelEditorSubsystem->NewLevel(MapAssetPath, /*bIsPartitionedWorld=*/true))
	{
		UE_LOG(LogAccidentalHeroWorldSetup, Error, TEXT("Failed to create partitioned level at %s."), *MapAssetPath);
		return 1;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogAccidentalHeroWorldSetup, Error, TEXT("New level was created but has no editor world."));
		return 1;
	}

	World->SpawnActor<APlayerStart>(FVector::ZeroVector, FRotator::ZeroRotator);

	if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapAssetPath))
	{
		UE_LOG(LogAccidentalHeroWorldSetup, Error, TEXT("Failed to save %s after adding PlayerStart."), *MapAssetPath);
		return 1;
	}

	UE_LOG(LogAccidentalHeroWorldSetup, Log, TEXT("Created World Partition level %s with a PlayerStart."), *MapAssetPath);
	return 0;
#else
	// This commandlet only makes sense run via UnrealEditor-Cmd.exe; non-editor (Game target)
	// builds still need Main() defined (UHT-generated glue references it unconditionally — see
	// the header comment), but it should never actually run in a packaged build.
	return 0;
#endif // WITH_EDITOR
}
