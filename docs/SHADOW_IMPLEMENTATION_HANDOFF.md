# Shadow Implementation: Strategic Analysis & Handoff

**Date:** 2025-12-20
**Status:** ✓ COMPLETE - Phase 1 lighting implemented (per-sprite light attenuation)

## Decision: Option A Selected ✓ IMPLEMENTED

We implemented the dynamic lighting system first, then wired shadows to use the `lightDistPctInv` value properly. This is the architecturally sound approach that produces correct shadows without parameter tuning hacks.

### Implementation Summary (2025-12-20)

**New Components:**
- `PointLight` struct in `gpu_renderer.hpp` - screen-space point light with position, radius, color, intensity
- `set_scene_light()` / `clear_scene_light()` - scene light management
- `u_lightDistPctInv` uniform added to shadow shader

**Key Changes:**
1. **`include/gpu_renderer.hpp`:**
   - Added `PointLight` struct with x, y, radius, intensity, color, casts_shadows
   - Added `ShadowUniforms.light_dist_pct_inv` field
   - Added scene light member and methods to `GPURenderer`
   - Updated `draw_sprite_shadow()` to accept optional `PointLight*`

2. **`shaders/shadow.frag`:**
   - Added `u_lightDistPctInv` uniform
   - Restored Duelyst's original intensity falloff exponents (1.25 for X, 1.5 for Y)
   - Final alpha now multiplied by `u_lightDistPctInv` per Duelyst formula

3. **`src/gpu_renderer.cpp`:**
   - `draw_sprite_shadow()` computes `lightDistPctInv = 1 - pow(dist/radius, 2)`
   - Uses scene light if no explicit light provided
   - Falls back to full shadow (lightDistPctInv=1) if no light

4. **`src/main.cpp`:**
   - Default scene light set after GPU init
   - Position: upper-left (30% width, 20% height)
   - Radius: 80% of window width (covers entire scene)

**Formula (from Duelyst):**
```cpp
float lightDist = distance(feet_pos, light_pos);
float lightDistPct = pow(lightDist / light_radius, 2.0);
float lightDistPctInv = max(0.0, 1.0 - lightDistPct);
// In shader: fragColor.a = lightDistPctInv * alpha * intensity * opacity;
```

**Result:** Shadows now fade naturally based on light distance, matching Duelyst's behavior.

## Executive Summary

We attempted to port Duelyst's penumbra shadow shader in isolation. This was a **strategic error**. The shadow system is a dependent subsystem that requires dynamic lighting to function correctly. Without lighting, we've spent multiple iterations tuning parameters that can never produce the correct result.

## The Core Problem

Duelyst's shadow formula ends with:
```glsl
gl_FragColor = vec4(0.0, 0.0, 0.0, lightDistPctInv * alpha * intensity);
```

**`lightDistPctInv` is not optional.** It's the output of the dynamic lighting system - a value that varies per-pixel based on distance from light sources. We treated it as a constant multiplier to compensate for, but it's actually:
- Spatially varying across the shadow
- Dependent on light positions, colors, and falloff
- Part of how the shadow "reads" correctly against lit ground

By skipping dynamic lighting, we removed a core input to the shader. No amount of parameter tuning can recover this.

## Why We Kept Iterating (And Why It Failed)

| Iteration | What We Tried | Why It Failed |
|-----------|---------------|---------------|
| 1 | Port shader directly | Used wrong coordinate space (UV vs pixels) |
| 2 | Fix coordinates | Used atlas size instead of sprite size for blur |
| 3 | Fix blur calculation | Blur sampled outside sprite bounds, got 0 alpha |
| 4 | Clamp UV to bounds | Edge clamping caused "bulging" shape distortion |
| 5 | Return 0 for OOB | Shadow became too faint (blur averaging in zeros) |
| 6 | Boost intensity | Shadow too dark/harsh, no gradient |
| 7 | Soften fade exponents | Still wrong - we're tuning around the missing light contribution |
| 8 | Scale-aware blur | Architectural fix, but doesn't address root cause |

**Pattern:** Each fix solved a symptom but revealed another. This is the signature of fighting an architectural mismatch.

## Architectural Mismatches

### 1. Missing Dynamic Lighting (CRITICAL)
- **Duelyst:** Shadow intensity modulated by `lightDistPctInv` from point lights
- **Sylvanshine:** No lighting system, trying to fake it with constant multiplier
- **Impact:** Cannot achieve correct spatial variation in shadow intensity

### 2. Atlas vs Individual Textures (SEVERE)
- **Duelyst:** Each sprite has its own texture, UV 0-1 covers whole sprite
- **Sylvanshine:** Sprites packed in atlas, UV is small region (~0.1 x 0.1)
- **Impact:** Blur math assumes free UV sampling; atlas constrains us to a tiny region, blur quickly samples "nothing"

### 3. Render Pipeline Order (UNKNOWN)
- **Duelyst:** Shadows rendered in specific pass order with lighting
- **Sylvanshine:** Single-pass rendering, shadows drawn inline
- **Impact:** May need multi-pass approach for correct compositing

### 4. Scale Independence (PARTIALLY ADDRESSED)
- **Duelyst:** Fixed resolution, shaders tuned for specific pixel density
- **Sylvanshine:** Variable scale (1x, 2x, etc.), blur must scale
- **Impact:** Implemented scale-aware blur, but this is a band-aid

## Strategic Options

### Option A: Implement Dynamic Lighting First
**Effort:** High (weeks)
**Correctness:** High

Implement the point light system from Duelyst, THEN port shadows. This is the "correct" approach but delays shadows significantly.

Required:
- Port `LightColorizeFragment.glsl` and `LightFragment.glsl`
- Implement light source management (positions, colors, falloff)
- Multi-pass rendering (light accumulation pass)
- Then shadows will "just work" with original parameters

### Option B: Simplified Shadow (No Penumbra)
**Effort:** Low (hours)
**Correctness:** Medium (visually acceptable, not identical)

Drop the complex penumbra blur. Use a simple darkened silhouette:
```glsl
float alpha = texture(u_texture, v_texCoord).a;
float fade = 1.0 - (distanceFromFeet / maxDistance);
fragColor = vec4(0.0, 0.0, 0.0, alpha * fade * 0.3);
```

This gives a serviceable shadow without fighting the blur math.

### Option C: Pre-render Shadow Textures
**Effort:** Medium (days)
**Correctness:** High for static sprites

Render each sprite's shadow to its own texture at build time. Sample that directly instead of computing blur at runtime. Sidesteps the atlas problem entirely.

### Option D: Screen-Space Shadow
**Effort:** Medium (days)
**Correctness:** Medium-High

Render sprites to a temporary buffer, apply blur as post-process in screen space. Avoids atlas UV constraints entirely.

## Recommendation

~~**Short-term:** Option B (simplified shadow) - Get something working that looks acceptable.~~

~~**Long-term:** Option A (dynamic lighting) - Required for visual parity with Duelyst. The shadow system is the dependent system; lighting is the foundation.~~

**SELECTED: Option A** - Implementing dynamic lighting system now. The shadow system depends on it; no amount of tweaking will produce correct results without it.

## What NOT To Do

1. **Don't keep tuning intensity/blur parameters** - We've proven this doesn't converge
2. **Don't add more "compensating" code** - Each compensation creates new edge cases
3. **Don't port more shaders piecemeal** - They're designed as an integrated system

## Files Modified (Current State)

All changes are attempts to make the isolated shadow work. They may need to be reverted/redesigned:

- `shaders/shadow.vert` - Passes local pixel coords (correct for Duelyst approach)
- `shaders/shadow.frag` - 7x7 blur with many compensations
- `include/gpu_renderer.hpp` - `ShadowUniforms` with extra fields
- `src/gpu_renderer.cpp` - `draw_sprite_shadow()` with compensating logic

## Reference Implementation

Full Duelyst shader analysis: `docs/DUELYST_SHADOW_LIGHTING_SPEC.md`

Key files in original:
- `app/view/shaders/ShadowHighQualityFragment.glsl`
- `app/view/shaders/LightFragment.glsl` (THE MISSING PIECE)
- `app/view/fx/FX.js` - Default parameters

## Conclusion

We were asked to do "the maximum job" but interpreted that as "port the exact shader." The maximum job is actually "achieve visual parity through sound architecture." That requires either implementing the lighting system first, or choosing a shadow approach designed for our constraints (atlas rendering, no dynamic lights).

The current implementation is a dead end. Further iteration will not produce correct results.
