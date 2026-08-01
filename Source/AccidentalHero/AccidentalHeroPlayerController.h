// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AccidentalHeroPlayerController.generated.h"

class UUserWidget;

UCLASS()
class ACCIDENTALHERO_API AAccidentalHeroPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAccidentalHeroPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Tab toggles between gameplay input (cursor hidden, look/move active) and UI input (cursor
	 *  shown, clicks reach the HUD). Tab is deliberately outside every Enhanced Input mapping
	 *  context, so binding it via the legacy input system here doesn't fight for the key. */
	void ToggleUIMode();

	/** Soft class ref so the HUD widget can be authored (as /Game/UI/WBP_HUD) after this code
	 *  exists, without needing an editor restart -- resolved lazily in BeginPlay, not at CDO
	 *  construction time like ConstructorHelpers::FClassFinder would require. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	bool bUIModeActive = false;
};
