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

## Code Organization

### Module Categories

| Category | Files | Purpose |
|----------|-------|---------|
| **Core Types** | `types`, `perspective` | Foundational types (Vec2, BoardPos, RenderConfig), perspective math |
| **Rendering** | `gpu_renderer`, `grid_renderer`, `text_renderer`, `render_pass` | All GPU/drawing code |
| **Entities** | `entity`, `fx` | Game objects and visual effects |
| **Assets** | `asset_manager`, `animation_loader`, `plist_parser` | Resource loading and caching |
| **Game State** | `game_state.hpp` | GameState struct, enums, helper types |
| **Game Systems** | `lighting_presets.hpp`, `settings_menu.hpp` | Self-contained game systems |
| **Entry** | `main.cpp` | Game loop, input, update/render orchestration |

### Where New Code Goes

- **New type definitions**: `game_state.hpp` or create a new header if unrelated to game state
- **New rendering code**: `gpu_renderer` for low-level, create new renderer for specific systems
- **New game logic**: Keep in `main.cpp` until it's large enough to extract (~100+ lines)
- **New assets**: Add to `asset_manager` and `assets.json` schema

### Naming Conventions

- **Files**: `snake_case.cpp/.hpp`
- **Classes/Structs**: `PascalCase`
- **Functions**: `snake_case`
- **Constants**: `SCREAMING_SNAKE_CASE`
- **Member variables**: `snake_case` (no prefix)

### Header-Only Modules

Use header-only (with `inline`) for:
- Small, self-contained systems (<150 lines)
- Systems with no complex state
- Example: `lighting_presets.hpp`, `settings_menu.hpp`
