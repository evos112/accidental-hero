// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GoalSubsystem.generated.h"

class UGoalDefinition;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalCompleted, UGoalDefinition*, Goal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllGoalsCompleted);

/** One checklist line as the UI wants it: the goal, whether it's ticked, and how far along. */
USTRUCT(BlueprintType)
struct ACCIDENTALHERO_API FGoalStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UGoalDefinition> Goal = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Goal")
	bool bComplete = false;

	/** Current count toward RequiredCount. Frozen at the requirement once complete, so a ticked
	 *  line never reads "2/3" after you spend what you made. */
	UPROPERTY(BlueprintReadOnly, Category = "Goal")
	int32 Progress = 0;
};

/**
 * Tracks the checklist that ends the game (SPEC 5.7).
 *
 * A GameInstance subsystem rather than a world one: the checklist is the player's progress through
 * the whole game, and must not reset when a level reloads or the pawn respawns.
 *
 * Completion is deliberately sticky. Goals are "obtain N of X", and almost everything you obtain is
 * meant to be spent — a stone axe becomes wear, an iron ingot becomes a pickaxe. Re-testing a
 * completed goal against the current inventory would un-tick lines as the player used what they
 * built, which reads as a bug and punishes normal play.
 */
UCLASS()
class ACCIDENTALHERO_API UGoalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Re-tests every unfinished goal against Inventory and ticks any that are now satisfied.
	 *  Cheap enough to call on every inventory change: it's a few integer compares over a handful
	 *  of goals, and it stops once they are all done. */
	UFUNCTION(BlueprintCallable, Category = "Goals")
	void EvaluateGoals(const UInventoryComponent* Inventory);

	/** Every goal with its current state, sorted by SortOrder. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Goals")
	TArray<FGoalStatus> GetGoalStatuses(const UInventoryComponent* Inventory) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Goals")
	bool IsGoalComplete(FName GoalId) const { return CompletedGoalIds.Contains(GoalId); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Goals")
	int32 GetCompletedCount() const { return CompletedGoalIds.Num(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Goals")
	int32 GetTotalCount() const { return AllGoals.Num(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Goals")
	bool IsEverythingComplete() const { return AllGoals.Num() > 0 && CompletedGoalIds.Num() >= AllGoals.Num(); }

	/** Save/load hooks. RestoreCompleted deliberately does not re-broadcast: loading a game should
	 *  not replay every completion banner the player already saw. */
	TArray<FName> GetCompletedGoalIds() const { return CompletedGoalIds.Array(); }
	void RestoreCompleted(const TArray<FName>& Ids);

	UPROPERTY(BlueprintAssignable, Category = "Goals")
	FOnGoalCompleted OnGoalCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Goals")
	FOnAllGoalsCompleted OnAllGoalsCompleted;

private:
	/** Blocking-loads every "Goal" primary asset. Called once on Initialize — there are a handful,
	 *  and they're needed before the first inventory change arrives. */
	void LoadGoals();

	UPROPERTY()
	TArray<TObjectPtr<UGoalDefinition>> AllGoals;

	/** Stored by id, not pointer, because this is what round-trips through the save. */
	TSet<FName> CompletedGoalIds;

	/** Stops the "all done" banner firing twice if the last goal is re-evaluated. */
	bool bAnnouncedCompletion = false;
};
