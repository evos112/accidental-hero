# Design — Tool Durability

**Status:** Implemented and verified in PIE · **Written:** 2026-08-01
Implements SPEC §5.5 (tools wear and break, generously) and resolves SPEC §6.2.

---

## 1. The problem, restated

Tools should wear out with use and break. Durability is per-object state ("*this* axe has 12 swings
left"), but the inventory is a list of stacks (`ItemDef` + `StackCount`) with no per-object storage.

## 2. Why it turns out to be easy

`UInventoryComponent::AddItem` merges into an existing entry only when
`SpaceInStack = MaxStackSize - StackCount` is positive. For an item with `MaxStackSize = 1` that is
always `0`, so **two tools can never occupy the same entry**. Every tool already has a private entry.

`FInventoryItemEntry` derives from `FFastArraySerializerItem`, so a new field costs one delta-
replicated int on entries that actually change — not a full array resend.

**Therefore: durability lives on the entry.** No parallel arrays, no instance IDs, no per-item
`UObject`s.

## 3. Data model

### `UItemDefinition` (new field)

```cpp
/** Uses before the tool breaks. 0 = indestructible (all non-tools, and anything we don't
 *  want to wear). Only meaningful when MaxStackSize == 1. */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "0"))
int32 MaxDurability = 0;
```

Per SPEC tuning targets: stone 30, iron 100, steel 300. Everything else stays 0.

### `FInventoryItemEntry` (new field)

```cpp
/** Uses remaining. Ignored entirely when ItemDef->MaxDurability == 0. */
UPROPERTY(BlueprintReadOnly, Category = "Inventory")
int32 Durability = 0;
```

Set to `ItemDef->MaxDurability` whenever an entry is created.

### Invariant that must hold

> An entry with `ItemDef->MaxDurability > 0` always has `StackCount == 1`.

Enforced by a guard in `AddItem`/`CanAddItem` treating damageable items as unstackable regardless of
`MaxStackSize`. Without this, someone setting a tool's `MaxStackSize` to 2 in the editor would
silently merge a worn axe into a fresh one and destroy the durability of one of them. Both functions
must apply the same rule or crafting's "will the output fit?" check will disagree with what
`AddItem` actually does.

## 4. Which tool gets used

`GetBestToolTier(Player, Tag)` returns the highest tier and no identity, so there is nothing to
decrement. Add alongside it:

```cpp
/** Index into the inventory of the tool that should be used for ToolTag: highest tier first,
 *  then LOWEST durability within that tier. Returns INDEX_NONE if none. */
int32 FindBestToolEntry(const FGameplayTag& ToolTag) const;
```

**Decision — lowest durability first.** Finish the worn axe before starting the fresh one. This
keeps the number of part-used tools down, and when one breaks you carry straight on with the spare
instead of ending up with several half-dead axes.

## 5. When durability is spent

**Decision — one point per successful harvest**, not per hit point of damage dealt. A swing is a
swing. `AResourceNode::Harvest` already returns false for out-of-range or depleted, so charging only
successful harvests falls out naturally.

Call sites: `ATreeNode` (axe), `ARockNode` (pickaxe or axe — whichever was actually credited),
`AForageNode` (knife, only when the knife bonus applied).

```cpp
/** Spends one use of the tool backing ToolTag. Removes the entry if it hits zero. */
void AResourceNode::ConsumeToolDurability(AAccidentalHeroCharacter* Player, const FGameplayTag& ToolTag);
```

Server-only, consistent with the rest of harvesting.

## 6. Breaking

At zero: remove the entry, fire `OnInventoryChanged`, show an on-screen message naming the tool.

**Decision — no repair.** SPEC §5.5 chose wear-and-break, and with the generous lifespans (30/100/300
uses) a repair bench would add a system the player rarely touches. Revisit only if breakage turns out
to feel punishing in play.

**Decision — warn at 20% remaining**, once per tool, so a break is never a surprise mid-job.

## 7. Two holes this exposed

### 7.1 Dropped tools would heal themselves

`AItemPickup::SpawnItemPickup(WorldContext, ItemDef, Count, Transform)` carries no durability. Drop a
worn axe and pick it up and it is fresh. "Drop" is a planned inventory context-menu action, so this
is a live duplication exploit, not a theoretical one.

**Fix:** add `int32 Durability` to `AItemPickup`, replicated alongside `ItemDef`; set it when
spawning from inventory, and pass it back through `AddItem` on pickup. Needs an `AddItem` overload
that accepts a starting durability.

### 7.2 The hotbar binds a type, not a tool

Hotbar slots store `UItemDefinition*`. With two axes of differing wear, the bar cannot say which one
it means.

**Decision — it doesn't need to.** The bar is a shortcut to "an axe"; §4 already decides which
concrete tool gets used. The hotbar should display the durability of *the tool that would be used*,
so what you see matches what you'd swing.

## 8. UI

- **Hotbar slot:** small bar along the bottom of the key, only when the bound item is damageable.
- **Inventory slot:** same treatment on the grid cell.
- **Detail panel:** exact numbers, `"Durability 24 / 30"`.
- Colour follows the existing convention — green healthy, red under 20%.

No icons exist yet (SPEC §8.11), so text-plus-bar is the right level of fidelity for now.

## 9. Persistence

Durability is a field on a replicated entry, so it serialises with the inventory in SPEC step 2 and
needs no separate handling — **provided the save is written after this change**, or the save version
is bumped. Worth sequencing durability *before* persistence for that reason.

## 10. Touch list

| File | Change |
|---|---|
| `ItemDefinition.h` | `MaxDurability` |
| `InventoryTypes.h` | `FInventoryItemEntry::Durability` |
| `InventoryComponent.h/.cpp` | Unstackable guard in `AddItem`/`CanAddItem`; set durability on entry creation; `FindBestToolEntry`; `AddItem` overload taking durability |
| `ResourceNode.h/.cpp` | `ConsumeToolDurability` |
| `TreeNode.cpp`, `RockNode.cpp`, `ForageNode.cpp` | Call it on successful harvest |
| `ItemPickup.h/.cpp` | Carry durability through drop/pickup |
| `HotbarSlotWidget`, `InventoryWidget` | Durability bar and numbers |
| Item data assets | Set `MaxDurability` on the 8 tools |

## 11. Found during implementation — still open

**`RemoveItem` picks the wrong tool.** It iterates backwards and removes the *newest* matching
entry, so "remove one Stone Axe" destroys the freshest one, not the worn one the player meant. It
also cannot express *which* axe at all, since it takes a `UItemDefinition*`.

Harmless today — nothing removes tools by definition. It becomes a bug the moment the inventory's
**Drop** context action is wired, and again if a recipe ever consumes a tool.

**Fix when needed:** an entry-index removal (`RemoveItemAt(int32 EntryIndex)`) for UI-driven removal,
leaving the by-definition version for stackable materials. The UI already knows the entry index it
is acting on.

## 12. Deliberately excluded

Repair · sharpening or quality tiers · durability on armour (no armour exists) · tools as crafting
ingredients (no recipe consumes one) · per-material wear rates (chopping stone wearing an axe faster
than wood) — all straightforward to add later on top of this model.
