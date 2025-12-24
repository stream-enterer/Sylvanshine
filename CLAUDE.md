# Sylvanshine

C++20 tactics engine with original game design (WEGO-UGO turns, color-coded classes, FFT-style job trees, roguelite structure). Uses Duelyst's sprites, FX, and shaders as asset library.

## Build

```bash
./build.fish {build|debug|br|clean|shaders}
```
- `build` = full pipeline (assets + shaders + release)
- `br` = build and run
- `quick` / `qr` = C++ only, skip assets/shaders (fast iteration)

**Requires:** CMake 3.20+, C++20, SDL3, SDL3_image, `glslangValidator`, Python 3.

## Asset Pipeline (IMPORTANT)

**DO NOT run `build_assets.py` directly** - it will fail due to sandbox restrictions on `dist/`.

Use these commands instead:
- `./build.fish build` - Runs asset pipeline with proper permissions (incremental by default)
- `./build.fish br` - Build + run

**Incremental behavior:**
- `build_assets.py` only copies changed files (checks mtimes)
- `build.fish` skips Python entirely if `dist/assets.json` is newer than all source files
- No `--force` or `--clean` flags needed for normal development

**If assets need regeneration:**
1. Ask user to run: `uv run build_assets.py --force` (outside sandbox)
2. Or delete specific files in `dist/` and run `./build.fish build`

## Architecture

- `src/`, `include/` — C++ engine
- `dist/` — Built assets (`build_assets.py` generates `assets.json` manifest)
- `data/shaders/` — GLSL shaders + compiled SPIR-V
- `duelyst_analysis/` — Forensic knowledge base (access via `duelyst-analyzer` skill)
- `app/` — Original Duelyst CoffeeScript (read when skill points you here)
