# Shadow System Replication Guide

## Purpose

This document enables an LLM to replicate Duelyst's unit shadow visuals in Sylvanshine's SDF-based shadow system. It contains complete shader specifications, uniform mappings, and implementation deltas.

---

## Part 1: Duelyst Shadow System Specification

### 1.1 Shader Files

| Shader | File Path | Type | Purpose |
|--------|-----------|------|---------|
| ShadowVertex | `app/shaders/ShadowVertex.glsl` | Vertex | Projects sprite silhouette onto ground plane |
| ShadowHighQualityFragment | `app/shaders/ShadowHighQualityFragment.glsl` | Fragment | 7x7 box blur with distance-based softening |
| ShadowLowQualityFragment | `app/shaders/ShadowLowQualityFragment.glsl` | Fragment | 3x3 box blur with distance-based softening |

### 1.2 Quality Selection

```javascript
// app/view/nodes/BaseSprite.js:1298-1302
if (CONFIG.shadowQuality === CONFIG.SHADOW_QUALITY_HIGH) {
  shadowProgram = cc.shaderCache.programForKey('ShadowHighQuality');
} else {
  shadowProgram = cc.shaderCache.programForKey('ShadowLowQuality');
}
```

Default: `CONFIG.SHADOW_QUALITY_HIGH` (set in `app/common/config.js:1339`)

### 1.3 Uniforms

| Uniform | Type | Default | Source | Purpose |
|---------|------|---------|--------|---------|
| `u_size` | vec2 | sprite dimensions | `BaseSprite.js:1306` | Shadow quad size in pixels |
| `u_anchor` | vec2 | (anchorX * width, shadowOffset) | `BaseSprite.js:1309` | Shadow pivot point; shadowOffset = 19.5 |
| `u_intensity` | float | 0.15 | `FX.js:249` | Base shadow darkness multiplier |
| `u_blurShiftModifier` | float | 1.0 | `FX.js:250` | Exponent for distance-based blur curve |
| `u_blurIntensityModifier` | float | 3.0 | `FX.js:251` | Blur kernel scale factor |

### 1.4 Vertex Shader Algorithm (ShadowVertex.glsl)

#### 1.4.1 Coordinate System

Duelyst uses Cocos2d (Y-up). Light position undergoes Y/Z swap in `app/view/nodes/fx/Light.js:319-322`:

```javascript
const altitude = Math.sqrt(node.radius) * 6 * scale;
const { y } = this.mvPosition3D;
this.mvPosition3D.y = this.mvPosition3D.z + altitude;  // Y becomes HEIGHT
this.mvPosition3D.z = y;  // Z becomes DEPTH (original screen Y)
```

After transformation:
- X = horizontal position (unchanged)
- Y = height/altitude above ground
- Z = depth (distance from camera, derived from screen Y)

#### 1.4.2 Depth-Based Flip (lines 27-30)

```glsl
float mv_depth = mv_anchorPosition.z;
float mv_depthDifference = mv_depth - v_mv_lightPosition.z;
float flip = mv_depthDifference + u_anchor.y < 0.0 ? -1.0 : 1.0;
```

Where `u_anchor.y` = `CONFIG.DEPTH_OFFSET` = 19.5 pixels.

| Condition | flip | Shadow Direction |
|-----------|------|------------------|
| `mv_depthDifference + 19.5 < 0` | -1.0 | Upward (away from camera) |
| `mv_depthDifference + 19.5 >= 0` | +1.0 | Downward (toward camera) |

#### 1.4.3 Cast Matrix (lines 38-43)

```glsl
mat4 castMatrix = mat4(
    1.0, 0.0, 0.0, 0.0,
    skew, 0.7071067811865475 * flip, -0.7071067811865475 * flip, 0.0,
    0.0, 0.7071067811865475 * flip, 0.7071067811865475 * flip, 0.0,
    0.0, 0.0, 0.0, 1.0
);
```

The constant `0.7071067811865475` = cos(45°) = sin(45°) for isometric projection.

#### 1.4.4 Skew Calculation (lines 33-36)

```glsl
float skew = 0.0;
if (abs(mv_lightDirY) > 0.01) {
    skew = -mv_lightDirX / mv_lightDirY;
}
```

Skew is the horizontal offset applied to shadow vertices based on light direction.

### 1.5 Fragment Shader Algorithm (ShadowHighQualityFragment.glsl)

#### 1.5.1 Light Distance Falloff (lines 17-21)

```glsl
vec3 lightDiff = v_mv_lightPosition - v_mv_castedAnchorPosition;
float lightDist = length(lightDiff);
float lightDistPct = pow(lightDist / v_mv_lightRadius, 2.0);
float lightDistPctInv = 1.0 - lightDistPct;
```

Shadow intensity decreases quadratically with distance from light.

#### 1.5.2 Distance-Based Blur (lines 23-32)

```glsl
vec2 anchorDiff = v_position - u_anchor;
float occluderDistPctX = min(abs(anchorDiff.x) / sizeRadius, 1.0);
float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);  // NOTE: intentionally uses .x (width)
float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);
float blurX = (1.0/u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
float blurY = (1.0/u_size.y * u_blurIntensityModifier) * occluderDistPctBlurModifier;
```

Key behavior: Blur increases with vertical distance from anchor (feet). At feet, blur = 0 (sharp). At head, blur = maximum (soft).

**WARNING**: Line 29 uses `u_size.x` (width) not `u_size.y` (height) for vertical distance normalization. This is intentional in Duelyst — it creates aspect-ratio-dependent blur behavior where wider sprites have more gradual blur falloff.

#### 1.5.3 Intensity Fade (lines 34-37)

```glsl
float intensityFadeX = pow(1.0 - occluderDistPctX, 1.25);
float intensityFadeY = pow(1.0 - occluderDistPctY, 1.5);
float intensity = intensityFadeX * intensityFadeY * u_intensity;
```

Shadow opacity decreases toward edges (X) and toward head (Y).

#### 1.5.4 Blur Kernel

High quality: 7x7 box blur (49 samples), `boxWeight = 1.0 / 49.0`
Low quality: 3x3 box blur (9 samples), `boxWeight = 1.0 / 9.0`

#### 1.5.5 Final Output (line 121)

```glsl
gl_FragColor = vec4(0.0, 0.0, 0.0, min(1.0, lightDistPctInv * alpha * intensity * v_fragmentColor.a));
```

### 1.6 Shadow Registration Pipeline

1. `BaseSprite.setupShadowCasting()` (`BaseSprite.js:445-447`) registers sprite with FX system
2. `ShadowCastingLayer.renderingShadows()` (`ShadowCastingLayer.js:33-41`) iterates all shadow casters
3. `BaseSprite.drawShadows()` (`BaseSprite.js:1290-1326`) renders individual shadow with uniforms

### 1.7 Default Values Summary

| Parameter | Value | Source |
|-----------|-------|--------|
| Shadow intensity | 0.15 | `FX.js:249` |
| Blur shift modifier | 1.0 | `FX.js:250` |
| Blur intensity modifier | 3.0 | `FX.js:251` |
| Depth offset | 19.5 | `config.js:DEPTH_OFFSET` |
| Isometric angle | 45° | Hardcoded as 0.7071067811865475 |

---

## Part 2: Sylvanshine Shadow System Specification

### 2.1 Shader Files

| Shader | File Path | Type | Purpose |
|--------|-----------|------|---------|
| sdf_shadow.vert | `shaders/sdf_shadow.vert` | Vertex | Passes NDC position, UV, local position |
| sdf_shadow.frag | `shaders/sdf_shadow.frag` | Fragment | SDF-based soft shadow with distance fade |

### 2.2 Uniforms

| Uniform | Type | Purpose | Duelyst Equivalent |
|---------|------|---------|-------------------|
| `u_opacity` | float | External opacity control | `v_fragmentColor.a` |
| `u_intensity` | float | Shadow darkness | `u_intensity` |
| `u_penumbraScale` | float | Penumbra softness multiplier | None (blur is distance-based in Duelyst) |
| `u_sdfMaxDist` | float | SDF encoding range (default 32.0) | N/A |
| `u_spriteSize` | vec2 | Sprite dimensions | `u_size` |
| `u_anchor` | vec2 | Shadow pivot point | `u_anchor` |
| `u_lightDir` | vec2 | Normalized direction to light | Derived from `v_mv_lightPosition` |
| `u_lightDistance` | float | Distance to light | `lightDist` |
| `u_lightIntensity` | float | Light contribution | `v_mv_lightRadius` affects falloff |

### 2.3 Fragment Shader Algorithm (sdf_shadow.frag)

#### 2.3.1 SDF Sampling (lines 32-36)

```glsl
float encoded = texture(u_sdfTexture, v_texCoord).r;
float sdf = encoded * 2.0 * u_sdfMaxDist - u_sdfMaxDist;
```

SDF texture encodes signed distance: negative = inside silhouette, positive = outside.

#### 2.3.2 Penumbra Calculation (lines 47-53)

```glsl
float penumbraWidth = u_penumbraScale * 8.0;
float shadowAlpha = 1.0 - smoothstep(-penumbraWidth * 0.5, penumbraWidth, sdf);
```

Current: Penumbra width is CONSTANT across entire shadow.

#### 2.3.3 Distance Fade (lines 55-61)

```glsl
float distFromFeet = max(u_anchor.y - v_localPos.y, 0.0);
float fadeRange = u_spriteSize.y * 0.8;
float distanceFade = 1.0 - smoothstep(0.0, fadeRange, distFromFeet);
```

Shadow opacity decreases from feet toward head. Matches Duelyst's `intensityFadeY`.

#### 2.3.4 Side Fade (lines 63-65)

```glsl
float sideDistance = abs(v_localPos.x - u_anchor.x) / (u_spriteSize.x * 0.5);
float sideFade = 1.0 - smoothstep(0.5, 1.2, sideDistance);
```

**BUG**: `sideFade` is calculated but NOT APPLIED in the final output. See 2.3.5.

#### 2.3.5 Final Output (lines 67-70)

```glsl
float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity;
fragColor = vec4(0.0, 0.0, 0.0, clamp(finalAlpha, 0.0, 1.0));
```

**BUG**: `sideFade` is missing from this calculation. To match Duelyst, should be:

```glsl
float finalAlpha = shadowAlpha * u_intensity * distanceFade * sideFade * u_opacity;
```

### 2.4 Projection (CPU-side)

Shadow projection is calculated in `gpu_renderer.cpp:1148-1290` (`draw_sdf_shadow`):
- Isometric 45° projection (COS_45 = 0.7071067811865475)
- Depth-based flip matching Duelyst's algorithm
- Skew based on light direction
- Stretch based on light altitude

---

## Part 3: Parameter Mapping (Duelyst → Sylvanshine)

### 3.1 Direct Mappings (Already Implemented)

| Duelyst Parameter | Sylvanshine Parameter | Status |
|-------------------|----------------------|--------|
| `u_intensity` (0.15) | `u_intensity` | Set to 0.15 for match |
| `u_size` | `u_spriteSize` | Equivalent |
| `u_anchor` | `u_anchor` | Equivalent |
| `intensityFadeY` | `distanceFade` | Similar (see 3.1.1) |
| `intensityFadeX` | `sideFade` | Similar but NOT APPLIED (see 2.3.5 BUG) |
| Depth-based flip | `shadow_below` in `gpu_renderer.cpp:1213` | Equivalent |
| Isometric skew | `skew` in `gpu_renderer.cpp:1218-1221` | Equivalent |
| 45° projection | `COS_45` constant | Equivalent |

#### 3.1.1 Fade Curve Differences

Duelyst uses power functions; Sylvanshine uses smoothstep:

| Fade | Duelyst | Sylvanshine |
|------|---------|-------------|
| Vertical | `pow(1.0 - y, 1.5)` | `1.0 - smoothstep(0.0, fadeRange, dist)` |
| Horizontal | `pow(1.0 - x, 1.25)` | `1.0 - smoothstep(0.5, 1.2, dist)` |

The curves are similar but not identical. For exact match, Sylvanshine would need to replace smoothstep with power functions:

```glsl
// Duelyst-exact vertical fade:
float distanceFade = pow(1.0 - clamp(distFromFeet / u_spriteSize.y, 0.0, 1.0), 1.5);

// Duelyst-exact horizontal fade:
float sideFade = pow(1.0 - clamp(sideDistance, 0.0, 1.0), 1.25);
```

### 3.2 Missing: Distance-Based Penumbra

Duelyst's blur INCREASES with distance from anchor:

```glsl
// Duelyst: blur scales with vertical distance
float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);
float blurX = (1.0/u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
```

Sylvanshine's penumbra is CONSTANT:

```glsl
// Sylvanshine: constant penumbra width
float penumbraWidth = u_penumbraScale * 8.0;
```

#### 3.2.1 Required Change

Replace lines 47-53 in `shaders/sdf_shadow.frag`:

```glsl
// CURRENT (constant penumbra):
float penumbraWidth = u_penumbraScale * 8.0;
float shadowAlpha = 1.0 - smoothstep(-penumbraWidth * 0.5, penumbraWidth, sdf);

// REPLACEMENT (distance-based penumbra):
float distFromAnchorNorm = clamp(max(u_anchor.y - v_localPos.y, 0.0) / u_spriteSize.y, 0.0, 1.0);
float blurModifier = pow(distFromAnchorNorm, u_blurShiftModifier);
float penumbraWidth = u_penumbraScale * 8.0 * (0.1 + blurModifier * 0.9);
float shadowAlpha = 1.0 - smoothstep(-penumbraWidth * 0.5, penumbraWidth, sdf);
```

#### 3.2.2 Required New Uniform

Add to `SDFShadowUniforms` struct in `sdf_shadow.frag`:

```glsl
float u_blurShiftModifier;  // Exponent for distance-based blur curve (default: 1.0)
```

### 3.3 Missing: Light Distance Falloff

Duelyst shadows fade based on distance from light source:

```glsl
// Duelyst: quadratic falloff
float lightDistPct = pow(lightDist / v_mv_lightRadius, 2.0);
float lightDistPctInv = 1.0 - lightDistPct;
// Applied in final: lightDistPctInv * alpha * intensity
```

Sylvanshine has `u_lightDistance` uniform but does not use it in the shader.

#### 3.3.1 Required Change

Add after line 65 in `shaders/sdf_shadow.frag`:

```glsl
// Light distance falloff (Duelyst-style)
float lightDistPct = pow(u_lightDistance / u_lightRadius, 2.0);
float lightFalloff = 1.0 - clamp(lightDistPct, 0.0, 1.0);
```

Modify line 68:

```glsl
// CURRENT:
float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity;

// REPLACEMENT:
float finalAlpha = shadowAlpha * u_intensity * distanceFade * lightFalloff * u_opacity;
```

#### 3.3.2 Required New Uniform

Add to `SDFShadowUniforms`:

```glsl
float u_lightRadius;  // Light radius for falloff calculation
```

---

## Part 4: Implementation Checklist

### 4.1 To Match Duelyst Shadows (70% Already Done)

| Feature | Status | Action Required |
|---------|--------|-----------------|
| Shadow intensity | Ready | Set `u_intensity = 0.15` |
| Vertical fade (feet→head) | Ready | Already implemented as `distanceFade` |
| Isometric 45° projection | Ready | Already in `gpu_renderer.cpp` |
| Depth-based flip | Ready | Already in `gpu_renderer.cpp:1213` |
| Light-based skew | Ready | Already in `gpu_renderer.cpp:1218` |

### 4.2 To Match Duelyst Shadows (30% Remaining)

| Feature | Status | Action Required | Priority |
|---------|--------|-----------------|----------|
| Horizontal fade (sideFade) | BUG | Apply `sideFade` in final output (`sdf_shadow.frag:68`) | High |
| Distance-based penumbra | Missing | Modify `sdf_shadow.frag:47-53`, add `u_blurShiftModifier` uniform | High |
| Light distance falloff | Missing | Add `u_lightRadius` uniform, apply quadratic falloff | Medium |
| Exact fade curves | Different | Replace smoothstep with pow() functions (see 3.1.1) | Low |

### 4.3 C++ Changes Required

For new uniforms, update `SDFShadowUniforms` struct in `src/gpu_renderer.cpp`:

```cpp
struct SDFShadowUniforms {
    float u_opacity;
    float u_intensity;
    float u_penumbraScale;
    float u_sdfMaxDist;

    float u_spriteSize[2];
    float u_anchor[2];

    float u_lightDir[2];
    float u_lightDistance;
    float u_lightIntensity;

    // NEW: Add these for Duelyst match
    float u_blurShiftModifier;  // default: 1.0
    float u_lightRadius;        // default: scene light radius
    float _pad[2];              // alignment padding
};
```

Update uniform binding in `draw_sdf_shadow()` to populate new fields.

### 4.4 Uniform Defaults for Duelyst Match

| Uniform | Value | Notes |
|---------|-------|-------|
| `u_intensity` | 0.15 | Duelyst default |
| `u_penumbraScale` | 1.0 | Adjust to taste after implementing distance-based scaling |
| `u_blurShiftModifier` | 1.0 | Duelyst default; controls blur curve steepness |
| `u_sdfMaxDist` | 32.0 | Depends on SDF generation parameters |

---

## Part 5: Validation Criteria

An LLM implementing these changes should verify:

1. **Sharp feet, soft head**: Shadow edge should be crisp at anchor point, progressively blurrier toward top of sprite
2. **Intensity gradient**: Shadow should be darkest at anchor, fading toward edges and top
3. **Flip behavior**: Shadow should project downward when light is behind sprite (higher screen Y), upward when light is in front
4. **Skew direction**: Shadow should skew opposite to light's horizontal component
5. **Light falloff**: Shadow should be less intense when sprite is far from light source

---

## Part 6: File References

### Duelyst Source Files

| File | Lines | Content |
|------|-------|---------|
| `app/shaders/ShadowVertex.glsl` | 1-71 | Vertex projection with cast matrix |
| `app/shaders/ShadowHighQualityFragment.glsl` | 1-123 | 7x7 blur fragment shader |
| `app/shaders/ShadowLowQualityFragment.glsl` | 1-71 | 3x3 blur fragment shader |
| `app/view/nodes/BaseSprite.js` | 1290-1326 | Shadow rendering call |
| `app/view/fx/FX.js` | 244-257 | Uniform initialization |
| `app/view/nodes/fx/Light.js` | 319-322 | Coordinate transformation |
| `app/common/config.js` | 1216-1218, 1339 | Quality constants and defaults |

### Sylvanshine Source Files

| File | Lines | Content |
|------|-------|---------|
| `shaders/sdf_shadow.vert` | 1-17 | Vertex passthrough |
| `shaders/sdf_shadow.frag` | 1-71 | SDF fragment shader |
| `src/gpu_renderer.cpp` | 1148-1290 | Shadow projection and rendering |

---

## Part 7: Data Model Difference

| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Shadow source | Sprite alpha channel (binary silhouette) | SDF texture (signed distance field) |
| Blur method | Post-projection box blur (49 texture samples) | SDF smoothstep (1 texture sample) |
| Penumbra source | Blur kernel size + distance scaling | SDF gradient width |
| Performance | O(49) samples per fragment | O(1) samples per fragment |

The SDF approach is more efficient but requires the distance-based penumbra scaling to be implemented in the smoothstep parameters rather than blur kernel scaling.
