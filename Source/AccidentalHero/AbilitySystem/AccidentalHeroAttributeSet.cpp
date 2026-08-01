// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "AccidentalHeroCharacter.h"
#include "AccidentalHeroGameplayTags.h"
#include "GameFramework/PlayerState.h"
#include "GE_Starvation.h"
#include "Net/UnrealNetwork.h"

UAccidentalHeroAttributeSet::UAccidentalHeroAttributeSet()
{
	// 340 uu/s = 12.2 km/h — a steady jog. Sprint adds on top of this (see UGE_SprintSpeedBoost).
	InitMoveSpeed(340.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitStaminaRegenRate(10.0f);
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitMana(100.0f);
	InitMaxMana(100.0f);
	InitManaRegenRate(10.0f);
	InitHunger(100.0f);
	InitMaxHunger(100.0f);
	InitThirst(100.0f);
	InitMaxThirst(100.0f);
}

void UAccidentalHeroAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MaxHunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, Thirst, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAccidentalHeroAttributeSet, MaxThirst, COND_None, REPNOTIFY_Always);
}

void UAccidentalHeroAttributeSet::OnRep_Hunger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, Hunger, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_MaxHunger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MaxHunger, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_Thirst(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, Thirst, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_MaxThirst(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MaxThirst, OldValue);
}

void UAccidentalHeroAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetHungerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHunger());
	}
	else if (Attribute == GetThirstAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxThirst());
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
}

void UAccidentalHeroAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Max(GetMoveSpeed(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		// Death is detected here rather than polled: every route to zero health — falling,
		// starvation, dehydration — lands in this function.
		if (GetHealth() <= 0.0f)
		{
			if (AAccidentalHeroCharacter* Character = GetOwningCharacter())
			{
				Character->HandleDeath();
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}

	// Hunger/Thirst are clamped in PreAttributeChange, so by here they hold their final value.
	// UGE_SurvivalDrain ticks once a second, which makes this the natural place to notice an empty
	// bar without adding a tick to the character.
	if (Data.EvaluatedData.Attribute == GetHungerAttribute())
	{
		// PreAttributeChange only clamps the current value, so an empty bar's *base* keeps sliding
		// negative while the drain ticks. Left alone, a player who starved for ten minutes would
		// have to eat off that debt before the bar moved at all.
		SetHunger(FMath::Clamp(GetHunger(), 0.0f, GetMaxHunger()));

		UpdateSurvivalCondition(GetHunger(), AccidentalHeroGameplayTags::State_Starving,
			UGE_Starvation::StaticClass(), StarvationHandle);
	}
	else if (Data.EvaluatedData.Attribute == GetThirstAttribute())
	{
		SetThirst(FMath::Clamp(GetThirst(), 0.0f, GetMaxThirst()));

		UpdateSurvivalCondition(GetThirst(), AccidentalHeroGameplayTags::State_Dehydrated,
			UGE_Dehydration::StaticClass(), DehydrationHandle);
	}
}

AAccidentalHeroCharacter* UAccidentalHeroAttributeSet::GetOwningCharacter() const
{
	// The ability system lives on the PlayerState, so the pawn is one hop away — but handle the
	// case where a future actor owns its own ASC directly.
	AActor* OwnerActor = GetOwningActor();
	if (const APlayerState* PlayerState = Cast<APlayerState>(OwnerActor))
	{
		return Cast<AAccidentalHeroCharacter>(PlayerState->GetPawn());
	}
	return Cast<AAccidentalHeroCharacter>(OwnerActor);
}

void UAccidentalHeroAttributeSet::UpdateSurvivalCondition(float CurrentValue, const FGameplayTag& StateTag,
	TSubclassOf<UGameplayEffect> ConditionEffect, FActiveGameplayEffectHandle& ConditionHandle)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC || !ConditionEffect)
	{
		return;
	}

	// The handle is the source of truth for "already applied", so the effect can never stack even
	// though it grants no tag of its own.
	const bool bEmpty = CurrentValue <= 0.0f;
	const bool bApplied = ConditionHandle.IsValid();

	if (bEmpty && !bApplied)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ConditionEffect, 1.0f, Context);
		if (Spec.IsValid())
		{
			ConditionHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			ASC->AddLooseGameplayTag(StateTag);
		}
	}
	else if (!bEmpty && bApplied)
	{
		// Eating or drinking anything at all lifts the condition immediately.
		ASC->RemoveActiveGameplayEffect(ConditionHandle);
		ASC->RemoveLooseGameplayTag(StateTag);
		ConditionHandle.Invalidate();
	}
}

void UAccidentalHeroAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MoveSpeed, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, Stamina, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MaxStamina, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, StaminaRegenRate, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, Health, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MaxHealth, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, Mana, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, MaxMana, OldValue);
}

void UAccidentalHeroAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAccidentalHeroAttributeSet, ManaRegenRate, OldValue);
}
