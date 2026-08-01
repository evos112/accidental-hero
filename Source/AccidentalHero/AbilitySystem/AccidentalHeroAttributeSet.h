// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AccidentalHeroAttributeSet.generated.h"

#define ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class ACCIDENTALHERO_API UAccidentalHeroAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAccidentalHeroAttributeSet();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Movement")
	FGameplayAttributeData Stamina;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Movement")
	FGameplayAttributeData MaxStamina;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Movement")
	FGameplayAttributeData StaminaRegenRate;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, StaminaRegenRate)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Combat")
	FGameplayAttributeData Health;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Combat")
	FGameplayAttributeData MaxHealth;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, MaxHealth)

	/** Survival needs. Drain over time and are refilled by consumables; the inventory panel shows
	 *  them alongside Health/Stamina so the player has one place to read their condition. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Hunger, Category = "Survival")
	FGameplayAttributeData Hunger;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, Hunger)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHunger, Category = "Survival")
	FGameplayAttributeData MaxHunger;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, MaxHunger)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Thirst, Category = "Survival")
	FGameplayAttributeData Thirst;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, Thirst)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxThirst, Category = "Survival")
	FGameplayAttributeData MaxThirst;
	ACCIDENTALHERO_ATTRIBUTE_ACCESSORS(UAccidentalHeroAttributeSet, MaxThirst)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

protected:
	/** The pawn behind this attribute set. Null before possession, so callers must check. */
	class AAccidentalHeroCharacter* GetOwningCharacter() const;

	/** Applies ConditionEffect while CurrentValue is empty and removes it once it isn't. The stored
	 *  handle is what prevents stacking; StateTag is applied loosely alongside for UI/queries. */
	void UpdateSurvivalCondition(float CurrentValue, const FGameplayTag& StateTag,
		TSubclassOf<UGameplayEffect> ConditionEffect, FActiveGameplayEffectHandle& ConditionHandle);

	/** Live handles for the two survival conditions, so each can be removed precisely. */
	FActiveGameplayEffectHandle StarvationHandle;
	FActiveGameplayEffectHandle DehydrationHandle;

	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Thirst(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxThirst(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};

#undef ACCIDENTALHERO_ATTRIBUTE_ACCESSORS
