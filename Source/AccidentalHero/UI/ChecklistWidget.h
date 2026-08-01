// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChecklistWidget.generated.h"

class UGoalDefinition;

/**
 * The checklist panel (SPEC 5.7) — the only place the player can see what finishing the game means.
 *
 * Its own widget rather than a corner of WBP_Inventory: the inventory's canvas is already full
 * (equipment, backpack, condition, selected item, nearby storage, quick bar), and an earlier attempt
 * to squeeze the list in landed on top of the condition panel.
 *
 * Expects two text widgets in the Blueprint, GoalHdr and GoalList. Missing either is survivable —
 * the panel just shows less rather than taking the HUD down.
 */
UCLASS()
class ACCIDENTALHERO_API UChecklistWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the list from UGoalSubsystem. Cheap — a handful of goals and a string build. */
	UFUNCTION(BlueprintCallable, Category = "Goals")
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bound to UGoalSubsystem::OnGoalCompleted so a tick appears the moment it is earned. */
	UFUNCTION()
	void HandleGoalCompleted(UGoalDefinition* Goal);
};
