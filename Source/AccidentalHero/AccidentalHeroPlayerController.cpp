// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"

AAccidentalHeroPlayerController::AAccidentalHeroPlayerController()
{
	HUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));
}

void AAccidentalHeroPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;

	if (IsLocalController() && !HUDWidgetClass.IsNull())
	{
		if (UClass* WidgetClass = HUDWidgetClass.LoadSynchronous())
		{
			HUDWidget = CreateWidget<UUserWidget>(this, WidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport();
			}
		}
	}
}

void AAccidentalHeroPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AAccidentalHeroPlayerController::ToggleUIMode);
	}
}

void AAccidentalHeroPlayerController::ToggleUIMode()
{
	bUIModeActive = !bUIModeActive;

	if (bUIModeActive)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}
