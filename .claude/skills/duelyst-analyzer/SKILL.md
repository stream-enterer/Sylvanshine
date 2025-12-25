---
name: duelyst-analyzer
description: Query tools for Duelyst codebase forensic analysis. Use when working with Duelyst game implementation, looking up cards/units/modifiers/animations/FX/shaders, or exploring the duelyst_analysis/ knowledge base. (project)
---

# Duelyst Analyzer

Query the Duelyst knowledge base at `./duelyst_analysis/`.

## Immediate Queries

Most questions can be answered with a single grep:

```bash
# Find card data
grep "CardName" ./duelyst_analysis/instances/cards.tsv

# Find unit stats and abilities
grep "UnitName" ./duelyst_analysis/instances/units.tsv

# Find modifier behavior
grep "ModifierName" ./duelyst_analysis/instances/modifiers.tsv

# Find FX definition
grep "FXName" ./duelyst_analysis/instances/fx.tsv

# Find timing constants
grep "CONSTANT_NAME" ./duelyst_analysis/instances/constants.tsv

# Find action type
grep "ActionName" ./duelyst_analysis/instances/actions.tsv
```

## TSV Column Formats

### units.tsv
- `abilities`: pipe-separated modifier names (e.g., `ModifierProvoke|ModifierZeal`)
- `sprite_resource`: 10 slots `Breathing|Idle|Run|Attack|Hit|Death|CastStart|CastEnd|CastLoop|Cast`
- `sound_resource`: 6 slots `Deploy|Walk|AttackSwing|Hit|AttackImpact|Death`

### modifiers.tsv
- `is_keyword`: TRUE = card text keyword (Provoke, Flying, Celerity)
- `is_watch`: TRUE = event trigger (DeathWatch, SpellWatch, SummonWatch)
- `stack_type`: stacking behavior (maxStacks=1, etc.)
- `triggers`: when activated (onBeforeAction, onAfterAction, etc.)

### actions.tsv
- `parent_class`: inheritance hierarchy
- Execution order: create → validate → sub-actions → execute → cleanup

## Cross-Reference Patterns

```
cards.tsv (identifier) → units.tsv (identifier) → modifiers.tsv (abilities column)
```

To trace a card's full data:
1. `grep "CardName" instances/cards.tsv` → get type, faction, mana
2. `grep "CardName" instances/units.tsv` → get atk, hp, abilities
3. Parse `abilities` column, grep each in `instances/modifiers.tsv`

## Critical Formulas

**Damage:**
```
totalDamage = floor(max((base + damageChange) * multiplier + finalChange, 0))
```

**Movement duration:**
```
duration = tileCount * 0.15s * CONFIG.ENTITY_MOVE_DURATION_MODIFIER
```

## Deeper Documentation

Only read these when grep isn't enough:

| When you need... | Read |
|------------------|------|
| Action execution order | `flows/unit_attack_flow.md` |
| Spell resolution | `flows/spell_cast_flow.md` |
| Modifier lifecycle | `schemas/Modifier.md` |
| Event hook order | `flows/modifier_trigger_flow.md` |
| Grid/tile rendering | `sylvanshine/grid_rendering.md` |
| Lighting coordinates | `summaries/lighting_shadows.md` |
| Data relationships | `README.md` → "Data Relationships" section |

## Directory Structure

| Folder | Query method |
|--------|--------------|
| `instances/` | grep TSV files |
| `schemas/` | read for entity structure |
| `summaries/` | read for system architecture |
| `flows/` | read for step-by-step sequences |
| `sylvanshine/` | Sylvanshine-specific implementation docs |
