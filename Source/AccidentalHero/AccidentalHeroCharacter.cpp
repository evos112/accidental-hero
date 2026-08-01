// Copyright Epic Games, Inc. All Rights Reserved.

#include "AccidentalHeroCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "AbilitySystemComponent.h"
#include "AccidentalHeroPlayerState.h"
#include "AbilitySystem/AccidentalHeroAttributeSet.h"
#include "AbilitySystem/GA_Sprint.h"
#include "AbilitySystem/GA_Gather.h"
#include "AbilitySystem/GE_StaminaRegen.h"
#include "AbilitySystem/GE_FallDamage.h"
#include "AbilitySystem/GE_SurvivalDrain.h"
#include "AbilitySystem/GE_BiomeDrain.h"
#include "World/BiomeDefinition.h"
#include "World/BiomeSubsystem.h"
#include "Items/InventoryComponent.h"
#include "Items/CraftingComponent.h"
#include "Items/Furnace.h"
#include "Items/FurnaceInventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "Items/ResourceNode.h"
#include "Items/FoliageHarvestLibrary.h"
#include "Items/CropPlant.h"
#include "Items/FarmPlot.h"
#include "Items/WaterSource.h"
#include "Blueprint/UserWidget.h"
#include "Save/SaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "Items/RecipeDefinition.h"
#include "AbilitySystem/AccidentalHeroGameplayTags.h"
#include "GameplayTagContainer.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"

AAccidentalHeroCharacter::AAccidentalHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SprintAbilityClass = UGA_Sprint::StaticClass();
	GatherAbilityClass = UGA_Gather::StaticClass();
	FallDamageEffect = UGE_FallDamage::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Movement is tuned for human scale rather than the third-person template's arcade defaults
	// (600 uu/s walk = 21 km/h sustained, a 2.5 m standing jump). 1 uu = 1 cm throughout.
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 400.0f, 0.0f);

	// Gravity above 1 keeps the jump arc snappy instead of floaty; with JumpZVelocity 400 that
	// gives a ~65 cm standing jump — athletic, but not superhuman.
	Movement->GravityScale = 1.25f;
	Movement->JumpZVelocity = 400.0f;

	// You cannot change direction much in mid-air once your feet leave the ground.
	Movement->AirControl = 0.12f;
	Movement->FallingLateralFriction = 0.25f;

	// A body has mass: it takes a moment to get moving and a step or two to stop, rather than
	// reaching full speed and halting within a single frame.
	Movement->MaxAcceleration = 900.0f;
	Movement->GroundFriction = 7.5f;
	Movement->bUseSeparateBrakingFriction = true;
	Movement->BrakingFriction = 1.6f;
	Movement->BrakingDecelerationWalking = 1400.0f;
	Movement->BrakingDecelerationFalling = 0.0f;

	// Waist-high obstacles need to be climbed, not stepped over; steep rock stays unwalkable so
	// the mountains read as terrain instead of ramps.
	Movement->MaxStepHeight = 40.0f;
	Movement->SetWalkableFloorAngle(47.0f);

	// Let the player shove felled trunks and other simulating debris around.
	Movement->bEnablePhysicsInteraction = true;
	Movement->PushForceFactor = 120000.0f;
	Movement->bPushForceScaledToMass = true;
	Movement->Mass = 80.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	StreamingSourceComponent = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StreamingSourceComponent"));

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	LookAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	SprintAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Sprint"));
	SprintAction->ValueType = EInputActionValueType::Boolean;

	InteractAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Interact"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	CraftAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Craft"));
	CraftAction->ValueType = EInputActionValueType::Boolean;

	GatherAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Gather"));
	GatherAction->ValueType = EInputActionValueType::Boolean;

	CrosshairAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Crosshair"));
	CrosshairAction->ValueType = EInputActionValueType::Boolean;

	CameraToggleAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_CameraToggle"));
	CameraToggleAction->ValueType = EInputActionValueType::Boolean;

	InventoryAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Inventory"));

	HotbarActions.SetNum(UInventoryComponent::HotbarSlotCount);
	for (int32 Index = 0; Index < HotbarActions.Num(); ++Index)
	{
		HotbarActions[Index] = CreateDefaultSubobject<UInputAction>(
			*FString::Printf(TEXT("IA_Hotbar%d"), Index + 1));
		HotbarActions[Index]->ValueType = EInputActionValueType::Boolean;
	}
	InventoryAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Default"));

	UInputModifierSwizzleAxis* SwizzleYX = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("Mod_SwizzleYX"));
	UInputModifierNegate* NegateAll = CreateDefaultSubobject<UInputModifierNegate>(TEXT("Mod_NegateAll"));
	UInputModifierNegate* NegateYOnly = CreateDefaultSubobject<UInputModifierNegate>(TEXT("Mod_NegateY"));
	NegateYOnly->bX = false;
	NegateYOnly->bZ = false;

	DefaultMappingContext->MapKey(MoveAction, EKeys::D);
	DefaultMappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(SwizzleYX);
	DefaultMappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(NegateAll);

	FEnhancedActionKeyMapping& SMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
	SMapping.Modifiers.Add(NegateAll);
	SMapping.Modifiers.Add(SwizzleYX);

	DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(NegateYOnly);
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	// LMB, RMB and Q are deliberately free: combat was cut (SPEC 5.2), and these are the obvious
	// keys for whatever replaces it — tool use, aiming, or an equip system.
	// E now opens the workbench/crafting UI. Interact (furnace deposit/withdraw) moved to F so the
	// two don't double-fire on the same key.
	DefaultMappingContext->MapKey(InteractAction, EKeys::F);
	DefaultMappingContext->MapKey(CraftAction, EKeys::E);
	DefaultMappingContext->MapKey(GatherAction, EKeys::G);
	DefaultMappingContext->MapKey(CrosshairAction, EKeys::F10);
	DefaultMappingContext->MapKey(CameraToggleAction, EKeys::V);
	DefaultMappingContext->MapKey(InventoryAction, EKeys::I);

	// Quick bar on the number row, the way every action-bar game does it.
	static const FKey HotbarKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
		EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
	for (int32 Index = 0; Index < HotbarActions.Num() && Index < UE_ARRAY_COUNT(HotbarKeys); ++Index)
	{
		DefaultMappingContext->MapKey(HotbarActions[Index], HotbarKeys[Index]);
	}
}

void AAccidentalHeroCharacter::Landed(const FHitResult& Hit)
{
	// Read the impact speed before Super, which hands off to SetPostLandedPhysics and zeroes it.
	const float ImpactSpeed = GetCharacterMovement() ? -GetCharacterMovement()->Velocity.Z : 0.0f;

	Super::Landed(Hit);

	// Damage is authoritative; the client's own landing would otherwise double-apply.
	if (!HasAuthority() || !FallDamageEffect || !AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	if (ImpactSpeed <= SafeFallSpeed || LethalFallSpeed <= SafeFallSpeed)
	{
		return;
	}

	// Linear from "first scrape" at SafeFallSpeed to a full health bar at LethalFallSpeed.
	const float Severity = FMath::Clamp(
		(ImpactSpeed - SafeFallSpeed) / (LethalFallSpeed - SafeFallSpeed), 0.0f, 1.0f);
	const float Damage = Severity * AttributeSet->GetMaxHealth();
	if (Damage <= 0.0f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(FallDamageEffect, 1.0f, Context);
	if (SpecHandle.IsValid())
	{
		// Health modifiers are additive, so damage goes in negative.
		SpecHandle.Data->SetSetByCallerMagnitude(AccidentalHeroGameplayTags::Data_Damage, -Damage);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	}
}

void AAccidentalHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A viewport widget outlives its owning pawn, so a respawn would otherwise stack a second bar.
	if (HotbarWidget)
	{
		HotbarWidget->RemoveFromParent();
		HotbarWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AAccidentalHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Player restore waits for the character because it needs the PlayerState's inventory, which
	// does not exist when the GameMode restores the world. Next tick, so BeginPlay's own setup
	// (ability grants, starting effects) has finished before values are overwritten.
	if (HasAuthority())
	{
		FTimerHandle RestoreHandle;
		GetWorldTimerManager().SetTimer(RestoreHandle, [WeakThis = TWeakObjectPtr<AAccidentalHeroCharacter>(this)]()
		{
			if (AAccidentalHeroCharacter* Self = WeakThis.Get())
			{
				if (const UGameInstance* GameInstance = Self->GetGameInstance())
				{
					if (USaveSubsystem* Save = GameInstance->GetSubsystem<USaveSubsystem>())
					{
						Save->RestorePlayer(Self);
					}
				}
			}
		}, 0.2f, false);
	}

	if (IsLocallyControlled())
	{
		// Quick bar is permanent HUD, created here rather than in the Blueprint's BeginPlay so it
		// can't be lost by a graph edit. ZOrder 0 keeps it under the inventory panel when that opens.
		if (HotbarWidgetClass && !HotbarWidget)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				HotbarWidget = CreateWidget<UUserWidget>(PC, HotbarWidgetClass);
				if (HotbarWidget)
				{
					HotbarWidget->AddToViewport(0);
				}
			}
		}

		if (UInventoryComponent* Inventory = GetInventoryComponent())
		{
			Inventory->OnInventoryChanged.AddDynamic(this, &AAccidentalHeroCharacter::RefreshInventoryDebugDisplay);
			RefreshInventoryDebugDisplay();
		}

		if (UCraftingComponent* Crafting = GetCraftingComponent())
		{
			Crafting->OnCraftingResult.AddDynamic(this, &AAccidentalHeroCharacter::OnCraftingResultDebug);
		}
	}
}

UAbilitySystemComponent* AAccidentalHeroCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UInventoryComponent* AAccidentalHeroCharacter::GetInventoryComponent() const
{
	const AAccidentalHeroPlayerState* AccidentalHeroPlayerState = GetPlayerState<AAccidentalHeroPlayerState>();
	return AccidentalHeroPlayerState ? AccidentalHeroPlayerState->GetInventoryComponent() : nullptr;
}

UCraftingComponent* AAccidentalHeroCharacter::GetCraftingComponent() const
{
	const AAccidentalHeroPlayerState* AccidentalHeroPlayerState = GetPlayerState<AAccidentalHeroPlayerState>();
	return AccidentalHeroPlayerState ? AccidentalHeroPlayerState->GetCraftingComponent() : nullptr;
}

void AAccidentalHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AAccidentalHeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void AAccidentalHeroCharacter::InitializeAbilitySystem()
{
	AAccidentalHeroPlayerState* AccidentalHeroPlayerState = GetPlayerState<AAccidentalHeroPlayerState>();
	if (!AccidentalHeroPlayerState)
	{
		return;
	}

	AbilitySystemComponent = AccidentalHeroPlayerState->GetAbilitySystemComponent();
	AttributeSet = AccidentalHeroPlayerState->GetAttributeSet();

	if (!AbilitySystemComponent || bAbilitySystemInitialized)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(AccidentalHeroPlayerState, this);
	bAbilitySystemInitialized = true;

	if (HasAuthority())
	{
		if (SprintAbilityClass && !SprintAbilitySpecHandle.IsValid())
		{
			SprintAbilitySpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(SprintAbilityClass, 1, INDEX_NONE, this));
		}

		if (GatherAbilityClass && !GatherAbilitySpecHandle.IsValid())
		{
			GatherAbilitySpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(GatherAbilityClass, 1, INDEX_NONE, this));
		}

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();

		FGameplayEffectSpecHandle StaminaRegenSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(UGE_StaminaRegen::StaticClass(), 1, EffectContext);
		if (StaminaRegenSpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*StaminaRegenSpecHandle.Data.Get());
		}

		// Hunger/thirst tick down from here on, mirroring the regen effect above.
		FGameplayEffectSpecHandle SurvivalDrainSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(UGE_SurvivalDrain::StaticClass(), 1, EffectContext);
		if (SurvivalDrainSpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SurvivalDrainSpecHandle.Data.Get());
		}

		// Biome penalties layer on top of that baseline. Polled rather than driven by overlap
		// events so a region can be moved or resized in the editor without rebuilding collision.
		GetWorldTimerManager().SetTimer(BiomeCheckTimerHandle, this, &AAccidentalHeroCharacter::UpdateBiome, 1.0f, true);
		UpdateBiome();
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAccidentalHeroAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &AAccidentalHeroCharacter::OnMoveSpeedAttributeChanged);

	if (AttributeSet)
	{
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}
}

void AAccidentalHeroCharacter::OnMoveSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue * BiomeSpeedMultiplier;
}

void AAccidentalHeroCharacter::UpdateBiome()
{
	UWorld* World = GetWorld();
	if (!World || !AbilitySystemComponent)
	{
		return;
	}

	UBiomeSubsystem* Subsystem = World->GetSubsystem<UBiomeSubsystem>();
	UBiomeDefinition* NewBiome = Subsystem ? Subsystem->GetBiomeAt(GetActorLocation()) : nullptr;
	if (NewBiome == CurrentBiome)
	{
		return;
	}

	CurrentBiome = NewBiome;

	// Remove the outgoing biome's drain before applying the incoming one, so crossing straight from
	// one biome into another never leaves both running.
	if (BiomeDrainHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(BiomeDrainHandle);
		BiomeDrainHandle.Invalidate();
	}

	if (CurrentBiome && CurrentBiome->HasSurvivalDrain())
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(UGE_BiomeDrain::StaticClass(), 1, EffectContext);
		if (SpecHandle.IsValid())
		{
			// Negated once, here: the data asset states drain as a positive number.
			SpecHandle.Data->SetSetByCallerMagnitude(AccidentalHeroGameplayTags::Data_Biome_Hunger, -CurrentBiome->ExtraHungerPerSecond);
			SpecHandle.Data->SetSetByCallerMagnitude(AccidentalHeroGameplayTags::Data_Biome_Thirst, -CurrentBiome->ExtraThirstPerSecond);
			BiomeDrainHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	BiomeSpeedMultiplier = CurrentBiome ? CurrentBiome->MoveSpeedMultiplier : 1.0f;
	if (AttributeSet)
	{
		// MoveSpeed itself hasn't changed, so the attribute delegate won't fire -- push the new
		// multiplier through by hand.
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed() * BiomeSpeedMultiplier;
	}
}

void AAccidentalHeroCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AAccidentalHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAccidentalHeroCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAccidentalHeroCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAccidentalHeroCharacter::StopSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AAccidentalHeroCharacter::StopSprint);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::Interact);
		EnhancedInputComponent->BindAction(CraftAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::Craft);
		EnhancedInputComponent->BindAction(GatherAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::Gather);
		EnhancedInputComponent->BindAction(CrosshairAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::ToggleCrosshairUI);
		EnhancedInputComponent->BindAction(CameraToggleAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::ToggleCameraView);
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAccidentalHeroCharacter::ToggleInventoryUI);

		// All eight keys share one handler; the slot index rides along as a bound payload.
		for (int32 Index = 0; Index < HotbarActions.Num(); ++Index)
		{
			if (HotbarActions[Index])
			{
				EnhancedInputComponent->BindAction(HotbarActions[Index], ETriggerEvent::Started, this,
					&AAccidentalHeroCharacter::HotbarSlotPressed, Index);
			}
		}
	}
}

void AAccidentalHeroCharacter::HotbarSlotPressed(const FInputActionValue& Value, int32 SlotIndex)
{
	UseHotbarSlot(SlotIndex);
}

bool AAccidentalHeroCharacter::UseHotbarSlot(int32 SlotIndex)
{
	UInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	UItemDefinition* Item = Inventory->GetHotbarItem(SlotIndex);
	if (!Item)
	{
		return false;
	}

	// The bar binds an item type, not a stack, so a slot can point at something you've run out of.
	if (!Inventory->HasItem(Item, 1))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
				FString::Printf(TEXT("Out of %s"), *Item->DisplayName.ToString()));
		}
		return false;
	}

	// A waterskin is used, not eaten: it spends one swig and stays in the pack, refillable at any
	// water. Charges are stored in the entry's durability, so this reuses the wear machinery.
	if (Item->IsWaterContainer())
	{
		if (!Inventory->ConsumeContainerCharge(Item))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
					FString::Printf(TEXT("Your %s is empty. Refill it at water."),
						*Item->DisplayName.ToString()));
			}
			return false;
		}

		if (AttributeSet)
		{
			// PreAttributeChange clamps to MaxThirst, so overfilling is harmless.
			AttributeSet->SetThirst(AttributeSet->GetThirst() + Item->ContainerThirstPerUse);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
				FString::Printf(TEXT("You drink from the %s."), *Item->DisplayName.ToString()));
		}
		return true;
	}

	if (Item->IsConsumable())
	{
		return ConsumeItem(Item);
	}

	// Tools and materials have no "use" yet — there is no equip system. Say so rather than
	// silently doing nothing, so the key never feels broken.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("%s selected (no equip system yet)"), *Item->DisplayName.ToString()));
	}
	return false;
}

void AAccidentalHeroCharacter::ToggleInventoryUI(const FInputActionValue& Value)
{
	OnToggleInventoryUI();
}

void AAccidentalHeroCharacter::ToggleCrosshairUI(const FInputActionValue& Value)
{
	OnToggleCrosshairUI();
}

void AAccidentalHeroCharacter::ToggleCameraView(const FInputActionValue& Value)
{
	SetFirstPerson(!bIsFirstPerson);
}

void AAccidentalHeroCharacter::HandleDeath()
{
	// Survival drain keeps ticking at zero health, so this fires repeatedly without the guard.
	if (bIsDead || !HasAuthority())
	{
		return;
	}

	bIsDead = true;

	// Stop dead rather than ragdoll: the mesh is shared with the third-person template and has no
	// physics asset tuned for it, and a ragdoll would also need un-ragdolling on respawn.
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, RespawnDelay, FColor::Red,
			TEXT("You died. Respawning..."));
	}

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AAccidentalHeroCharacter::Respawn,
		FMath::Max(0.1f, RespawnDelay), false);
}

void AAccidentalHeroCharacter::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	// Back to the level's start point. Falls back to standing up where you fell if the map has
	// none, which is better than teleporting to the world origin.
	if (AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr)
	{
		if (AActor* Start = GameMode->FindPlayerStart(GetController()))
		{
			SetActorLocation(Start->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
			SetActorRotation(Start->GetActorRotation());
			if (AController* OwningController = GetController())
			{
				OwningController->SetControlRotation(Start->GetActorRotation());
			}
		}
	}

	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
		// Non-zero, or the starvation effect that killed you is still running when you land.
		AttributeSet->SetHunger(FMath::Min(RespawnHunger, AttributeSet->GetMaxHunger()));
		AttributeSet->SetThirst(FMath::Min(RespawnThirst, AttributeSet->GetMaxThirst()));
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}

	bIsDead = false;

	// Belongings are kept (SPEC 5.6), so a save here just records the new position.
	SaveNow();
}

bool AAccidentalHeroCharacter::SaveNow()
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSubsystem* Save = GameInstance->GetSubsystem<USaveSubsystem>())
		{
			return Save->SaveGame();
		}
	}
	return false;
}

void AAccidentalHeroCharacter::SetFirstPerson(bool bNewFirstPerson)
{
	bIsFirstPerson = bNewFirstPerson;

	if (bIsFirstPerson)
	{
		// Pull the camera onto the head and steer the body with the camera, so aiming and facing
		// agree — third-person leaves the body free to face its movement direction instead.
		CameraBoom->TargetArmLength = 0.0f;
		CameraBoom->SocketOffset = FVector(0.0f, 0.0f, FirstPersonEyeHeight);
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetMesh()->SetOwnerNoSee(true);
	}
	else
	{
		CameraBoom->TargetArmLength = ThirdPersonArmLength;
		CameraBoom->SocketOffset = FVector::ZeroVector;
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetMesh()->SetOwnerNoSee(false);
	}
}

void AAccidentalHeroCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAccidentalHeroCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAccidentalHeroCharacter::StartSprint(const FInputActionValue& Value)
{
	if (AbilitySystemComponent && SprintAbilitySpecHandle.IsValid())
	{
		AbilitySystemComponent->TryActivateAbility(SprintAbilitySpecHandle);
	}
}

void AAccidentalHeroCharacter::StopSprint(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAbilityHandle(SprintAbilitySpecHandle);
	}
}

void AAccidentalHeroCharacter::Gather(const FInputActionValue& Value)
{
	if (AbilitySystemComponent && GatherAbilitySpecHandle.IsValid())
	{
		AbilitySystemComponent->TryActivateAbility(GatherAbilitySpecHandle);
	}
}

void AAccidentalHeroCharacter::SetUIInputMode(bool bUIActive)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (bUIActive)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

bool AAccidentalHeroCharacter::ConsumeItem(UItemDefinition* Item)
{
	if (!HasAuthority() || !Item || !Item->IsConsumable() || !AttributeSet)
	{
		return false;
	}

	UInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory || Inventory->RemoveItem(Item, 1) < 1)
	{
		return false;
	}

	// Set the attributes directly rather than via a GameplayEffect: the restore amounts are
	// per-item data, and PreAttributeChange already clamps each to its Max, so a GE would only
	// add indirection here. Damage still goes through effects — this is a heal/refill, not combat.
	if (Item->HungerRestore > 0.0f)
	{
		AttributeSet->SetHunger(AttributeSet->GetHunger() + Item->HungerRestore);
	}
	if (Item->ThirstRestore > 0.0f)
	{
		AttributeSet->SetThirst(AttributeSet->GetThirst() + Item->ThirstRestore);
	}
	if (Item->HealthRestore > 0.0f)
	{
		AttributeSet->SetHealth(AttributeSet->GetHealth() + Item->HealthRestore);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("Consumed %s"), *Item->DisplayName.ToString()));
	}
	return true;
}

TArray<UItemDefinition*> AAccidentalHeroCharacter::FindItemsWithAnyTag(const FGameplayTagContainer& TagsToMatch) const
{
	TArray<UItemDefinition*> Result;

	UAssetManager& Manager = UAssetManager::Get();
	const FPrimaryAssetType ItemType(TEXT("Item"));

	TArray<FPrimaryAssetId> ItemIds;
	Manager.GetPrimaryAssetIdList(ItemType, ItemIds);
	if (ItemIds.Num() == 0)
	{
		return Result;
	}

	TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAssets(ItemIds);
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> ItemObjects;
	Manager.GetPrimaryAssetObjectList(ItemType, ItemObjects);

	for (UObject* Object : ItemObjects)
	{
		if (UItemDefinition* Item = Cast<UItemDefinition>(Object))
		{
			if (TagsToMatch.IsEmpty() || Item->ItemTags.HasAny(TagsToMatch))
			{
				Result.Add(Item);
			}
		}
	}
	return Result;
}

TArray<URecipeDefinition*> AAccidentalHeroCharacter::FindRecipesWithTag(const FGameplayTag& Tag) const
{
	TArray<URecipeDefinition*> Result;

	UAssetManager& Manager = UAssetManager::Get();
	const FPrimaryAssetType RecipeType(TEXT("Recipe"));

	TArray<FPrimaryAssetId> RecipeIds;
	Manager.GetPrimaryAssetIdList(RecipeType, RecipeIds);
	if (RecipeIds.Num() == 0)
	{
		return Result;
	}

	TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAssets(RecipeIds);
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> RecipeObjects;
	Manager.GetPrimaryAssetObjectList(RecipeType, RecipeObjects);

	for (UObject* Object : RecipeObjects)
	{
		if (URecipeDefinition* Recipe = Cast<URecipeDefinition>(Object))
		{
			if (Recipe->RecipeTags.HasTag(Tag))
			{
				Result.Add(Recipe);
			}
		}
	}
	return Result;
}

AFurnace* AAccidentalHeroCharacter::GetNearbyFurnace()
{
	// Returns the first in-range furnace found, not necessarily the closest one — fine while
	// there's ever only one or two furnaces near the player at once.
	TArray<AActor*> Furnaces;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFurnace::StaticClass(), Furnaces);
	for (AActor* Actor : Furnaces)
	{
		if (AFurnace* Furnace = Cast<AFurnace>(Actor); Furnace && Furnace->IsInRange(this))
		{
			return Furnace;
		}
	}
	return nullptr;
}

TArray<URecipeDefinition*> AAccidentalHeroCharacter::GetCraftingRecipes()
{
	// Blocking asset load (see FindRecipesWithTag) -- call once at HUD init, never per-frame.
	return FindRecipesWithTag(AccidentalHeroGameplayTags::Recipe_Category_Crafting);
}

TArray<URecipeDefinition*> AAccidentalHeroCharacter::GetSmeltingRecipes()
{
	// Same blocking load as above; the crafting UI caches both lists once when it opens.
	return FindRecipesWithTag(AccidentalHeroGameplayTags::Recipe_Category_Smelting);
}

void AAccidentalHeroCharacter::Interact(const FInputActionValue& Value)
{
	UInventoryComponent* PlayerInventory = GetInventoryComponent();
	if (!PlayerInventory)
	{
		return;
	}

	AFurnace* NearestFurnace = GetNearbyFurnace();
	if (!NearestFurnace)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No furnace in range."));
		}
		return;
	}

	FGameplayTagContainer FuelAndOreTags;
	FuelAndOreTags.AddTag(AccidentalHeroGameplayTags::Item_Category_Ore);
	FuelAndOreTags.AddTag(AccidentalHeroGameplayTags::Item_Category_Fuel);

	for (UItemDefinition* Item : FindItemsWithAnyTag(FuelAndOreTags))
	{
		const int32 Count = PlayerInventory->GetItemCount(Item);
		if (Count > 0 && NearestFurnace->DepositItem(this, Item, Count))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
					FString::Printf(TEXT("Deposited %dx %s"), Count, *Item->DisplayName.ToString()));
			}
		}
	}

	UFurnaceInventoryComponent* FurnaceInventory = NearestFurnace->GetFurnaceInventory();
	for (UItemDefinition* Item : FindItemsWithAnyTag(FGameplayTagContainer()))
	{
		if (FuelAndOreTags.HasAny(Item->ItemTags))
		{
			continue;
		}

		const int32 Count = FurnaceInventory ? FurnaceInventory->GetItemCount(Item) : 0;
		if (Count > 0 && NearestFurnace->WithdrawItem(this, Item, Count))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
					FString::Printf(TEXT("Withdrew %dx %s"), Count, *Item->DisplayName.ToString()));
			}
		}
	}
}

AFurnace* AAccidentalHeroCharacter::GetAimedStation() const
{
	if (!FollowCamera)
	{
		return nullptr;
	}

	// Sphere-trace along the camera aim so the player only needs to look at the station. The
	// radius gives a little forgiveness so you don't have to hit the mesh dead centre.
	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * (CraftStationRange + CameraBoom->TargetArmLength);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(40.0f), Params))
	{
		if (AFurnace* Furnace = Cast<AFurnace>(Hit.GetActor()))
		{
			// Range is measured from the player, not the camera, so the boom length doesn't
			// let you open a station from further away than CraftStationRange.
			if (FVector::Dist(GetActorLocation(), Furnace->GetActorLocation()) <= CraftStationRange)
			{
				return Furnace;
			}
		}
	}
	return nullptr;
}

AResourceNode* AAccidentalHeroCharacter::GetAimedHandPickable() const
{
	if (!FollowCamera || !CameraBoom)
	{
		return nullptr;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * (HandPickRange + CameraBoom->TargetArmLength);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(40.0f), Params))
	{
		AResourceNode* Node = Cast<AResourceNode>(Hit.GetActor());

		// Most of the world's bushes and grass are foliage instances, not actors, so a plain cast
		// finds nothing to pick. Convert the struck instance into its node first; from there it is
		// an ordinary AResourceNode and the range check below applies unchanged.
		if (!Node && HasAuthority())
		{
			Node = UFoliageHarvestLibrary::ConvertFoliageHitToNode(Hit, FoliageHarvestSet);
		}

		if (Node)
		{
			// Measured from the player so the third-person boom can't extend your reach.
			if (FVector::Dist(GetActorLocation(), Node->GetActorLocation()) <= HandPickRange)
			{
				return Node;
			}
		}
	}

	// Nothing solid in front. Grass, shrubs and berry bushes have no collision on purpose — you
	// walk through them rather than into them — so the sweep above can never find them and they
	// need the direct instance search instead.
	if (HasAuthority() && FoliageHarvestSet)
	{
		return UFoliageHarvestLibrary::HarvestNearestFoliage(GetWorld(), GetActorLocation(),
			FollowCamera->GetForwardVector(), HandPickRange, FoliageHarvestSet);
	}

	return nullptr;
}

AActor* AAccidentalHeroCharacter::PlaceStructure(UItemDefinition* Item, TSubclassOf<AActor> StructureClass,
	float Clearance, float MinGroundNormalZ, const FString& Noun)
{
	UInventoryComponent* Inventory = GetInventoryComponent();
	if (!HasAuthority() || !Item || !StructureClass || !Inventory || !Inventory->HasItem(Item, 1))
	{
		return nullptr;
	}

	const FVector Target = GetActorLocation() + GetActorForwardVector() * PlantRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlaceStructure), false, this);
	FHitResult Ground;
	if (!GetWorld()->LineTraceSingleByChannel(Ground, Target + FVector(0, 0, 300),
			Target - FVector(0, 0, 500), ECC_Visibility, Params))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No ground to build on."));
		}
		return nullptr;
	}

	// Built things need level ground — stricter than a single seed, since the whole frame sits flat.
	if (Ground.ImpactNormal.Z < MinGroundNormalZ)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
				FString::Printf(TEXT("Ground is too uneven for a %s."), *Noun));
		}
		return nullptr;
	}

	// Spacing is measured against others of the same kind, so a well doesn't block a bed.
	TArray<AActor*> Existing;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), StructureClass, Existing);
	for (const AActor* Other : Existing)
	{
		if (Other && FVector::Dist2D(Other->GetActorLocation(), Ground.ImpactPoint) < Clearance)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
					FString::Printf(TEXT("Too close to another %s."), *Noun));
			}
			return nullptr;
		}
	}

	if (Inventory->RemoveItem(Item, 1) <= 0)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Square up to the player so built things line up with how you were standing.
	const FRotator PlaceRotation(0.0f, GetActorRotation().Yaw, 0.0f);
	AActor* Placed = GetWorld()->SpawnActor<AActor>(StructureClass,
		FTransform(PlaceRotation, Ground.ImpactPoint), SpawnParams);

	if (!Placed)
	{
		// Spawn failed after the item was spent — hand it back rather than eating it.
		Inventory->AddItem(Item, 1);
		return nullptr;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("%s placed."), *Noun));
	}
	return Placed;
}

bool AAccidentalHeroCharacter::PlaceFarmPlot()
{
	return PlaceStructure(FarmPlotItem, FarmPlotClass, PlotClearance, 0.9f, TEXT("bed")) != nullptr;
}

bool AAccidentalHeroCharacter::PlaceWell()
{
	return PlaceStructure(WellItem, WellClass, PlotClearance, 0.9f, TEXT("well")) != nullptr;
}

bool AAccidentalHeroCharacter::DrinkFromWater()
{
	if (!HasAuthority())
	{
		return false;
	}

	// Nearest reachable water wins; ponds and wells are the same actor so both are found here.
	TArray<AActor*> Sources;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWaterSource::StaticClass(), Sources);

	AWaterSource* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Sources)
	{
		AWaterSource* Source = Cast<AWaterSource>(Actor);
		if (!Source || !Source->IsInRange(this))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared2D(Source->GetActorLocation(), GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Source;
		}
	}

	return Nearest && Nearest->Drink(this);
}

bool AAccidentalHeroCharacter::WaterAimedCrop()
{
	UInventoryComponent* Inventory = GetInventoryComponent();
	if (!HasAuthority() || !WateringCanItem || !Inventory || !Inventory->HasItem(WateringCanItem, 1))
	{
		return false;
	}

	// No trace here, for two reasons. GetAimedHandPickable() converts a struck foliage instance
	// into a node as a side effect, so reusing it would quietly delete scenery whenever the player
	// pointed a can at a bush. And a crop's origin sits on the ground, so any sweep aimed at one
	// hits the terrain first — which even SweepMulti stops at, since terrain blocks Visibility.
	// Proximity plus aim alignment is what actually matches "the plant I'm standing over".
	if (!FollowCamera)
	{
		return false;
	}

	const FVector Origin = GetActorLocation();
	const FVector Aim = FollowCamera->GetForwardVector().GetSafeNormal2D();

	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACropPlant::StaticClass(), Candidates);

	ACropPlant* Crop = nullptr;
	float BestScore = -1.0f;
	for (AActor* Actor : Candidates)
	{
		ACropPlant* Candidate = Cast<ACropPlant>(Actor);
		if (!Candidate || Candidate->IsWithered() || !Candidate->NeedsWater())
		{
			continue;
		}

		const FVector ToCrop = Candidate->GetActorLocation() - Origin;
		const float Distance = ToCrop.Size();
		if (Distance > HandPickRange)
		{
			continue;
		}

		// Flat alignment: looking down at your feet shouldn't stop you watering what's there.
		const float Alignment = Distance > KINDA_SMALL_NUMBER
			? FVector::DotProduct(ToCrop.GetSafeNormal2D(), Aim)
			: 1.0f;
		if (Alignment < 0.0f)
		{
			continue;
		}

		const float Score = Alignment - (Distance / FMath::Max(HandPickRange, 1.0f)) * 0.25f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Crop = Candidate;
		}
	}

	if (!Crop)
	{
		return false;
	}

	if (!Crop->AddWater(Crop->GetWaterPerPour()))
	{
		return false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FText::Format(NSLOCTEXT("AccidentalHero", "WateredCrop", "Watered {0}."),
				Crop->GetCropName()).ToString());
	}
	return true;
}

bool AAccidentalHeroCharacter::PlantSeed()
{
	if (!HasAuthority() || PlantableSeeds.IsEmpty())
	{
		return false;
	}

	UInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	// First seed the player is actually carrying wins; with one crop this is simply "the seed".
	UItemDefinition* SeedItem = nullptr;
	TSubclassOf<ACropPlant> CropClass = nullptr;
	for (const TPair<TObjectPtr<UItemDefinition>, TSubclassOf<ACropPlant>>& Pair : PlantableSeeds)
	{
		if (Pair.Key && Pair.Value && Inventory->HasItem(Pair.Key, 1))
		{
			SeedItem = Pair.Key;
			CropClass = Pair.Value;
			break;
		}
	}

	if (!SeedItem)
	{
		return false;
	}

	// A bed nearby always wins over bare ground: if the player went to the trouble of building
	// one, seeds should land in it in tidy rows rather than wherever they happen to be facing.
	AFarmPlot* TargetPlot = nullptr;
	int32 TargetSlot = INDEX_NONE;
	{
		TArray<AActor*> Plots;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFarmPlot::StaticClass(), Plots);
		float NearestDistSq = FMath::Square(PlotSnapRange);
		for (AActor* Actor : Plots)
		{
			AFarmPlot* Plot = Cast<AFarmPlot>(Actor);
			if (!Plot || Plot->FindFreeSlot() == INDEX_NONE)
			{
				continue;
			}
			// Horizontal distance only — standing on a rise above your own bed shouldn't stop a
			// seed snapping into it.
			const float DistSq = FVector::DistSquared2D(Plot->GetActorLocation(), GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				TargetPlot = Plot;
			}
		}
		if (TargetPlot)
		{
			TargetSlot = TargetPlot->FindFreeSlot();
		}
	}

	if (TargetPlot && TargetSlot != INDEX_NONE)
	{
		if (Inventory->RemoveItem(SeedItem, 1) <= 0)
		{
			return false;
		}

		FActorSpawnParameters PlotSpawnParams;
		PlotSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACropPlant* Sown = GetWorld()->SpawnActor<ACropPlant>(CropClass,
			TargetPlot->GetSlotTransform(TargetSlot), PlotSpawnParams);

		if (!Sown)
		{
			Inventory->AddItem(SeedItem, 1);
			return false;
		}

		TargetPlot->OccupySlot(TargetSlot, Sown);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Planted in bed."));
		}
		return true;
	}

	// Plant where the player is standing-and-looking, then drop onto whatever ground is there —
	// planting should follow the terrain rather than leaving crops hovering on a slope.
	const FVector Target = GetActorLocation() + GetActorForwardVector() * PlantRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlantSeed), false, this);

	// Beds are excluded from the soil trace: with a full plot in front of you the fallback would
	// otherwise plant a crop standing on the frame instead of in the earth beside it.
	TArray<AActor*> PlotActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFarmPlot::StaticClass(), PlotActors);
	Params.AddIgnoredActors(PlotActors);

	FHitResult Ground;
	if (!GetWorld()->LineTraceSingleByChannel(Ground, Target + FVector(0, 0, 300),
			Target - FVector(0, 0, 500), ECC_Visibility, Params))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No ground to plant on."));
		}
		return false;
	}

	// A crop needs soil under it, not a rock face.
	if (Ground.ImpactNormal.Z < 0.75f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Ground is too steep to plant."));
		}
		return false;
	}

	TArray<AActor*> Existing;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACropPlant::StaticClass(), Existing);
	for (const AActor* Crop : Existing)
	{
		if (Crop && FVector::Dist(Crop->GetActorLocation(), Ground.ImpactPoint) < PlantClearance)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Too close to another plant."));
			}
			return false;
		}
	}

	if (Inventory->RemoveItem(SeedItem, 1) <= 0)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACropPlant* Planted = GetWorld()->SpawnActor<ACropPlant>(CropClass,
		FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Ground.ImpactPoint), SpawnParams);

	if (!Planted)
	{
		// Spawn failed after the seed was taken — hand it back rather than eating it.
		Inventory->AddItem(SeedItem, 1);
		return false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Planted."));
	}
	return true;
}

void AAccidentalHeroCharacter::Craft(const FInputActionValue& Value)
{
	// Standing at water is unambiguous, so drinking comes first — and the same press refills a
	// waterskin, so there's no separate "fill" verb to discover.
	if (DrinkFromWater())
	{
		return;
	}

	// Tending beats taking: if the crop in front of you is thirsty and you're carrying a can, E
	// waters it rather than stripping it. Otherwise a player with a can could never water anything
	// they could also harvest.
	if (WaterAimedCrop())
	{
		return;
	}

	// E is the bare-hands interact. Picking comes first: a berry bush you're looking at should
	// yield fruit immediately, with no tool and no menu — the very first survival loop. Only if
	// there's nothing to pick does E fall through to opening a station.
	if (AResourceNode* Pickable = GetAimedHandPickable())
	{
		if (Pickable->Harvest(this))
		{
			return;
		}
	}

	// E opens a station by looking at it. Aim first (so two adjacent stations are unambiguous),
	// then fall back to plain proximity if the player is standing right on top of one.
	AFurnace* Station = GetAimedStation();

	if (!Station)
	{
		const FVector MyLocation = GetActorLocation();
		float NearestDistSq = FMath::Square(CraftStationRange);
		TArray<AActor*> Stations;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFurnace::StaticClass(), Stations);
		for (AActor* Candidate : Stations)
		{
			if (!Candidate)
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(MyLocation, Candidate->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				Station = Cast<AFurnace>(Candidate);
			}
		}
	}

	if (!Station)
	{
		// Nothing to pick and no station. Build what's being carried, otherwise sow a seed.
		// Placing beats planting so a player holding both doesn't scatter seeds when they meant to
		// lay out a farm. All sit last in the chain, so none can steal the keypress from
		// harvesting or a workbench.
		if (PlaceWell() || PlaceFarmPlot() || PlantSeed())
		{
			return;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
				TEXT("Look at a workbench or furnace to open it."));
		}
		return;
	}

	OnToggleFurnaceUI(Station);
	OnToggleCraftingUI();
}

void AAccidentalHeroCharacter::OnCraftingResultDebug(URecipeDefinition* Recipe, bool bSuccess)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, bSuccess ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("Craft %s: %s"), Recipe ? *Recipe->DisplayName.ToString() : TEXT("?"),
				bSuccess ? TEXT("success") : TEXT("failed")));
	}
}

void AAccidentalHeroCharacter::RefreshInventoryDebugDisplay()
{
	UInventoryComponent* PlayerInventory = GetInventoryComponent();
	if (!PlayerInventory || !GEngine)
	{
		return;
	}

	TArray<UItemDefinition*> AllItems = FindItemsWithAnyTag(FGameplayTagContainer());
	for (int32 Index = 0; Index < AllItems.Num(); ++Index)
	{
		const int32 Count = PlayerInventory->GetItemCount(AllItems[Index]);
		if (Count > 0)
		{
			GEngine->AddOnScreenDebugMessage(1000 + Index, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("%s x%d"), *AllItems[Index]->DisplayName.ToString(), Count));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(1000 + Index, 0.001f, FColor::Cyan, TEXT(""));
		}
	}
}
