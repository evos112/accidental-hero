// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AccidentalHeroGameMode.generated.h"

UCLASS()
class ACCIDENTALHERO_API AAccidentalHeroGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAccidentalHeroGameMode();

protected:
	/** Restores saved crops and farm plots once the level is up. */
	virtual void BeginPlay() override;

	/** Saves while the world is still intact. The GameInstance's own shutdown is too late — by
	 *  then the pawn is gone and there is nothing left to record. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
