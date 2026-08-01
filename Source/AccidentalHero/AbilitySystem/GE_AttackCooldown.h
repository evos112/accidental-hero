// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_AttackCooldown.generated.h"

UCLASS()
class ACCIDENTALHERO_API UGE_AttackCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_AttackCooldown();

	virtual void PostInitProperties() override;
};
