# AccidentalHero — Project Specification

**Status:** Decided · **Revised:** 2026-08-01
Decisions below were settled in interview. Where a number was not specified, a **DEFAULT** is
proposed and flagged.

---

## 1. What it is

A single-player survival-crafting game in which the player climbs a **tool ladder** — bare hands to
stone to iron to steel — gathering from a hand-authored landscape, farming, and keeping fed.

**It is not an MMORPG.** The `.uproject` description says so and is wrong; correcting it is a task.

**It has no combat.** This is a decision, not a gap.

## 2. Audience

Built for **you and your son to play together**. That single fact drives most of what follows:
forgiving failure, short sessions, no onboarding polish, no store presence, and co-op as a real
eventual requirement rather than a maybe.

### Not for

- Players wanting combat, story, or an MMO.
- Speedrunners and min-maxers — pacing is deliberately slow.
- Low-spec hardware (Nanite, Lumen, World Partition, ~216k foliage instances).
- Anyone needing a hundred-hour game. See §4.

## 3. Success

A player spawns with nothing and, within 30 minutes and without instruction, chops wood, makes a
stone tool, eats, plants and harvests a crop — and **quits and returns to find their farm intact**.

- 15–30 minute sessions with no dead ends
- Zero loss of player progress on quit
- 60 fps at 1080p (currently 86–144, holds)
- Completing the checklist (§5.7) feels like winning

**Not success:** feature count. There are already more systems than the game can prove.

## 4. Honest scope warning

With generous durability and three rungs, **the tool climb is a few sessions long**. Longevity must
come from farming, the checklist, and improving the home valley — not from the ladder. This is
acceptable for two players; it would not be for a commercial release.

## 5. Decisions

| # | Decision | Choice |
|---|---|---|
| 5.1 | Players | Single-player first; 2–4 co-op later. Keep existing replication |
| 5.2 | Combat | **Cut entirely** |
| 5.3 | Spine | The tool ladder |
| 5.4 | Tiers | Three: stone → iron → steel |
| 5.5 | Durability | Tools wear and break, but **generously** |
| 5.6 | Death | Respawn at spawn, keep everything |
| 5.7 | Goal | An explicit checklist to complete |
| 5.8 | Session | 15–30 minutes |
| 5.9 | Survival pacing | Roughly one meal and one drink per session |
| 5.10 | World | Compact home valley, ~1 km around spawn |
| 5.11 | Persistence | Player, crops and farm plots. **Not** foliage |
| 5.12 | Regrowth | Felled trees regrow themselves in ~15–20 minutes |
| 5.13 | Steel gate | Harder recipe (more iron + coal), no new system |
| 5.14 | Foliage save scheme | Not needed — see §6.1 |

### Tuning targets

**DEFAULT** where the interview gave a shape rather than a number:

| Value | Current | Target |
|---|---|---|
| Stone tool life | ∞ | ~30 uses |
| Iron tool life | ∞ | ~100 uses |
| Steel tool life | — | ~300 uses |
| Hunger drain | 0.12/s (~14 min) | 0.06/s (~28 min) |
| Thirst drain | 0.18/s (~9 min) | 0.09/s (~18 min) |
| Tree regrowth | none | ~1000 s |

## 6. Consequences worth knowing

### 6.1 The hardest task disappeared

Because trees regrow on their own, saving *which* trees were felled is nearly pointless — reload and
a regrown forest is indistinguishable from correct. **Foliage persistence is dropped from scope.**
This removes the foliage-identity problem, which was the single riskiest piece of work in the
project. If it is ever needed, the chosen scheme is *record removed world positions and re-remove on
load*.

### 6.2 Durability has no place to live yet

`FInventoryItemEntry` stores an item definition and a stack count — there is **no per-instance
state**. Tools stack to 1, so each occupies its own entry, but there is still nowhere to record
"this axe has 12 uses left". Adding durability means extending the inventory entry and its
replication. This is a bigger change than it sounds and should be designed before it is written.

### 6.3 Felled trees currently accumulate as actors

A struck foliage instance becomes an actor, depletes, and respawns **as an actor**. Fell 200 trees
and you have 200 actors. For regrowth to work as specified, a depleted node must return to being a
foliage instance and remove its actor. This is new work implied by §5.12.

### 6.4 Cutting combat removes real weight

`GA_MeleeAttack`, `GA_RangedAttack`, `GA_MagicBolt`, `GA_WeaponAttackBase`, `GE_MeleeDamage`,
`GE_RangedDamage`, `GE_MagicDamage`, `GE_MagicCooldown`, `GE_AttackCooldown`, `GE_ManaCost`,
`GE_ManaRegen`, `WeaponDefinition`, plus the Mana/MaxMana/ManaRegenRate attributes. It also frees
LMB, RMB and Q. `GE_FallDamage` and the Health attribute stay — falling still hurts.

## 7. Out of scope

MMO infrastructure · combat, AI, PvP · narrative, quests, NPCs · base building beyond farm plots ·
voxel terrain (`VoxelFree` is installed and unused — leave it) · console/mobile · procedural
generation · modding, localisation, anti-cheat · foliage persistence (§6.1)

## 8. Build order

Dependency-ordered. Steps 1–2 are the difference between a tech demo and a game.

1. **Correct the premise** — fix the `.uproject` description; delete the combat classes (§6.4).
2. ~~**Persistence**~~ — **DONE.** `UAccidentalHeroSaveGame` (versioned) + `USaveSubsystem`.
   Player position, camera mode, attributes, inventory (with durability) and hotbar; crops and farm
   plots including bed slot links. Saves on level teardown, every 2 minutes, after crafting, and via
   `SaveNow()`. Single slot. Round trip verified in PIE.
   **Note:** saving must happen from `GameMode::EndPlay`, not the subsystem's `Deinitialize` — by
   then the pawn is gone and there is nothing left to record.
3. **Death and respawn** — health zero currently does nothing at all.
4. **Retune survival** — halve hunger and thirst rates (§5).
5. **Tree regrowth** — depleted nodes return to foliage instances (§6.3).
6. **Durability** — extend the inventory entry (§6.2), then tool wear and breaking.
7. **Steel tier** — ore, recipes, and the tools themselves.
8. ~~**Compact the valley**~~ — **DONE.** Within 1 km of spawn: 1,917 pines (wood), 357 boulders
   (stone, and iron ore via pickaxe at 50%/strike), the home pond at 408 m, and flat farmland.
   Off-map settlements moved back on; the six POI water markers now have ponds. The 14 `BG_Mountain`
   backdrop meshes were ringed around the world origin instead of the map centre, which had left four
   2 km-wide mountains standing on playable terrain — re-centred.
   **Note:** coal had no source anywhere in the game, so both smelting recipes were uncraftable and
   the entire metal half of the tool ladder (§5.3) was unreachable. `DA_Recipe_Charcoal` (3 Timber →
   2 Coal, 12 s, smelting) closes it. Verified in PIE end to end: timber → charcoal → iron ingot.
9. ~~**Equip system**~~ — **DONE.** `EquippedItem` on `UInventoryComponent` (replicated to the owner,
   so it survives respawn with the rest of the pack). A hotbar key equips, and pressing it again puts
   the item away. **What you hold is what works:** harvest nodes read `GetEquippedToolTier` instead of
   scanning the pack, so a steel axe no longer chops from inside the backpack, and only the equipped
   tool takes the wear. Breaking or running out clears the hand automatically.
   Verified in PIE: bare hands strike power 1 vs stone axe 2; packed axe 30→30 durability, equipped
   axe 30→29; on break the hand empties.
   **Still a mockup:** seven of the paper doll's eight slots. Only Tool is wired — there is no armour
   or weapon in the game to fill Head/Chest/Legs/Feet/Hands/Back/Primary. The character also has no
   skeletal mesh assigned, so an equipped tool cannot be shown in hand.
10. ~~**The checklist**~~ — **DONE.** `UGoalDefinition` (a "Goal" primary asset type) + `UGoalSubsystem`
    on the GameInstance, so progress survives respawn and level reload. Every goal is "obtain N of
    item X", which covers crafting, smelting, building and harvesting with one rule.
    **Completion is sticky:** goals are things you make in order to spend, so re-testing against the
    current inventory would un-tick lines as the player used what they built. Ticks persist in the
    save (`CompletedGoals`, save version 2) and are restored *before* the inventory, otherwise
    restoring items re-evaluates the checklist and a spent goal reads as unfinished.
    The six rungs: stone pickaxe → 2 coal → iron ingot → farm plot → well → steel axe. None are in
    the starting kit — the first draft used a Stone Axe and a Waterskin, both of which the player
    already spawns with, so two lines ticked before the game began.
    Verified in PIE: 0/6 at spawn; ticks on acquisition; stays ticked after the item is spent; 6/6
    fires the completion banner; and after save→reload it is still 6/6 with 0 pickaxes held.
    **UI done (2026-08-02):** `UChecklistWidget` + `WBP_Checklist`, toggled with **J**. Its own panel
    rather than a corner of WBP_Inventory — that canvas is already full (equipment, backpack,
    condition, selected item, nearby storage, quick bar) and an attempt to fit the list in landed on
    top of the condition panel. Each line shows its hint until ticked, and the widget binds to
    `OnGoalCompleted` so a tick appears the moment it is earned.
    Verified in PIE: reads `CHECKLIST 0 / 6` with all six hints, and acquiring a stone pickaxe
    flipped it to `1 / 6` with that line ticked and its hint gone.
11. **Content** — **icons and the farm bed are DONE; audio is blocked.**
    All 27 items have an icon (rendered in-editor from meshes by a SceneCapture2D rig) and a
    WorldMesh, so dropped items are no longer invisible. Icons are stand-ins built from existing
    project meshes with per-item tints, not authored art.
    `SM_FarmBed` replaces the engine Cube + WorldGridMaterial: a 2 x 2 m timber-framed raised bed
    with tilled rows, generated by `Tools/blender_make_farmbed.py`.
    **Audio is blocked on source assets.** There is not one sound in `/Game`, and the only
    SoundWaves reachable are `/Engine` editor UI sounds, which are editor-only and must not ship.
    This needs a sound pack, recordings, or generated audio before any wiring is worth doing.
    **Partly done:** nine meshes modelled in Blender by the project owner are imported under
    `/Game/Meshes/Blender` — furnace, anvil, barrel, bellows, firewood pile, three fence variants and
    a street lamp. `BP_Furnace` now uses `SM_Furnace` instead of an engine Cylinder (its interaction
    sphere went 150 → 250, since the mesh is 2.3 m wide and the old radius barely cleared its own
    footprint). Lamps and a fence run are placed at spawn.
    **Note on re-importing these:** the `.blend` objects sit away from the scene origin, so a naive
    FBX export gives Unreal a pivot metres from the geometry. `scratchpad/bl_export2.py` re-pivots
    each object to base-centre before exporting; do that again rather than fixing it per-Blueprint.
12. **Co-op** — listen server, 2–4 players, invite-only.
13. **Ship-shape** — settings, key rebinding, main menu.
    ~~performance re-verification~~ — **DONE (2026-08-01).** Measured in PIE: spawn ~108 fps
    (GPU-bound), rainforest interior **12–26 fps**. `ProfileGPU` blamed `ShadowDepths` for 71.8 ms of
    a 78.7 ms frame — Nanite trees rasterising into virtual shadow maps. Fixed by thinning the
    rainforest's trees to 45% (the 35,000/km² canopy density was several times a real forest):
    rainforest now **66–74 fps**, spawn ~200 fps. Ground cover and shrubs no longer cast shadows.

## 9. Standing engineering notes

Full list in `CLAUDE.md` §11. The ones that have cost the most time:

- **Two editor instances open at once make every save silently fail** (sharing violation). Check
  `Get-Process UnrealEditor` returns exactly one before trusting a save.
- `LandscapeService.get_height_at_location` clamps out-of-bounds samples and still reports
  `valid=True` — off-map coordinates look like solid ground. The landscape spans 0…630,000.
- Widget canvas geometry is `Slot.LayoutData.Offsets`; the obvious names fail silently.
- `CaptureViewport` returns the **editor** viewport even during PIE, so in-game HUD cannot be
  screenshotted for verification.
- The MCP server listens on **port 8000**.
