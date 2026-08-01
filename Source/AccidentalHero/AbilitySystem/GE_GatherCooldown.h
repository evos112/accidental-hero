// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_GatherCooldown.generated.h"

UCLASS()
class ACCIDENTALHERO_API UGE_GatherCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_GatherCooldown();

	virtual void PostInitProperties() override;
};
