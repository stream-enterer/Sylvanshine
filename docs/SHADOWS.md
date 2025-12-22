# Shadow System

## Goal

Realistic sun shadows on sprites: contact hardening (sharp at feet, soft far away), proper blur gradient, natural fade.

## Current State

Shadows connect to feet, show sprite silhouette, respond to light angle. Missing realistic blur/softness.

## You Have No GPU

Add `SDL_Log()` to print values, ask user to run and paste output, then fix based on data.

To isolate shader vs geometry: set `float encoded = 0.0;` in `sdf_shadow.frag` to bypass SDF.

## Code

| What | Where |
|------|-------|
| Entry | `src/entity.cpp:223` |
| Geometry/UVs | `src/gpu_renderer.cpp:2061` |
| Fragment shader | `shaders/sdf_shadow.frag` |
| Tunables | `include/gpu_renderer.hpp:220` |

## Alpha Chain (sdf_shadow.frag:71)

```glsl
float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity;
```

## Available Data in Fragment Shader

- `v_localPos` - pixel position in sprite space
- `u_anchor` - feet position in sprite space
- `u_spriteSize` - sprite dimensions
- `distFromFeet` - already computed, distance from anchor

## Build

```bash
./build.fish build && ./build/tactics
```
