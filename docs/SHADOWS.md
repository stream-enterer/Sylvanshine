# Shadow System

## Goal

Realistic sun shadows on sprites: contact hardening (sharp at feet, soft far away), proper blur gradient, natural fade.

## Current State

- Shadows connect to feet ✓
- Shadows show sprite silhouette ✓
- Shadows respond to light angle ✓
- Shadows render BEHIND sprites ✓ (fixed: render order in multipass)
- Missing: contact hardening, realistic blur gradient

## Architecture

```
main.cpp: render_multi_pass()
    │
    ├─► Shadows → surface FBO (via draw_sdf_shadow)
    ├─► end_surface_pass() + bloom + post-processing
    ├─► composite_to_screen() - shadows to swapchain
    └─► Sprites → swapchain directly (on top of shadows)
```

**Critical**: Sprites must render AFTER composite_to_screen() to appear on top of shadows. They bypass the surface FBO to avoid tone mapping.

## You Have No GPU

Add `SDL_Log()` to print values, ask user to run and paste output, then fix based on data.

To isolate shader vs geometry bugs: set `float encoded = 0.0;` in `sdf_shadow.frag` to bypass SDF.

## Code

| What | Where |
|------|-------|
| Render order | `src/main.cpp:1105` render_multi_pass() |
| Shadow entry | `src/entity.cpp:223` render_shadow() |
| SDF shadow geometry | `src/gpu_renderer.cpp:2061` draw_sdf_shadow() |
| Fragment shader | `shaders/sdf_shadow.frag` |
| Composite shader | `shaders/surface_composite.frag` (has tone mapping) |
| Tunables | `include/gpu_renderer.hpp:220` FXConfig |

## Fragment Shader Alpha Chain (sdf_shadow.frag)

```glsl
float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity;
```

## Available Data in Fragment Shader

- `v_localPos` - pixel position in sprite space
- `u_anchor` - feet position in sprite space
- `u_spriteSize` - sprite dimensions
- `distFromFeet` - already computed, distance from anchor

## Key Constants

```cpp
SHADOW_OFFSET = 19.5f  // Sprite feet are this far above frame bottom
```

## Build

```bash
./build.fish build && ./build/tactics
```
