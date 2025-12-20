---
name: duelyst-analyzer
description: Query tools for Duelyst codebase forensic analysis. Use when working with Duelyst game implementation, looking up cards/units/modifiers/animations/FX/shaders, or exploring the duelyst_analysis/ knowledge base.
---

# Duelyst Analyzer

Complete LLM-usable documentation of the Duelyst codebase at `./duelyst_analysis/`.

## First Steps

Before any Duelyst work, read these in order:

1. `./duelyst_analysis/README.md` — navigation map, data coverage, quick start
2. `./duelyst_analysis/system_interactions.md` — action execution, attack flow, FX system, network sync
3. `./duelyst_analysis/summaries/README.md` — system relationship diagram

## Key Resources

| Path | Contents |
|------|----------|
| `./duelyst_analysis/instances/*.tsv` | All game entity data (cards, units, modifiers, fx, etc.) |
| `./duelyst_analysis/schemas/*.md` | Entity definitions with fields and lifecycle |
| `./duelyst_analysis/summaries/*.md` | System-level documentation |
| `./duelyst_analysis/flows/*.md` | End-to-end gameplay sequences |
| `./duelyst_analysis/semantic/*.tsv` | Classes, functions, constants, events |

## Query Scripts

16 Python scripts in `./duelyst_analysis/scripts/`. Run with:

```bash
uv run ./duelyst_analysis/scripts/<script>.py --help
```

All scripts support `--help` for usage details.

## Discovery

Entity IDs live in TSV files. To find valid IDs:

```bash
# List cards
head -5 ./duelyst_analysis/instances/cards.tsv

# List modifiers
cut -f1 ./duelyst_analysis/instances/modifiers.tsv | head -20

# Search for specific terms
rg "Provoke" ./duelyst_analysis/instances/modifiers.tsv
rg "Faction1" ./duelyst_analysis/instances/units.tsv
```

## Common Tasks

| Task | Start here |
|------|------------|
| Understand game architecture | `./duelyst_analysis/system_interactions.md` |
| Implement new unit | `./duelyst_analysis/schemas/Unit.md`, `./duelyst_analysis/flows/unit_spawn_flow.md` |
| Debug modifier behavior | `./duelyst_analysis/schemas/Modifier.md`, `./duelyst_analysis/flows/modifier_trigger_flow.md` |
| Add visual effects | `./duelyst_analysis/summaries/animations.md`, `./duelyst_analysis/summaries/shaders.md` |
| Trace action execution | `./duelyst_analysis/schemas/Action.md`, `./duelyst_analysis/flows/unit_attack_flow.md` |
