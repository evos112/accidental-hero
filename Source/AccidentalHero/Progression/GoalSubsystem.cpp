// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoalSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "GoalDefinition.h"
#include "Items/InventoryComponent.h"
#include "Items/ItemDefinition.h"

void UGoalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadGoals();
}

void UGoalSubsystem::LoadGoals()
{
	AllGoals.Reset();

	UAssetManager& Manager = UAssetManager::Get();
	const FPrimaryAssetType GoalType(TEXT("Goal"));

	TArray<FPrimaryAssetId> GoalIds;
	Manager.GetPrimaryAssetIdList(GoalType, GoalIds);
	if (GoalIds.Num() == 0)
	{
		return;
	}

	// Blocking, but only a handful of tiny data assets and only once per session — the first
	// inventory change can arrive before any async load would have finished.
	TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAssets(GoalIds);
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> GoalObjects;
	Manager.GetPrimaryAssetObjectList(GoalType, GoalObjects);
	for (UObject* Object : GoalObjects)
	{
		if (UGoalDefinition* Goal = Cast<UGoalDefinition>(Object))
		{
			AllGoals.Add(Goal);
		}
	}

	AllGoals.Sort([](const UGoalDefinition& A, const UGoalDefinition& B)
	{
		return A.SortOrder < B.SortOrder;
	});
}

void UGoalSubsystem::EvaluateGoals(const UInventoryComponent* Inventory)
{
	if (!Inventory || AllGoals.Num() == 0)
	{
		return;
	}

	for (UGoalDefinition* Goal : AllGoals)
	{
		if (!Goal || !Goal->RequiredItem || CompletedGoalIds.Contains(Goal->GoalId))
		{
			continue;
		}

		if (Inventory->GetItemCount(Goal->RequiredItem) < Goal->RequiredCount)
		{
			continue;
		}

		CompletedGoalIds.Add(Goal->GoalId);
		OnGoalCompleted.Broadcast(Goal);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Green,
				FString::Printf(TEXT("Checklist: %s  (%d/%d)"),
					*Goal->DisplayName.ToString(), CompletedGoalIds.Num(), AllGoals.Num()));
		}
	}

	if (!bAnnouncedCompletion && IsEverythingComplete())
	{
		bAnnouncedCompletion = true;
		OnAllGoalsCompleted.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow,
				TEXT("Checklist complete. You made it."));
		}
	}
}

TArray<FGoalStatus> UGoalSubsystem::GetGoalStatuses(const UInventoryComponent* Inventory) const
{
	TArray<FGoalStatus> Result;
	Result.Reserve(AllGoals.Num());

	for (UGoalDefinition* Goal : AllGoals)
	{
		if (!Goal)
		{
			continue;
		}

		FGoalStatus Status;
		Status.Goal = Goal;
		Status.bComplete = CompletedGoalIds.Contains(Goal->GoalId);
		// A ticked line reports the full requirement even after the items are spent, so completed
		// goals never appear to regress.
		Status.Progress = Status.bComplete
			? Goal->RequiredCount
			: ((Inventory && Goal->RequiredItem) ? Inventory->GetItemCount(Goal->RequiredItem) : 0);
		Result.Add(Status);
	}
	return Result;
}

void UGoalSubsystem::RestoreCompleted(const TArray<FName>& Ids)
{
	CompletedGoalIds.Reset();
	CompletedGoalIds.Append(Ids);
	// Suppress the banner for a save that was already finished, rather than congratulating the
	// player again every time they load it.
	bAnnouncedCompletion = IsEverythingComplete();
}
