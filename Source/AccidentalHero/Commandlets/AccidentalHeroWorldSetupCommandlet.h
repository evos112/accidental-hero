// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AccidentalHeroWorldSetupCommandlet.generated.h"

/**
 * Creates the initial World Partition-enabled persistent level for the seamless open world,
 * since there is no in-editor step (no Python plugin) to author the .umap otherwise.
 * Usage: UnrealEditor-Cmd.exe AccidentalHero -run=AccidentalHeroWorldSetup
 *
 * The class itself must NOT be wrapped in #if WITH_EDITOR: Unreal Header Tool generates
 * reflection glue for a UCLASS regardless of preprocessor guards around it, so a header-level
 * WITH_EDITOR guard leaves the generated .gen.cpp referencing a type that doesn't exist in
 * non-editor (Game target) builds, breaking packaging. Only Main()'s editor-only body (in the
 * .cpp) is guarded.
 */
UCLASS()
class UAccidentalHeroWorldSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
