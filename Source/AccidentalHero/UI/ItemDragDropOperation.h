// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDropOperation.generated.h"

class UItemDefinition;

/**
 * Payload for dragging an item onto a quick-bar key.
 *
 * Carries where the drag started as well as what is being dragged, because the two cases behave
 * differently: dragging between two keys swaps their bindings, while dragging in from the inventory
 * grid simply assigns. SourceHotbarIndex is INDEX_NONE for the latter.
 */
UCLASS()
class ACCIDENTALHERO_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Drag")
	TObjectPtr<UItemDefinition> Item = nullptr;

	/** Quick-bar slot the drag started from, or INDEX_NONE when it came from the inventory grid. */
	UPROPERTY(BlueprintReadWrite, Category = "Drag")
	int32 SourceHotbarIndex = INDEX_NONE;
};
