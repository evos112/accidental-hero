// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Gather.h"
#include "AccidentalHeroGameplayTags.h"
#include "GE_GatherCooldown.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Items/ResourceNode.h"
#include "Items/FoliageHarvestLibrary.h"
#include "Items/FoliageHarvestSet.h"
#include "AccidentalHeroCharacter.h"

UGA_Gather::UGA_Gather()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(AccidentalHeroGameplayTags::Ability_Gather));

	CooldownGameplayEffectClass = UGE_GatherCooldown::StaticClass();
}

void UGA_Gather::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AAccidentalHeroCharacter* AvatarCharacter = Cast<AAccidentalHeroCharacter>(ActorInfo->AvatarActor.Get());
	if (!AvatarCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Start = AvatarCharacter->GetActorLocation();
	const FVector End = Start + AvatarCharacter->GetActorForwardVector() * GatherRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GA_Gather), false, AvatarCharacter);

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(GatherRadius), QueryParams);

	bool bHarvested = false;
	for (const FHitResult& Hit : HitResults)
	{
		// Already an actor node — harvest it directly.
		if (AResourceNode* Node = Cast<AResourceNode>(Hit.GetActor()))
		{
			Node->Harvest(AvatarCharacter);
			bHarvested = true;
			break;
		}

		// Otherwise it may be a scattered foliage instance. Convert it to a node on first strike
		// so the world's trees/rocks are harvestable without pre-placing actors, then harvest the
		// freshly spawned node with this same swing.
		if (AResourceNode* Converted = UFoliageHarvestLibrary::ConvertFoliageHitToNode(Hit, FoliageHarvestSet))
		{
			Converted->Harvest(AvatarCharacter);
			bHarvested = true;
			break;
		}
	}

	// The sweep only ever finds foliage that has collision. Ground cover deliberately has none, so
	// a swing into grass or a berry bush needs the direct instance search to land at all.
	if (!bHarvested && AvatarCharacter->HasAuthority())
	{
		if (AResourceNode* Converted = UFoliageHarvestLibrary::HarvestNearestFoliage(AvatarCharacter,
			Start, AvatarCharacter->GetActorForwardVector(), GatherRange, FoliageHarvestSet))
		{
			Converted->Harvest(AvatarCharacter);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
