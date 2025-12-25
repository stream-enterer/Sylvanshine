# Sylvanshine

C++20 tactics engine with original game design. Uses Duelyst sprites, FX, and shaders as asset library.

## Build

```bash
./build.fish <command>
```

| Command | Description |
|---------|-------------|
| `build` | Full build (assets + shaders + release) |
| `br` | Build + run |
| `quick` / `qr` | C++ only, skip assets/shaders |
| `debug` | Debug build |
| `shaders` | Compile shaders only |
| `clean` | Remove build directory |

## Asset Pipeline

Asset building is handled by `./build.fish build` which runs `uv run build_assets.py`.

## Architecture

| Directory | Purpose |
|-----------|---------|
| `src/`, `include/` | C++ engine |
| `shaders/` | GLSL source → compiled to `dist/shaders/` |
| `dist/` | Built assets (manifest: `assets.json`) |
| `duelyst_analysis/` | Curated knowledge base |
| `app/` | Original Duelyst CoffeeScript |

**ONLY** read `app/` when `duelyst_analysis/` lacks information or explicitly references a source file.
