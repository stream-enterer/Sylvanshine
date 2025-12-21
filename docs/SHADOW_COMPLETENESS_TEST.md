# Shadow System Completeness Test

## Purpose
Trace every input to Duelyst's shadow shaders and verify Sylvanshine can provide equivalent data.

---

## Shader Inputs Trace

### Vertex Shader (ShadowVertex.glsl)

| Input | Type | Duelyst Source | Sylvanshine Equivalent | Status |
|-------|------|----------------|------------------------|--------|
| `a_position` | vec4 | Sprite quad vertices | `ShadowVertex.x/y` | ✓ Have |
| `a_texCoord` | vec2 | Sprite UV coords | `ShadowVertex.u/v` | ✓ Have |
| `a_color` | vec4 | Light color + effective alpha | `Light.r/g/b` + computed alpha | ✓ Have |
| `a_originRadius` | vec4 | Light xyz position + radius | `Light.x/y/z` + `Light.radius` | ✓ Have |
| `u_size` | vec2 | Sprite content size | `sprite_pass->width/height` | ✓ Have |
| `u_anchor` | vec2 | (anchorX*width, shadowOffset) | Computed in C++ | ✓ Have |
| `CC_MVMatrix` | mat4 | Modelview matrix | **Not directly available** | ⚠️ See below |
| `CC_PMatrix` | mat4 | Projection matrix | **Not directly available** | ⚠️ See below |

### Fragment Shader (ShadowHighQualityFragment.glsl)

| Input | Type | Duelyst Source | Sylvanshine Equivalent | Status |
|-------|------|----------------|------------------------|--------|
| `u_intensity` | float | `FX.shadowIntensity` (0.15) | `fx_config.shadow_intensity` | ✓ Have (currently 0.5) |
| `u_blurShiftModifier` | float | `FX.shadowBlurShiftModifier` (1.0) | `fx_config.shadow_blur_shift` | ✓ Have |
| `u_blurIntensityModifier` | float | `FX.shadowBlurIntensityModifier` (3.0) | `fx_config.shadow_blur_intensity` | ✓ Have |
| `v_mv_lightPosition` | vec3 | From vertex shader (light 3D pos) | Need to compute | ⚠️ Derive |
| `v_mv_lightRadius` | float | From vertex shader | `Light.radius` | ✓ Have |
| `v_mv_castedAnchorPosition` | vec3 | From vertex shader (casted anchor) | Need to compute | ⚠️ Derive |
| `v_fragmentColor` | vec4 | Light color from `a_color` | Light color | ✓ Have |
| `v_position` | vec2 | Original sprite position | `ShadowVertex.lx/ly` | ✓ Have |
| `v_texCoord` | vec2 | Texture coordinates | `ShadowVertex.u/v` | ✓ Have |

---

## Critical Finding: Modelview Matrix

Duelyst uses `CC_MVMatrix` (Cocos2d modelview matrix) for:

1. **Transforming anchor position** (line 24):
   ```glsl
   vec3 mv_anchorPosition = (CC_MVMatrix * vec4(u_anchor.x, 0.0, 0.0, 1.0)).xzy;
   ```

2. **Transforming casted position** (line 64):
   ```glsl
   vec4 mv_castedPosition = CC_MVMatrix * castedPosition;
   ```

### Why This Matters

The modelview matrix in Duelyst's isometric view converts 2D screen coordinates to a fake 3D space where:
- X = horizontal
- Y = depth (distance into screen)
- Z = height (vertical position)

**However**, Sylvanshine can bypass this because:
- We're already in screen space
- We have light position in screen space (x, y) plus height (z)
- We can compute skew/stretch directly from geometric relationships

---

## Light Altitude Calculation (Critical Discovery)

From `Light.js:319`:
```javascript
const altitude = Math.sqrt(node.radius) * 6 * scale;
```

**The light's Z (height) is derived from its radius!**

| Radius | Altitude (at scale 1.0) |
|--------|-------------------------|
| 100 | 60 |
| 285 (default) | 101.2 |
| 400 | 120 |

This explains why larger lights cast different shadows - they're "higher up".

---

## Skew/Stretch Calculation (From Vertex Shader)

### Skew (horizontal tilt based on light position)
```glsl
vec3 mv_anchorDir = mv_anchorDifference / mv_anchorDistance;
float skew = tan(atan(mv_anchorDir.x, mv_anchorDir.z) * flip) * 0.5;
```

In 2D terms: `skew = tan(atan(light_dx / light_altitude)) * 0.5`

Where `light_dx = sprite_x - light_x`

### Stretch (vertical elongation based on light altitude angle)
```glsl
float altitudeModifier = pow(1.0 / abs(tan(abs(atan(mv_anchorDir.y, mv_anchorDir.z)))), 0.35) * 1.25;
float skewModifier = min(pow(skewAbs, 0.1) / skewAbs, 1.0);
float mv_stretch = min(skewModifier * altitudeModifier, 1.6);
```

### Cast Matrix (45° projection)
```glsl
mat4 castMatrix = mat4(
    1.0, 0.0, 0.0, 0.0,
    skew, 0.7071067811865475 * flip, -0.7071067811865475 * flip, 0.0,
    0.0, 0.7071067811865475 * flip, 0.7071067811865475 * flip, 0.0,
    0.0, 0.0, 0.0, 1.0
);
```

Note: `0.7071067811865475 = cos(45°) = sin(45°) = 1/√2`

---

## Implementation Path: CPU vs GPU

### Option 1: GPU (Full Duelyst Port)
- Port vertex shader math to GLSL
- Pass light position as vertex attribute or uniform
- Compute skew/stretch/cast in vertex shader

**Risks:**
- Need to handle coordinate space differences (Duelyst had modelview matrix)
- More complex debugging

### Option 2: CPU (Simpler, Recommended)
- Compute skew/stretch in C++ when building shadow quad
- Apply transformations to vertex positions directly
- Pass pre-transformed vertices to simple vertex shader

**Advantages:**
- We already build vertices in `draw_shadow_from_pass`
- Easier to debug (can log values)
- No coordinate space translation needed

---

## Completeness Verdict

### What We Have
- ✓ All uniform values
- ✓ Light position (x, y, z)
- ✓ Light radius and color
- ✓ Sprite geometry and texture
- ✓ Blur calculation formulas
- ✓ Intensity falloff formulas

### What We Need to Add
1. **Light altitude derivation**: `altitude = sqrt(radius) * 6 * scale`
2. **Skew calculation**: `tan(atan(dx / altitude)) * 0.5`
3. **Stretch calculation**: altitude-based modifier
4. **Cast matrix application**: 45° projection with skew

### No Cascading Inconsistencies Expected

All the math is self-contained in the vertex shader. The fragment shader only needs:
- `v_position` (local sprite position) - already have
- `v_texCoord` (texture coords) - already have
- Light distance calculation - can compute from position
- Blur/intensity uniforms - already have

**The GPU approach would work** because we have all required inputs. The only translation needed is handling the coordinate space difference, which is straightforward since we're computing positions on CPU anyway.

---

## Recommended Test

Before implementing, verify our Light.z values are correct:

```cpp
// In draw_shadow_from_pass, add:
if (active_light) {
    float expected_altitude = std::sqrt(active_light->radius) * 6.0f * scale;
    SDL_Log("Light: pos=(%.1f, %.1f) radius=%.1f altitude=%.1f (z=%.1f)",
            active_light->x, active_light->y, active_light->radius,
            expected_altitude, /* actual light->z if we have it */);
}
```

This confirms the altitude calculation matches Duelyst before implementing dynamic geometry.
