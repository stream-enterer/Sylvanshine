# Sylvanshine

C++20 tactics engine with original game design (WEGO-UGO turns, color-coded classes, FFT-style job trees, roguelite structure). Uses Duelyst's sprites, FX, and shaders as asset library.

**Invoke the `duelyst-analyzer` skill before game feature work.** The skill maps Duelyst's 664 units, 240 FX, 96 shaders, and their connections. Use it to:
- Find assets that fit the user's custom systems (e.g., "which units suit my Grey/Tank class?")
- Understand how Duelyst implemented rendering, animation, or FX technically
- See relationships between assets (unit → FX → shader → sound) before implementation

The game mechanics are original. Duelyst provides the asset library. The skill helps you navigate that library and understand how assets work together—so you can guide the user toward assets and technical approaches that fit their vision, not redirect them toward Duelyst's mechanics.

## Build

```bash
./build.fish {build|debug|br|clean|shaders|assets}
```
- `build` = full pipeline (assets + shaders + release)
- `br` = build and run

**Requires:** CMake 3.20+, C++20, SDL3, SDL3_image, `glslangValidator`, Python 3.

## Architecture

- `src/`, `include/` — C++ engine
- `dist/` — Built assets (`build_assets.py` generates `assets.json` manifest)
- `data/shaders/` — GLSL shaders + compiled SPIR-V
- `duelyst_analysis/` — Forensic knowledge base (access via `duelyst-analyzer` skill)
- `app/` — Original Duelyst CoffeeScript (read when skill points you here)

## Key Patterns

**Entity State Machine:** `Spawning → Idle ⟷ Moving/Attacking → Dying → Dissolving → dead`

**Board:** 9×5 tiles. `BoardPos` ↔ `Vec2` via `board_to_screen_perspective()` / `screen_to_board_perspective()`.
