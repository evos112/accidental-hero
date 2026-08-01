// Copyright Epic Games, Inc. All Rights Reserved.

#include "FoliageHarvestLibrary.h"
#include "FoliageHarvestSet.h"
#include "ResourceNode.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InstancedFoliageActor.h"
#include "EngineUtils.h"

namespace
{
	/** Shared tail of both entry points: read the instance transform, remove it, spawn the node. */
	AResourceNode* ConvertInstance(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex,
		TSubclassOf<AResourceNode> NodeClass)
	{
		UWorld* World = ISM ? ISM->GetWorld() : nullptr;
		AActor* Owner = ISM ? ISM->GetOwner() : nullptr;
		if (!World || !Owner || !NodeClass)
		{
			return nullptr;
		}

		FTransform InstanceTransform;
		if (!ISM->GetInstanceTransform(InstanceIndex, InstanceTransform, /*bWorldSpace=*/true))
		{
			return nullptr;
		}

		// Order matters: read the transform first, then remove — RemoveInstance re-indexes the
		// component, so the index is meaningless afterwards.
		ISM->RemoveInstance(InstanceIndex);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = Owner;

		// Instance scale is baked per-instance; carrying it over keeps a big pine big.
		AResourceNode* Node = World->SpawnActor<AResourceNode>(NodeClass, InstanceTransform, SpawnParams);

		// Remember where it came from so the instance can be put back when the node is used up.
		// Without this the world loses a tree permanently and gains an actor permanently.
		if (Node)
		{
			Node->SetFoliageOrigin(ISM, InstanceTransform);
		}
		return Node;
	}
}

AResourceNode* UFoliageHarvestLibrary::ConvertFoliageHitToNode(const FHitResult& Hit,
	const UFoliageHarvestSet* HarvestSet)
{
	if (!HarvestSet)
	{
		return nullptr;
	}

	// Foliage hits report the ISM component and put the instance index in Hit.Item. Anything
	// else (a plain static mesh, a node actor) isn't ours to convert.
	UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!ISM || Hit.Item < 0)
	{
		return nullptr;
	}

	UWorld* World = ISM->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Spawning is authoritative; on a client the instance must stay put or the two desync.
	AActor* Owner = ISM->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return nullptr;
	}

	const TSubclassOf<AResourceNode> NodeClass = HarvestSet->FindNodeClassForMesh(ISM->GetStaticMesh());
	if (!NodeClass)
	{
		return nullptr;
	}

	return ConvertInstance(ISM, Hit.Item, NodeClass);
}

AResourceNode* UFoliageHarvestLibrary::HarvestNearestFoliage(UObject* WorldContextObject,
	const FVector& Origin, const FVector& AimDirection, float Range, const UFoliageHarvestSet* HarvestSet)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World || !HarvestSet || Range <= 0.0f)
	{
		return nullptr;
	}

	const FVector Aim = AimDirection.GetSafeNormal();

	UInstancedStaticMeshComponent* BestISM = nullptr;
	int32 BestIndex = INDEX_NONE;
	TSubclassOf<AResourceNode> BestClass = nullptr;
	float BestScore = -1.0f;

	for (TActorIterator<AInstancedFoliageActor> It(World); It; ++It)
	{
		AInstancedFoliageActor* FoliageActor = *It;

		// Spawning is authoritative; on a client the instance must stay put or the two desync.
		if (!FoliageActor || !FoliageActor->HasAuthority())
		{
			continue;
		}

		TArray<UInstancedStaticMeshComponent*> Components;
		FoliageActor->GetComponents(Components);

		for (UInstancedStaticMeshComponent* ISM : Components)
		{
			if (!ISM || ISM->GetInstanceCount() == 0)
			{
				continue;
			}

			const TSubclassOf<AResourceNode> NodeClass = HarvestSet->FindNodeClassForMesh(ISM->GetStaticMesh());
			if (!NodeClass)
			{
				continue;
			}

			// Uses the component's instance tree, so this is a bounded query rather than a walk
			// over every instance in the level.
			const TArray<int32> Nearby = ISM->GetInstancesOverlappingSphere(Origin, Range, /*bSphereInWorldSpace=*/true);
			for (int32 Index : Nearby)
			{
				FTransform InstanceTransform;
				if (!ISM->GetInstanceTransform(Index, InstanceTransform, /*bWorldSpace=*/true))
				{
					continue;
				}

				const FVector ToInstance = InstanceTransform.GetLocation() - Origin;
				const float Distance = ToInstance.Size();
				if (Distance > Range)
				{
					continue;
				}

				// Prefer what the player is looking at. Anything behind them is rejected outright
				// so E never picks a bush at your back; among those in front, the tightest angle
				// wins and distance only breaks near-ties.
				const float Alignment = Distance > KINDA_SMALL_NUMBER
					? FVector::DotProduct(ToInstance / Distance, Aim)
					: 1.0f;
				if (Alignment < 0.35f)
				{
					continue;
				}

				const float Score = Alignment - (Distance / Range) * 0.25f;
				if (Score > BestScore)
				{
					BestScore = Score;
					BestISM = ISM;
					BestIndex = Index;
					BestClass = NodeClass;
				}
			}
		}
	}

	return BestIndex != INDEX_NONE ? ConvertInstance(BestISM, BestIndex, BestClass) : nullptr;
}
