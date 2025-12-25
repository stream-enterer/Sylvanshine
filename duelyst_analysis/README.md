# Duelyst Forensic Analysis - Knowledge Base

## Purpose

Complete LLM-usable documentation of the Duelyst codebase extracted from `app/`. This knowledge base provides structured, queryable data for understanding game mechanics, implementing features, and exploring the codebase.

## Directory Structure

| Folder | Contents | Use Case |
|--------|----------|----------|
| **instances/** | TSV data: cards, units, spells, modifiers, FX, etc. | Primary data source - grep these |
| **schemas/** | Entity definitions with fields, lifecycle, dependencies | Understanding data shapes |
| **summaries/** | System-level documentation | High-level architecture |
| **flows/** | Step-by-step gameplay sequences with timing | Understanding mechanics |
| **sylvanshine/** | Project-specific implementation docs | Sylvanshine-specific work |

## Quick Start: Common Queries

### Find a Card's Data
```bash
grep "SunstoneTemplar" instances/cards.tsv
grep "SunstoneTemplar" instances/units.tsv  # for stats, abilities
```

### Find a Modifier's Behavior
```bash
grep "ModifierProvoke" instances/modifiers.tsv
```
Check `is_keyword`, `is_watch`, `triggers` columns for classification.

### Find FX for a Card
```bash
grep "ForceField" instances/fx.tsv
```

### Understand an Action Type
```bash
grep "AttackAction" instances/actions.tsv
```
Then read `flows/unit_attack_flow.md` for the full sequence.

### Find Timing Constants
```bash
grep "ATTACK_DURATION" instances/constants.tsv
```

## Key TSV Columns

### units.tsv
| Column | Format | Example |
|--------|--------|---------|
| abilities | pipe-separated | `ModifierProvoke\|ModifierZeal` |
| sprite_resource | 10 slots | `Breathing\|Idle\|Run\|Attack\|Hit\|Death\|CastStart\|CastEnd\|CastLoop\|Cast` |
| sound_resource | 6 slots | `Deploy\|Walk\|AttackSwing\|Hit\|AttackImpact\|Death` |

### modifiers.tsv
| Column | Values | Meaning |
|--------|--------|---------|
| is_keyword | TRUE/FALSE | Card text keyword (Provoke, Flying) |
| is_watch | TRUE/FALSE | Event trigger (DeathWatch, SpellWatch) |
| stack_type | maxStacks=N | Stacking behavior |
| triggers | onBeforeAction | When modifier activates |

### actions.tsv
| Column | Meaning |
|--------|---------|
| parent_class | Inheritance hierarchy |
| execution_order | 5 steps: create → validate → sub-actions → execute → cleanup |

## Critical Formulas

### Damage Calculation
```
totalDamage = floor(max((baseDamage + damageChange) * damageMultiplier + finalDamageChange, 0))
```

### Movement Duration
```
duration = tileCount * baseDuration * CONFIG.ENTITY_MOVE_DURATION_MODIFIER
baseDuration typically 0.15s per tile
```

### Mana per Turn
```
mana = min(turnCount + 1, 9)
```

## Board Configuration

| Constant | Value |
|----------|-------|
| BOARDROW | 5 |
| BOARDCOL | 9 |
| TILESIZE | 95 |
| MAX_MANA | 9 |
| STARTING_MANA | 2 |
| MAX_HAND_SIZE | 6 |
| MAX_DECK_SIZE | 40 |
| MAX_ARTIFACTS | 3 |

## Timing Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| ACTION_DELAY | 0.5s | Default action animation delay |
| ANIMATE_FAST_DURATION | 0.2s | Quick animations |
| ANIMATE_MEDIUM_DURATION | 0.35s | Standard animations |
| ANIMATE_SLOW_DURATION | 1.0s | Slow animations |
| TURN_DURATION | 90.0s | Maximum turn time |
| HIGH_DAMAGE | 7 | Threshold for screen shake |

## Navigation Guide

### Implementing a New Attack Mechanic
1. Read `flows/unit_attack_flow.md` for full sequence
2. Check `schemas/Action.md` for action structure
3. Grep `instances/actions.tsv` for existing attack actions

### Adding a New Unit
1. Check `schemas/Unit.md` for required fields
2. Grep `instances/units.tsv` for examples with similar abilities
3. Grep `instances/modifiers.tsv` for ability modifiers

### Understanding FX System
1. Check `schemas/FX.md` for FX structure
2. Grep `instances/fx.tsv` for FX definitions
3. Grep `instances/animations.tsv` for frame timing

### Implementing Grid/Tile Rendering
1. **READ FIRST:** `sylvanshine/grid_rendering.md` - Complete forensic analysis
2. Check `sylvanshine/tile_interaction_flow.md` for hover/path state machine
3. Key systems: TileLayer.js, Player.js, TileMapScaledSprite.js

### Porting Lighting/Shadows
1. **READ FIRST:** `summaries/lighting_shadows.md` - Coordinate system differences
2. Key file: `app/view/nodes/fx/Light.js` lines 319-322 (Y/Z swap)
3. Key file: `app/shaders/ShadowVertex.glsl` lines 27-43

### Debugging Modifier Behavior
1. Read `schemas/Modifier.md` for modifier lifecycle
2. Check `flows/modifier_trigger_flow.md` for event order
3. Grep `instances/modifiers.tsv` with `is_watch=TRUE` for trigger modifiers

## Architecture Patterns

1. **Command Pattern** - All game state changes go through Actions
   - 64 action types in `instances/actions.tsv`
   - Actions can spawn sub-actions during execution
   - Modifiers intercept via `onBeforeAction`/`onAfterAction`

2. **Observer Pattern** - Event-based modifier triggers
   - Watch modifiers subscribe to specific events
   - DeathWatch, SummonWatch, SpellWatch, etc.

3. **Factory Pattern** - Entity creation
   - ModifierFactory, ActionFactory, CardFactory

4. **Singleton** - GameSession holds all game state

5. **Decorator Pattern** - Modifier stacking
   - Stat getters sum all modifier contributions

## Key Documents

### System Architecture
- `summaries/actions.md` - Action classes with command pattern
- `summaries/modifiers.md` - Modifier system with classification
- `summaries/README.md` - System relationships diagram

### Gameplay Flows
- `flows/unit_attack_flow.md` - Complete attack sequence with timing
- `flows/spell_cast_flow.md` - Spell resolution pipeline
- `flows/unit_spawn_flow.md` - Summoning and Opening Gambit
- `flows/turn_sequence_flow.md` - Turn structure and phases
- `flows/modifier_trigger_flow.md` - Event hook execution order

### Visual Systems
- `sylvanshine/grid_rendering.md` - Grid/tile system (32 sections)
- `summaries/shaders.md` - GLSL shaders
- `summaries/lighting_shadows.md` - Lighting coordinate system

## Data Relationships

```
cards.tsv (id, name, type, faction, mana)
    │
    ├─→ units.tsv (atk, hp, abilities, sprites, sounds)
    │       │
    │       └─→ modifiers.tsv (abilities lookup by name)
    │
    ├─→ spells.tsv (spell-specific fields)
    │
    └─→ artifacts.tsv (artifact-specific fields)

actions.tsv (action hierarchy)
    │
    └─→ flows/*.md (step-by-step execution)

fx.tsv (visual effects)
    │
    ├─→ animations.tsv (frame timing)
    ├─→ particles.tsv (emitter configs)
    └─→ shaders.tsv (GLSL uniforms)
```

## Remaining Gaps

### Server-Side Data (Inaccessible)
- Matchmaking logic
- Player progression
- Leaderboard rankings

### Design Intent (Not in Code)
- Card balance rationale
- Meta design decisions
- Playtesting notes
