// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Sprint.h"
#include "AbilitySystemComponent.h"
#include "AccidentalHeroAttributeSet.h"
#include "AccidentalHeroGameplayTags.h"
#include "GE_SprintSpeedBoost.h"
#include "GE_SprintStaminaDrain.h"

UGA_Sprint::UGA_Sprint()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(AccidentalHeroGameplayTags::Ability_Sprint));

	SprintSpeedBoostEffect = UGE_SprintSpeedBoost::StaticClass();
	SprintStaminaDrainEffect = UGE_SprintStaminaDrain::StaticClass();
}

bool UGA_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	return ASC->GetNumericAttribute(UAccidentalHeroAttributeSet::GetStaminaAttribute()) > 0.0f;
}

void UGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.SetAbility(this);

	if (SprintSpeedBoostEffect)
	{
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SprintSpeedBoostEffect, GetAbilityLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			SpeedBoostEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	if (SprintStaminaDrainEffect)
	{
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SprintStaminaDrainEffect, GetAbilityLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			StaminaDrainEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	ASC->AddLooseGameplayTag(AccidentalHeroGameplayTags::State_Sprinting);
}

void UGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (SpeedBoostEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(SpeedBoostEffectHandle);
		}
		if (StaminaDrainEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StaminaDrainEffectHandle);
		}
		ASC->RemoveLooseGameplayTag(AccidentalHeroGameplayTags::State_Sprinting);
	}

	SpeedBoostEffectHandle.Invalidate();
	StaminaDrainEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
