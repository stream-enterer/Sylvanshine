# SDL_GPU Migration Status

## ✅ COMPLETED WORK

### 1. GPU Renderer Core Infrastructure
- ✅ Fixed critical lifecycle bugs (added `SDL_WaitForGPUIdle()` before texture/buffer release)
- ✅ Created color vertex pipeline for UI rendering
- ✅ Created line pipeline for grid rendering
- ✅ Implemented `draw_sprite()` for textured quads
- ✅ Implemented `draw_sprite_dissolve()` for death effects
- ✅ Implemented `draw_quad_colored()` for solid color quads
- ✅ Implemented `draw_line()` for grid lines
- ✅ Created GLSL color shaders: `data/shaders/glsl/color.{vert,frag}`

### 2. Entity System Migration
- ✅ Changed `TextureHandle` → `GPUTextureHandle` in `entity.hpp`
- ✅ Updated `Entity::load()` to use `g_gpu.load_texture()`
- ✅ Updated `Entity::load_shadow()` to use GPU textures
- ✅ Updated `Entity::render()` to use `g_gpu.draw_sprite()` and dissolve shader
- ✅ Updated `Entity::render_shadow()` to use GPU
- ✅ Updated `Entity::render_hp_bar()` to use `g_gpu.draw_quad_colored()`
- ✅ Removed SDL_Renderer parameters from all Entity methods

### 3. FX System Migration
- ✅ Changed `TextureHandle` → `GPUTextureHandle` in `fx.hpp`
- ✅ Updated `FXCache::get_asset()` to use `g_gpu.load_texture()`
- ✅ Updated `FXCache::get_texture()` to return `const GPUTextureHandle*`
- ✅ Updated `FXEntity::render()` to use `g_gpu.draw_sprite()`
- ✅ Updated `create_fx()` function signature
- ✅ Removed SDL_Renderer parameters from all FX methods

### 4. Grid Renderer Migration
- ✅ Updated `GridRenderer::init()` - removed SDL_Renderer parameter
- ✅ Updated `GridRenderer::render()` to use `g_gpu.draw_line()`
- ✅ Updated `GridRenderer::render_highlight()` to use `g_gpu.draw_quad_colored()`
- ✅ Updated `GridRenderer::render_move_range()` and `render_attack_range()`
- ✅ Changed `SDL_Color` → `SDL_FColor` throughout

## 🔧 REMAINING WORK

### 1. Main.cpp Call Site Updates

You need to update all call sites in `src/main.cpp`:

#### Remove SDL_Renderer Creation
```cpp
// OLD:
WindowHandle window;
RendererHandle renderer;
if (!init(config, window, renderer)) return 1;

// NEW:
WindowHandle window;
if (!init_window(config, window)) return 1;
if (!g_gpu.init(window.get())) return 1;
```

#### Update Entity Loading
```cpp
// OLD:
Entity::load_shadow(renderer.get());
unit.load(renderer.get(), "f1_general");

// NEW:
Entity::load_shadow();
unit.load("f1_general");
```

#### Update FX Creation
```cpp
// OLD:
spawn_fx_at_pos(state, renderer, "fxSmokeGround", pos);

// NEW:
spawn_fx_at_pos(state, "fxSmokeGround", pos);
```

#### Update Rendering Calls
```cpp
// OLD:
state.grid_renderer.init(renderer.get(), config);
state.grid_renderer.render(renderer.get(), config);
unit.render(renderer.get(), config);
unit.render_shadow(renderer.get(), config);
unit.render_hp_bar(renderer.get(), config);
fx.render(renderer.get(), config);

// NEW:
state.grid_renderer.init(config);
g_gpu.begin_frame();
state.grid_renderer.render(config);
unit.render_shadow(config);
unit.render(config);
unit.render_hp_bar(config);
fx.render(config);
g_gpu.end_frame();
```

#### Update Helper Functions
Update these function signatures in main.cpp:
- `spawn_fx_at_pos()` - remove SDL_Renderer parameter
- `spawn_unit_spawn_fx()` - remove SDL_Renderer parameter
- `spawn_unit_death_fx()` - remove SDL_Renderer parameter
- `spawn_attack_fx()` - remove SDL_Renderer parameter
- `create_unit()` - remove SDL_Renderer parameter
- `reset_game()` - remove SDL_Renderer parameter
- `process_pending_damage()` - remove SDL_Renderer parameter
- `handle_events()` - remove SDL_Renderer parameter
- `update()` - remove SDL_Renderer parameter
- `render()` - remove SDL_Renderer parameter, replace with GPU calls

#### Replace SDL_RenderClear and SDL_RenderPresent
```cpp
// OLD:
SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
SDL_RenderClear(renderer);
// ... draw stuff ...
SDL_RenderPresent(renderer);

// NEW:
g_gpu.begin_frame();
// ... all drawing happens here ...
g_gpu.end_frame();
```

#### Update render_floating_texts()
```cpp
// Replace SDL_SetRenderDrawColor + SDL_RenderFillRect with:
g_gpu.draw_quad_colored(rect, {r, g, b, a});
```

#### Update render_turn_indicator() and render_game_over_overlay()
```cpp
// Replace all SDL_RenderFillRect calls with:
g_gpu.draw_quad_colored(dst, color);
```

### 2. Shader Compilation

Compile the GLSL shaders to SPIR-V:

```bash
# From project root:
./compile_shaders.sh data/shaders

# This should compile:
# - sprite.vert → data/shaders/compiled/spirv/sprite.vert.spv
# - sprite.frag → data/shaders/compiled/spirv/sprite.frag.spv
# - dissolve.frag → data/shaders/compiled/spirv/dissolve.frag.spv
# - color.vert → data/shaders/compiled/spirv/color.vert.spv
# - color.frag → data/shaders/compiled/spirv/color.frag.spv
```

If `compile_shaders.sh` doesn't exist or needs updating, use:
```bash
glslc data/shaders/glsl/sprite.vert -o data/shaders/compiled/spirv/sprite.vert.spv
glslc data/shaders/glsl/sprite.frag -o data/shaders/compiled/spirv/sprite.frag.spv
glslc data/shaders/glsl/dissolve.frag -o data/shaders/compiled/spirv/dissolve.frag.spv
glslc data/shaders/glsl/color.vert -o data/shaders/compiled/spirv/color.vert.spv
glslc data/shaders/glsl/color.frag -o data/shaders/compiled/spirv/color.frag.spv
```

### 3. Build and Test

```bash
./build.fish rebuild && ./build.fish run
```

If you encounter Vulkan validation errors:
```bash
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./build/tactics
```

## 📋 VERIFICATION CHECKLIST

- [ ] Game renders identically (grid, units, FX, HP bars, UI)
- [ ] No compilation errors
- [ ] No SDL_Renderer references remaining in codebase
- [ ] Dissolve shader works on unit death
- [ ] No Vulkan validation errors
- [ ] HP bars render correctly
- [ ] Grid highlights render correctly (move/attack range)
- [ ] Shadow textures render correctly
- [ ] FX animations play correctly

## 🐛 COMMON ISSUES

### Issue: Black screen
**Cause**: g_gpu.begin_frame() / end_frame() not called properly
**Fix**: Ensure every render loop has exactly one begin_frame() at start and end_frame() at end

### Issue: Vulkan validation error about texture lifecycle
**Cause**: Texture destroyed while still referenced
**Fix**: Already fixed with SDL_WaitForGPUIdle() in destructors

### Issue: Shaders not found
**Cause**: Shaders not compiled or wrong path
**Fix**: Run shader compilation script, check `data/shaders/compiled/spirv/` exists

### Issue: Grid/UI not rendering
**Cause**: Color pipeline not created
**Fix**: Check color shaders compiled, verify color_pipeline initialization

## 📝 NOTES

- All GPU drawing must happen between `begin_frame()` and `end_frame()`
- The render pass automatically clears to background color (40, 40, 60)
- Transfer buffers are now properly synchronized with `SDL_WaitForGPUIdle()`
- Line rendering uses separate pipeline from triangles
- Dissolve shader automatically used when entity is in Dissolving state
