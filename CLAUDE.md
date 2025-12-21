# Sylvanshine

C++20 tactics engine with original game design (WEGO-UGO turns, color-coded classes, FFT-style job trees, roguelite structure). Uses Duelyst's sprites, FX, and shaders as asset library.

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
