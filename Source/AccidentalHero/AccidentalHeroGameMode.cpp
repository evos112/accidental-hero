// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroGameMode.h"
#include "AccidentalHeroCharacter.h"
#include "AccidentalHeroPlayerController.h"
#include "AccidentalHeroPlayerState.h"
#include "Save/SaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

AAccidentalHeroGameMode::AAccidentalHeroGameMode()
{
	// Blueprint child so the UI events (crafting / crosshair / furnace) have an implementation.
	// Falls back to the C++ class if the asset is missing.
	static ConstructorHelpers::FClassFinder<APawn> PawnBP(TEXT("/Game/Blueprints/BP_AccidentalHeroCharacter"));
	if (PawnBP.Succeeded())
	{
		DefaultPawnClass = PawnBP.Class;
	}
	else
	{
		DefaultPawnClass = AAccidentalHeroCharacter::StaticClass();
	}
	PlayerControllerClass = AAccidentalHeroPlayerController::StaticClass();
	PlayerStateClass = AAccidentalHeroPlayerState::StaticClass();
}

void AAccidentalHeroGameMode::BeginPlay()
{
	Super::BeginPlay();

	// World restore happens here rather than on the character, because crops and beds belong to the
	// level, not the player — and there is exactly one level load per session.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSubsystem* Save = GameInstance->GetSubsystem<USaveSubsystem>())
		{
			Save->RestoreWorld(GetWorld());
		}
	}
}

void AAccidentalHeroGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSubsystem* Save = GameInstance->GetSubsystem<USaveSubsystem>())
		{
			Save->SaveGame();
		}
	}

	Super::EndPlay(EndPlayReason);
}
