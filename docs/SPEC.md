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
8. **Compact the valley** — bring wood, stone, iron, water and flat farmland within ~1 km of spawn.
   Move the off-map towns back onto the landscape; place the six unpositioned POI markers.
9. **Equip system** — hotbar keys currently report "no equip system yet"; the paper doll is a mockup.
10. **The checklist** — the goal system, and a UI for it.
11. **Content** — item icons (**not one item has one**), a real farm-bed mesh, audio (there is none).
12. **Co-op** — listen server, 2–4 players, invite-only.
13. **Ship-shape** — settings, key rebinding, main menu, performance re-verification.

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
