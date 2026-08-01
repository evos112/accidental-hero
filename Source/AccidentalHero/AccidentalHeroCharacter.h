// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "AccidentalHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class UAccidentalHeroAttributeSet;
class UGameplayAbility;
class UInventoryComponent;
class UCraftingComponent;
class UWorldPartitionStreamingSourceComponent;
class AResourceNode;
class AFurnace;
class UItemDefinition;
class URecipeDefinition;
class UFoliageHarvestSet;
class UGameplayEffect;
class ACropPlant;
class AFarmPlot;
struct FOnAttributeChangeData;
struct FGameplayTag;
struct FGameplayTagContainer;

UCLASS()
class ACCIDENTALHERO_API AAccidentalHeroCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAccidentalHeroCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	/** Forwards to the InventoryComponent on this character's PlayerState (items live there so they survive respawn). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const;

	/** Forwards to the CraftingComponent on this character's PlayerState. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	UCraftingComponent* GetCraftingComponent() const;

	/** Scans for the first in-range AFurnace (not necessarily closest). Not BlueprintPure since it
	 *  runs an actor-class scan each call — callers should call once and cache the result locally. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AFurnace* GetNearbyFurnace();

	/** Called when Health reaches zero. Freezes the player, then respawns them after RespawnDelay.
	 *  Re-entrant safe — repeated damage while already dead is ignored. */
	UFUNCTION(BlueprintCallable, Category = "Survival|Death")
	void HandleDeath();

	/** Puts the player back at the spawn point with their belongings intact (SPEC 5.6). */
	UFUNCTION(BlueprintCallable, Category = "Survival|Death")
	void Respawn();

	UFUNCTION(BlueprintPure, Category = "Survival|Death")
	bool IsDead() const { return bIsDead; }

	/** Seconds face-down before respawning. Long enough to register what happened. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Death", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.0f;

	/** Hunger and thirst granted on respawn. Deliberately not zero: waking up still starving would
	 *  re-trigger the drain that killed you and loop the death instantly. Not full either — you
	 *  come back hungry, which is the cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Death", meta = (ClampMin = "0.0"))
	float RespawnHunger = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Death", meta = (ClampMin = "0.0"))
	float RespawnThirst = 30.0f;

	/** Writes the save immediately. Exposed for a pause menu and for testing; the game also saves
	 *  on a timer, after crafting, and on level teardown. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveNow();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera")
	bool IsFirstPerson() const { return bIsFirstPerson; }

	/** Sets the camera mode directly rather than flipping it, so a loaded save can restore it. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFirstPerson(bool bNewFirstPerson);

	/** Read-only for systems that only need to inspect attributes (e.g. saving). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	const UAccidentalHeroAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Mutable access, used by the save system to put attribute values back on load. */
	UAccidentalHeroAttributeSet* GetMutableAttributeSet() const { return AttributeSet; }

	/** All Recipe.Category.Crafting recipes. Blocking asset load -- call once at HUD init, never per-frame. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<URecipeDefinition*> GetCraftingRecipes();

	/** All Recipe.Category.Smelting recipes. Same blocking load caveat as GetCraftingRecipes. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<URecipeDefinition*> GetSmeltingRecipes();

	/** Switches the local player between game input and menu input. Call with true when a
	 *  full-screen UI opens and false when it closes, otherwise the mouse stays captured by the
	 *  camera and nothing in the menu is clickable.
	 *
	 *  Uses GameAndUI rather than UIOnly so movement/look keys still reach the pawn — a survival
	 *  inventory that freezes you in place while a bear approaches is worse than one you can
	 *  walk away from. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetUIInputMode(bool bUIActive);

	/** Server-authoritative. Eats/drinks one unit of Item: removes it from the inventory and
	 *  applies its Hunger/Thirst/Health restore. Returns false if not held or not consumable.
	 *  Front-door for the inventory UI's "Use" action. */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	bool ConsumeItem(UItemDefinition* Item);

protected:
	virtual void BeginPlay() override;

	/** Tears down the HUD quick bar so it doesn't outlive the pawn on death or level change. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Converts landing impact speed into health loss above SafeFallSpeed. */
	virtual void Landed(const FHitResult& Hit) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);

	/** Activates UGA_Gather to harvest whatever AResourceNode is in front of the player. */
	void Gather(const FInputActionValue& Value);

	/** Debug front-door for AFurnace: finds the nearest in-range furnace and deposits any
	 *  Ore/Fuel-tagged items the player is carrying, then withdraws any non-Ore/Fuel items
	 *  (i.e. smelting output) the furnace is holding. No UI yet — see CLAUDE.md-tracked plan. */
	void Interact(const FInputActionValue& Value);

	/** Debug front-door for UCraftingComponent: tries every Recipe.Category.Crafting recipe and
	 *  crafts the first one that's affordable. No recipe-selection UI yet. */
	void Craft(const FInputActionValue& Value);

	/** Toggles the crosshair settings UI (F10). */
	void ToggleCrosshairUI(const FInputActionValue& Value);

	/** Switches between third- and first-person view (V). */
	void ToggleCameraView(const FInputActionValue& Value);

	/** Opens/closes the inventory (I). */
	void ToggleInventoryUI(const FInputActionValue& Value);

	/** Implement in a Blueprint child to show/hide WBP_Inventory. Fired by I. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleInventoryUI();

	/** Implement in a Blueprint child to show/hide WBP_CrosshairSettings. Fired by F10. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleCrosshairUI();

	/** Implement in a Blueprint child to show/hide WBP_Crafting. Fired by the Craft key (E)
	 *  when a crafting station is in range. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleCraftingUI();

	/** Implement in a Blueprint child to show/hide the furnace UI. Fired by E when the player is
	 *  aiming at a furnace within CraftStationRange. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleFurnaceUI(AFurnace* Furnace);

	/** Traces from the camera along the aim direction and returns the first AFurnace hit within
	 *  CraftStationRange, or nullptr. Used by E so stations open by looking at them. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	AFurnace* GetAimedStation() const;

	/** Same aim trace, but for a resource node you can gather with bare hands (berries, loose
	 *  stones). Used by E so the earliest survival loop needs no tool and no menu. */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	AResourceNode* GetAimedHandPickable() const;

	void InitializeAbilitySystem();
	void OnMoveSpeedAttributeChanged(const FOnAttributeChangeData& Data);

	/** Blocking-loads every "Item" primary asset and returns those tagged with any tag in Tags.
	 *  Mirrors AFurnace::RefreshSmeltingRecipeCache's AssetManager scan pattern. */
	TArray<UItemDefinition*> FindItemsWithAnyTag(const FGameplayTagContainer& TagsToMatch) const;

	/** Blocking-loads every "Recipe" primary asset and returns those tagged with Tag. */
	TArray<URecipeDefinition*> FindRecipesWithTag(const FGameplayTag& Tag) const;

	/** Bound to InventoryComponent's OnInventoryChanged; prints nonzero item counts on screen. */
	UFUNCTION()
	void RefreshInventoryDebugDisplay();

	UFUNCTION()
	void OnCraftingResultDebug(URecipeDefinition* Recipe, bool bSuccess);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Partition", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWorldPartitionStreamingSourceComponent> StreamingSourceComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CraftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> GatherAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrosshairAction;

	/** How close the player must be to a crafting station (furnace/workbench) for E to open the
	 *  crafting UI. Centre-to-centre in cm; 150 leaves roughly 1 m of clearance to the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float CraftStationRange = 150.0f;

	/** Always-on-screen quick bar. Created in BeginPlay for the local player; unset means no HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HotbarWidgetClass;

	/** Live instance of HotbarWidgetClass, kept so it can be torn down with the pawn. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> HotbarWidget;

	/** Uses whatever the quick bar holds in SlotIndex: eats it if it's food, otherwise reports what
	 *  it is. Returns false when the slot is empty or the item can't be used yet. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	bool UseHotbarSlot(int32 SlotIndex);

	/** Item consumed to place a farm plot, and the bed it becomes. Both must be set for E to build. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Farming")
	TObjectPtr<UItemDefinition> FarmPlotItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Farming")
	TSubclassOf<AFarmPlot> FarmPlotClass;

	/** Consumes a farm plot item and places the bed on the ground ahead. */
	UFUNCTION(BlueprintCallable, Category = "Survival|Farming")
	bool PlaceFarmPlot();

	/** How far a seed will reach to find a free bed slot before falling back to bare ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Farming", meta = (ClampMin = "0.0"))
	float PlotSnapRange = 400.0f;

	/** Minimum spacing between beds, so they can't be stacked on top of each other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Farming", meta = (ClampMin = "0.0"))
	float PlotClearance = 300.0f;

	/** Carrying this lets E water a thirsty crop. Null means watering is disabled entirely. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Farming")
	TObjectPtr<UItemDefinition> WateringCanItem;

	/** Waters the crop the player is looking at, if they carry a can and it's thirsty. */
	UFUNCTION(BlueprintCallable, Category = "Survival|Farming")
	bool WaterAimedCrop();

	/** Which seed item grows into which crop. Data rather than a hardcoded pair so a second crop
	 *  is an editor entry, not a code change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Farming")
	TMap<TObjectPtr<UItemDefinition>, TSubclassOf<ACropPlant>> PlantableSeeds;

	/** How far in front of the player a seed can be planted. Deliberately shorter than
	 *  HandPickRange: plant any further out and you'd have to step forward before you could water
	 *  or harvest what you just sowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Farming", meta = (ClampMin = "0.0"))
	float PlantRange = 170.0f;

	/** Keeps crops from being planted on top of each other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Farming", meta = (ClampMin = "0.0"))
	float PlantClearance = 120.0f;

	/** Consumes one seed and plants its crop on the ground ahead. Server-authoritative; returns
	 *  false (with an on-screen reason) when there's no seed, no room, or no valid ground. */
	UFUNCTION(BlueprintCallable, Category = "Survival|Farming")
	bool PlantSeed();

	/** Impact speed (uu/s, downward) a landing can reach before it hurts. 900 is roughly a 3.3 m
	 *  drop at this gravity scale — you can hop off a boulder, not off a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Falling", meta = (ClampMin = "0.0"))
	float SafeFallSpeed = 900.0f;

	/** Impact speed that deals MaxHealth in one go — about a 16 m drop. Between this and
	 *  SafeFallSpeed damage scales linearly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Falling", meta = (ClampMin = "1.0"))
	float LethalFallSpeed = 2000.0f;

	/** Applied to self on a hard landing, with the computed damage passed as Data.Damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Survival|Falling")
	TSubclassOf<UGameplayEffect> FallDamageEffect;

	/** Arm's reach for picking berries/loose stones by hand with E. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	float HandPickRange = 200.0f;

	/** Which scattered foliage meshes E can pick, and the node each becomes. Shares the asset with
	 *  UGA_Gather so a bush behaves the same whether you pick it by hand or swing a tool at it.
	 *  Unset simply means E only finds hand-placed node actors. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival")
	TObjectPtr<UFoliageHarvestSet> FoliageHarvestSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraToggleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

	/** One action per quick-bar key, 1 through 8. An array rather than eight named properties so
	 *  binding is a loop and the slot index comes straight from the position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> HotbarActions;

	/** Enhanced Input handler; the slot index arrives as a bound payload. */
	void HotbarSlotPressed(const FInputActionValue& Value, int32 SlotIndex);

	/** True between dying and respawning. Blocks input and repeat death triggers. */
	UPROPERTY(BlueprintReadOnly, Category = "Survival|Death")
	bool bIsDead = false;

	FTimerHandle RespawnTimerHandle;

	/** True while in first-person. Toggled by V. */
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsFirstPerson = false;

	/** Spring-arm length used for third-person; captured from the constructor default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ThirdPersonArmLength = 400.0f;

	/** Camera height above the capsule centre when in first-person. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FirstPersonEyeHeight = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> SprintAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> GatherAbilityClass;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAccidentalHeroAttributeSet> AttributeSet;

	FGameplayAbilitySpecHandle SprintAbilitySpecHandle;
	FGameplayAbilitySpecHandle GatherAbilitySpecHandle;

	bool bAbilitySystemInitialized = false;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
