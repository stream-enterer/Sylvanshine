# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Sylvanshine is a C++20 tactics game engine that renders Duelyst game assets. It uses SDL3 with GPU rendering and includes a comprehensive knowledge base (`duelyst_analysis/`) documenting the original Duelyst codebase.

## Build Commands

```bash
# Build (builds assets + shaders + release build)
./build.fish build

# Debug build
./build.fish debug

# Build and run
./build.fish br

# Clean build directory
./build.fish clean

# Compile shaders only
./build.fish shaders

# Build assets only (force rebuild)
./build.fish assets
```

**Requirements:**
- CMake 3.20+, C++20 compiler, SDL3, SDL3_image
- `glslangValidator` for shader compilation (GLSL → SPIR-V)
- Python 3 for asset building (`build_assets.py`)
- Duelyst assets in `app/resources/` (copied from open-duelyst/duelyst)

## Architecture

### C++ Engine (`src/`, `include/`)

| Component | Purpose |
|-----------|---------|
| `main.cpp` | Game loop, input handling, AI turn logic |
| `asset_manager` | Singleton that loads assets from `dist/assets.json` |
| `gpu_renderer` | SDL3 GPU pipeline (sprite, dissolve, color shaders) |
| `entity` | Unit state machine (Spawning→Idle→Moving/Attacking→Dying→Dissolving) |
| `grid_renderer` | Isometric board rendering with perspective |
| `animation_loader` | Parses Duelyst plist sprite atlases (build-time only) |
| `timing_loader` | Loads unit timing data (attack delays) |
| `fx` | Visual effects system |
| `plist_parser` | Apple plist XML parser for animation data |
| `perspective` | Screen↔board coordinate transforms with isometric projection |

### Asset Pipeline

The game loads assets from `dist/` which is built by `build_assets.py`:

```
app/resources/          →  build_assets.py  →  dist/
├── units/*.png,plist                          ├── assets.json (manifest)
├── fx/*.png,plist                             └── resources/
└── ...                                            ├── units/*.png
                                                   ├── fx/*.png
                                                   └── unit_shadow.png
```

`assets.json` contains pre-parsed animation data, timing, and FX mappings.

### Data Directory (`data/`)

- `shaders/` - GLSL shaders + compiled SPIR-V in `shaders/compiled/spirv/`
- `timing/` - Unit timing TSV (source for build_assets.py)
- `fx/` - FX mapping TSVs (source for build_assets.py)

### Duelyst Analysis (`duelyst_analysis/`)

Complete forensic documentation of the original Duelyst JavaScript codebase. Use the `duelyst-analyzer` skill when working with game mechanics.

**Key paths:**
- `instances/*.tsv` - All game entities (1,116 cards, 717 modifiers, 64 actions, 240 FX)
- `schemas/*.md` - Entity definitions with fields and lifecycle
- `summaries/*.md` - System-level documentation
- `flows/*.md` - Gameplay sequences (attack, spell, spawn, death)
- `scripts/` - Python query tools (run with `uv run`)

**Query examples:**
```bash
uv run duelyst_analysis/scripts/get_card_data.py Cards.Faction1.SunstoneTemplar
uv run duelyst_analysis/scripts/get_fx_composition.py ForceField
uv run duelyst_analysis/scripts/get_action_chain.py AttackAction
```

### Original Duelyst Source (`app/`)

The CoffeeScript source from open-duelyst/duelyst, used as reference. Key locations:
- `app/sdk/` - Game logic (actions, modifiers, cards, entities)
- `app/view/` - Cocos2d rendering
- `app/shaders/` - Original shader source

## Key Patterns

**Entity State Machine:**
```
Spawning → Idle ⟷ Moving/Attacking → Dying → Dissolving → dead
```

**Coordinate Systems:**
- `BoardPos` - Grid position (0-8 x, 0-4 y)
- `Vec2` - Screen position in pixels
- `board_to_screen_perspective()` / `screen_to_board_perspective()` - Transforms with isometric correction

**Timing Constants (from Duelyst CONFIG):**
- Action delay: 0.5s
- Fast/Medium/Slow fade: 0.2s / 0.35s / 1.0s
- Board: 9×5 tiles
