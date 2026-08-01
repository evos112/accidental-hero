// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponAttackBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Items/WeaponDefinition.h"

void UGA_WeaponAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!ASC || !AvatarActor || !DamageEffect)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Start = AvatarActor->GetActorLocation();
	const FVector End = Start + AvatarActor->GetActorForwardVector() * AttackRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GA_WeaponAttack), false, AvatarActor);

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(AttackRadius), QueryParams);

	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == AvatarActor || HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.Add(HitActor);

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.SetAbility(this);
		EffectContext.AddInstigator(AvatarActor, AvatarActor);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -AttackDamage);
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_WeaponAttackBase::ConfigureFromWeapon(const UWeaponDefinition* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	AttackDamage = Weapon->Damage;
	AttackRange = Weapon->Range;
	AttackRadius = Weapon->Radius;
}
