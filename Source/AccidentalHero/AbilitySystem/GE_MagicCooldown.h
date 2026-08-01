// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_MagicCooldown.generated.h"

UCLASS()
class ACCIDENTALHERO_API UGE_MagicCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_MagicCooldown();

	virtual void PostInitProperties() override;
};
