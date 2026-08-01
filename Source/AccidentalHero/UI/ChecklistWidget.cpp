// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChecklistWidget.h"

#include "AccidentalHeroCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Items/InventoryComponent.h"
#include "Progression/GoalDefinition.h"
#include "Progression/GoalSubsystem.h"

void UChecklistWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const AAccidentalHeroCharacter* Character = Cast<AAccidentalHeroCharacter>(GetOwningPlayerPawn()))
	{
		if (UGoalSubsystem* Goals = Character->GetGoals())
		{
			Goals->OnGoalCompleted.RemoveDynamic(this, &UChecklistWidget::HandleGoalCompleted);
			Goals->OnGoalCompleted.AddDynamic(this, &UChecklistWidget::HandleGoalCompleted);
		}
	}

	Refresh();
}

void UChecklistWidget::NativeDestruct()
{
	if (const AAccidentalHeroCharacter* Character = Cast<AAccidentalHeroCharacter>(GetOwningPlayerPawn()))
	{
		if (UGoalSubsystem* Goals = Character->GetGoals())
		{
			Goals->OnGoalCompleted.RemoveDynamic(this, &UChecklistWidget::HandleGoalCompleted);
		}
	}

	Super::NativeDestruct();
}

void UChecklistWidget::HandleGoalCompleted(UGoalDefinition* Goal)
{
	Refresh();
}

void UChecklistWidget::Refresh()
{
	const AAccidentalHeroCharacter* Character = Cast<AAccidentalHeroCharacter>(GetOwningPlayerPawn());
	UGoalSubsystem* Goals = Character ? Character->GetGoals() : nullptr;
	if (!Goals || !WidgetTree)
	{
		return;
	}

	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->PlayerState : nullptr;
	// Inventory lives on the PlayerState so it survives respawn, same as everywhere else.
	const UInventoryComponent* Inventory = PS ? PS->FindComponentByClass<UInventoryComponent>() : nullptr;

	if (UTextBlock* Header = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GoalHdr"))))
	{
		Header->SetText(FText::FromString(FString::Printf(TEXT("CHECKLIST   %d / %d"),
			Goals->GetCompletedCount(), Goals->GetTotalCount())));
	}

	UTextBlock* List = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GoalList")));
	if (!List)
	{
		return;
	}

	FString Text;
	for (const FGoalStatus& Status : Goals->GetGoalStatuses(Inventory))
	{
		if (!Status.Goal)
		{
			continue;
		}

		Text += Status.bComplete ? TEXT("[x]  ") : TEXT("[  ]  ");
		Text += Status.Goal->DisplayName.ToString();

		// A count only earns its space when the goal needs more than one of something.
		if (!Status.bComplete && Status.Goal->RequiredCount > 1)
		{
			Text += FString::Printf(TEXT("   %d/%d"), Status.Progress, Status.Goal->RequiredCount);
		}
		Text += LINE_TERMINATOR;

		// The hint is the only place the game says where to go, so show it until the line is ticked.
		if (!Status.bComplete && !Status.Goal->Hint.IsEmpty())
		{
			Text += FString::Printf(TEXT("       %s%s"), *Status.Goal->Hint.ToString(), LINE_TERMINATOR);
		}
		Text += LINE_TERMINATOR;
	}

	if (Goals->IsEverythingComplete())
	{
		Text += TEXT("All done. You made it.");
	}

	List->SetText(FText::FromString(Text));
}
