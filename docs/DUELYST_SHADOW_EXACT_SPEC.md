# Duelyst Shadow Exact Replication Spec

## Current Problem

Shadows are rendering but appear **faint** due to low final alpha values.

## Root Cause Analysis

### The Math Chain

The final shadow alpha in `shadow_perpass.frag`:

```glsl
float intensity = intensityFadeX * intensityFadeY * u_intensity;
fragColor = vec4(0.0, 0.0, 0.0, min(1.0, u_lightDistPctInv * alpha * intensity * u_opacity));
```

With current defaults:
- `u_intensity` = 0.15 (from `fx_config.shadow_intensity`)
- `u_opacity` = ~0.78 (SHADOW_OPACITY constant)
- `u_lightDistPctInv` = 1.0 (no light source set)
- `intensityFadeX/Y` = 1.0 near feet, fading toward extremities
- `alpha` = ~1.0 from 7x7 blur of solid sprite

**Result at feet**: 1.0 * 1.0 * 0.15 * 0.78 = **0.117** (only 12% visible)

### Why Duelyst Shadows Were Visible

Looking at `ShadowHighQualityFragment.glsl`:

```glsl
gl_FragColor = vec4(0.0, 0.0, 0.0,
    min(1.0, lightDistPctInv * alpha * intensity * v_fragmentColor.a));
```

The `v_fragmentColor.a` came from `u_lightColor` in the vertex shader, which was the light's RGBA. For a standard scene light with full brightness, this was 1.0.

**Key insight**: Duelyst's 0.15 intensity was balanced for their specific lighting setup. In Sylvanshine without a scene light, we need different base values.

## Duelyst Shadow Constants (Extracted)

From `ShadowHighQualityFragment.glsl` and Duelyst codebase:

| Constant | Duelyst Default | Purpose |
|----------|-----------------|---------|
| `u_intensity` | 0.15 | Shadow darkness multiplier |
| `u_blurShiftModifier` | 1.0 | Controls blur gradient steepness |
| `u_blurIntensityModifier` | 3.0 | Controls blur spread amount |
| `SHADOW_OFFSET` | 25 | Pixels from sprite bottom to feet |
| Intensity fade X exponent | 1.25 | Horizontal edge softening |
| Intensity fade Y exponent | 1.5 | Vertical edge softening |

## Implementation Fix Strategy

### Option A: Increase Base Intensity (Recommended)

Increase `shadow_intensity` to compensate for lack of lighting boost:

```cpp
// In FXConfig struct (gpu_renderer.hpp)
float shadow_intensity = 0.5f;  // Was 0.15f
```

This gives: 1.0 * 1.0 * 0.5 * 0.78 = **0.39** (39% visible - much better)

### Option B: Add Intensity Boost Factor

Add a separate boost multiplier for non-lit scenes:

```cpp
float shadow_base_boost = 3.0f;  // Applied when no scene light
float effective_intensity = shadow_intensity * shadow_base_boost;
```

### Option C: Adjust the Fade Curve

The intensity fade exponents (1.25, 1.5) may be too aggressive:

```glsl
// Less aggressive fade = more visible shadow body
float intensityFadeX = pow(1.0 - occluderDistPctX, 0.75);  // Was 1.25
float intensityFadeY = pow(1.0 - occluderDistPctY, 1.0);   // Was 1.5
```

## Recommended Implementation

**Phase 1: Quick Fix**
1. Increase `shadow_intensity` from 0.15 to 0.5
2. This immediately makes shadows visible while preserving the penumbra effect

**Phase 2: Proper Light Integration**
1. When a scene light exists, use the original 0.15 intensity
2. Without a scene light, apply boost factor
3. Or: make shadows respect scene light color/intensity properly

## Shader Verification Checklist

- [x] 7x7 box blur implemented correctly
- [x] Progressive blur (0 at feet, increasing toward head)
- [x] Intensity falloff with correct exponents (1.25, 1.5)
- [x] Light distance attenuation formula
- [ ] **Base intensity needs boost for visibility**
- [ ] UV mapping correct (sprite fills 0-1 in FBO)

## Files to Modify

1. `include/gpu_renderer.hpp`: Change `shadow_intensity` default
2. `src/gpu_renderer.cpp`: (Optional) Add conditional boost logic
3. `shaders/shadow_perpass.frag`: (Optional) Adjust fade exponents

## Quick Test Command

After fix, verify with:
```bash
./build.fish br
# Press M to toggle multipass mode
# Shadows should be clearly visible at unit feet
```
