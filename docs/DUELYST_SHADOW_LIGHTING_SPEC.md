# Duelyst Shadow & Lighting System Specification

This document provides a complete technical specification of Duelyst's shadow and lighting systems, extracted from source analysis. It serves as the authoritative reference for porting to Sylvanshine.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Component Architecture](#2-component-architecture)
3. [Data Structures](#3-data-structures)
4. [Shader Specifications](#4-shader-specifications)
5. [Render Pipeline](#5-render-pipeline)
6. [Configuration Constants](#6-configuration-constants)
7. [Mathematical Formulas](#7-mathematical-formulas)
8. [Architectural Mismatches](#8-architectural-mismatches)
9. [Implementation Plan](#9-implementation-plan)

---

## 1. System Overview

Duelyst uses a **dynamic point-light system** where:
- Multiple light sources exist in the scene with position, radius, color, and intensity
- Each sprite can be an **occluder** (receives lighting) and/or **shadow caster** (casts shadows from lights)
- Shadows are cast **FROM light sources**, with dynamic direction based on light-to-sprite angle
- A penumbra effect creates progressive blur: sharp at feet, blurry toward head

### Quality Levels

```javascript
CONFIG.SHADOW_QUALITY_HIGH   = 1.0   // ShadowHighQualityFragment (7x7 blur)
CONFIG.SHADOW_QUALITY_MEDIUM = 0.5   // ShadowLowQualityFragment (3x3 blur)
CONFIG.SHADOW_QUALITY_LOW    = 0.0   // Shadows disabled

CONFIG.LIGHTING_QUALITY_HIGH   = 1.0
CONFIG.LIGHTING_QUALITY_MEDIUM = 0.5
CONFIG.LIGHTING_QUALITY_LOW    = 0.0
```

---

## 2. Component Architecture

### 2.1 Light Node (`app/view/nodes/fx/Light.js`)

Point light source in the scene.

```javascript
Light = cc.Node.extend({
    castsShadows: true,           // Whether this light casts shadows
    duration: 0.0,                // Lifetime (0 = infinite)
    fadeInDuration: 0.0,          // Fade in time
    fadeInDurationPct: 0.35,      // Fade in as % of duration
    fadeOutDuration: 0.0,         // Fade out time
    fadeOutDurationPct: 1.5,      // Fade out as % of duration
    intensity: 1.0,               // Light intensity (higher dims lower)
    radius: CONFIG.TILESIZE * 3.0 // Light radius (default 285px)
});
```

**Key Methods:**
- `getEffectiveIntensity()`: Returns `intensity * (realColor.a/255) * (displayedOpacity/255)`
- `getMVPosition3D()`: Model-view space 3D position (y↔z swapped for pseudo-3D)

**Altitude Calculation** (from `_updateBounds`):
```javascript
const altitude = Math.sqrt(node.radius) * 6 * scale;
mvPosition3D.y = mvPosition3D.z + altitude;  // Lift light above ground
mvPosition3D.z = y;  // Original y becomes depth
```

### 2.2 BatchLights (`app/view/fx/BatchLights.js`)

Batches all light sources for efficient rendering.

**Vertex Attributes (13 floats per vertex):**
| Index | Name | Description |
|-------|------|-------------|
| 0-2 | position | xyz world position |
| 3-4 | texCoord | uv (screen-space for lights) |
| 5-8 | color | rgba (light color + alpha) |
| 9-11 | origin | light center xyz |
| 12 | radius | light radius |

**Intensity Normalization:**
```javascript
// Lights sorted by effective intensity, alpha adjusted
if (maxIntensity !== 1.0) {
    alpha *= Math.pow(effectiveIntensity / maxIntensity, 0.5);
}
```

### 2.3 BatchLighting (`app/view/fx/BatchLighting.js`)

Per-occluder batch that combines occluder geometry with light data.

**Vertex Attributes (5 floats per vertex):**
| Index | Name | Description |
|-------|------|-------------|
| 0-2 | position | xyz occluder position |
| 3-4 | texCoord | uv texture coordinates |

**Shared Data:** Uses `BatchLights.verticesBuffer` for color and origin/radius via `gl.vertexAttribPointer`.

### 2.4 BaseSprite Occlusion (`app/view/nodes/BaseSprite.js`)

Sprites that participate in lighting/shadows.

**Key Properties:**
```javascript
isOccluding: false,           // Receives lighting
shadowOffset: CONFIG.DEPTH_OFFSET,  // 19.5 pixels
normal: cc.kmVec3(0, 0, 1),   // Sprite plane normal
```

---

## 3. Data Structures

### 3.1 Vertex Formats

**Shadow Vertex (passed to ShadowVertex.glsl):**
```c
attribute vec4 a_position;     // Local sprite position
attribute vec2 a_texCoord;     // Texture coordinates
attribute vec4 a_color;        // Light color (from BatchLights)
attribute vec4 a_originRadius; // Light position xyz + radius
```

**Lighting Vertex (passed to LightingVertex.glsl):**
```c
attribute vec4 a_position;     // Occluder quad position
attribute vec4 a_color;        // Light color
attribute vec4 a_originRadius; // Light origin + radius
```

### 3.2 Uniforms

**Shadow Uniforms:**
| Uniform | Type | Source | Description |
|---------|------|--------|-------------|
| `u_size` | vec2 | sprite contentSize | Sprite width/height in pixels |
| `u_anchor` | vec2 | (anchorX*width, shadowOffset) | Feet position |
| `u_intensity` | float | FX.shadowIntensity (0.15) | Base shadow darkness |
| `u_blurShiftModifier` | float | FX.shadowBlurShiftModifier (1.0) | Blur ramp exponent |
| `u_blurIntensityModifier` | float | FX.shadowBlurIntensityModifier (3.0) | Blur radius multiplier |

**Lighting Uniforms:**
| Uniform | Type | Description |
|---------|------|-------------|
| `u_depthRange` | float | Pseudo-3D depth range |
| `u_depthOffset` | float | Depth offset for ordering |
| `u_lightMapScale` | float | Light map texture scale |
| `u_depthRotationMatrix` | mat4 | Isometric rotation |
| `u_normal` | vec3 | Sprite surface normal |
| `u_falloffModifier` | float | Light falloff exponent |
| `u_intensityModifier` | float | Light intensity multiplier |

---

## 4. Shader Specifications

### 4.1 ShadowVertex.glsl (71 lines)

**Purpose:** Transform sprite vertices to cast shadow at 45° angle from light source.

**Inputs:**
- `a_position`: Local sprite vertex position
- `a_texCoord`: Texture coordinates
- `a_color`: Light RGBA
- `a_originRadius`: Light xyz + radius

**Outputs (Varyings):**
- `v_texCoord`: Passed through
- `v_fragmentColor`: Light color with alpha
- `v_mv_lightPosition`: Light position in MV space
- `v_mv_lightRadius`: Light radius
- `v_position`: **LOCAL vertex position (critical for blur calculation)**
- `v_mv_castedAnchorPosition`: For light distance calculation

**Key Algorithm:**

```glsl
// 1. Get anchor in modelview space
vec3 mv_anchorPosition = (CC_MVMatrix * vec4(u_anchor.x, 0.0, 0.0, 1.0)).xzy;
mv_anchorPosition.z += u_anchor.y;  // Add shadow offset to depth

// 2. Calculate flip based on light depth
float mv_depthDifference = mv_depth - v_mv_lightPosition.z;
float flip = mv_depthDifference + u_anchor.y < 0.0 ? -1.0 : 1.0;

// 3. Calculate skew from light direction
vec3 mv_anchorDifference = mv_depthAnchorPosition - v_mv_lightPosition;
vec3 mv_anchorDir = normalize(mv_anchorDifference);
float skew = tan(atan(mv_anchorDir.x, mv_anchorDir.z) * flip) * 0.5;

// 4. Build 45° cast matrix
mat4 castMatrix = mat4(
    1.0, 0.0, 0.0, 0.0,
    skew, 0.7071067811865475 * flip, -0.7071067811865475 * flip, 0.0,
    0.0, 0.7071067811865475 * flip, 0.7071067811865475 * flip, 0.0,
    0.0, 0.0, 0.0, 1.0
);
// Note: 0.7071... = cos(45°) = sin(45°) = 1/sqrt(2)

// 5. Calculate stretch (closer to light = shorter shadow)
float altitudeModifier = pow(1.0 / abs(tan(abs(atan(mv_anchorDir.y, mv_anchorDir.z)))), 0.35) * 1.25;
float skewModifier = min(pow(abs(skew), 0.1) / abs(skew), 1.0);
float mv_stretch = min(skewModifier * altitudeModifier, 1.6);

// 6. Apply transformation
vec4 castedPosition = a_position;
castedPosition.y = (castedPosition.y - u_anchor.y) * mv_stretch;
castedPosition = castMatrix * castedPosition;

// 7. Pass local position for fragment shader
v_position = a_position.xy;
```

### 4.2 ShadowHighQualityFragment.glsl (123 lines)

**Purpose:** Calculate shadow alpha with progressive blur (penumbra effect).

**Key Algorithm:**

```glsl
// 1. Light distance attenuation
vec3 lightDiff = v_mv_lightPosition - v_mv_castedAnchorPosition;
float lightDist = length(lightDiff);
float lightDistPct = pow(lightDist / v_mv_lightRadius, 2.0);
float lightDistPctInv = 1.0 - lightDistPct;  // 1 at light, 0 at radius edge

// 2. Distance from anchor (feet)
vec2 anchorDiff = v_position - u_anchor;  // Uses LOCAL position!
float sizeRadius = length(u_size) * 0.5;

// 3. Blur modifier - ZERO at feet, increases with distance
float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);
float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);
float blurX = (1.0/u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
float blurY = (1.0/u_size.y * u_blurIntensityModifier) * occluderDistPctBlurModifier;

// CRITICAL: At feet (occluderDistPctY = 0), blurX = blurY = 0 (sharp!)
// At head (occluderDistPctY = 1), blurX/Y = blurIntensityModifier/size

// 4. Intensity falloff
float occluderDistPctX = min(abs(anchorDiff.x) / sizeRadius, 1.0);
float intensityFadeX = pow(1.0 - occluderDistPctX, 1.25);  // Fade at sides
float intensityFadeY = pow(1.0 - occluderDistPctY, 1.5);   // Fade toward head
float intensity = intensityFadeX * intensityFadeY * u_intensity;

// 5. 7x7 box blur
float alpha = 0.0;
float boxWeight = 1.0 / 49.0;
for (y = -3 to 3) {
    for (x = -3 to 3) {
        alpha += texture2D(tex, v_texCoord + vec2(x*blurX, y*blurY)).a * boxWeight;
    }
}

// 6. Final color
gl_FragColor = vec4(0.0, 0.0, 0.0, min(1.0, lightDistPctInv * alpha * intensity * v_fragmentColor.a));
```

### 4.3 ShadowLowQualityFragment.glsl (71 lines)

Same logic as high quality but with 3x3 blur instead of 7x7.

### 4.4 LightingVertex.glsl (53 lines)

Transforms occluder vertices for lighting accumulation with pseudo-3D depth.

### 4.5 LightingFragment.glsl (25 lines)

```glsl
// Light contribution based on distance and angle
float falloff = 1.0 - pow(min(dist / radius, 1.0), u_falloffModifier);
float angle = dot(v_normal, dir);
float intensity = clamp(angle * u_intensityModifier, 0.0, 1.0);
gl_FragColor = vec4(v_fragmentColor.rgb * intensity, v_fragmentColor.a * falloff);
```

### 4.6 MultipliedLightingFragment.glsl (13 lines)

Composites lighting map with sprite diffuse:
```glsl
vec4 lightColor = texture2D(u_lightMap, v_texCoord);
vec3 finalLight = max(lightColor.rgb, u_ambientColor);
gl_FragColor = vec4(texture.rgb * finalLight, texture.a);
```

---

## 5. Render Pipeline

### 5.1 Per-Frame Flow

```
For each frame:
1. Update BatchLights (rebuild if dirty)
2. Update BatchShadowCastingLights (rebuild if dirty)
3. For each occluding sprite:
   a. cacheDrawLighting() → Lighting FBO
   b. drawShadows() → Direct to screen
4. Draw sprites with MultipliedLighting shader
```

### 5.2 cacheDrawLighting() - Lighting Accumulation

```javascript
// Set up lighting FBO
_lightingPass.beginWithResetClear();

// Use Lighting shader
lightingProgram.use();
lightingProgram.setUniformLocationWith1f(loc_depthRange, ...);
lightingProgram.setUniformLocationWith1f(loc_depthOffset, ...);
lightingProgram.setUniformLocationWith1f(loc_lightMapScale, ...);
lightingProgram.setUniformLocationWithMatrix4fv(loc_depthRotationMatrix, ...);
lightingProgram.setUniformLocationWith3f(loc_normal, ...);

// Additive blending for light accumulation
gl.blendFunc(gl.SRC_ALPHA, gl.ONE);

// Render all lights affecting this occluder
_batchLighting.renderWithLights();

_lightingPass.end();
```

### 5.3 drawShadows() - Shadow Rendering

```javascript
// Select quality level
if (CONFIG.shadowQuality === CONFIG.SHADOW_QUALITY_HIGH) {
    shadowProgram = cc.shaderCache.programForKey('ShadowHighQuality');
} else {
    shadowProgram = cc.shaderCache.programForKey('ShadowLowQuality');
}
shadowProgram.use();

// Get size from composite or content
let width, height;
if (isCompositing) {
    width = _compositePass.getWidth();
    height = _compositePass.getHeight();
    cc.glBindTexture2DN(0, _compositePass.getTexture());
} else {
    width = node._contentSize.width;
    height = node._contentSize.height;
    cc.glBindTexture2DN(0, node._texture);
}

// Set uniforms
shadowProgram.setUniformLocationWith2f(loc_size, width, height);
shadowProgram.setUniformLocationWith2f(loc_anchor,
    node._anchorPoint.x * width,  // X anchor in pixels
    node.shadowOffset);            // Y = CONFIG.DEPTH_OFFSET = 19.5

// Standard alpha blending
gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

// Render shadow for each shadow-casting light
_batchLighting.renderWithShadowCastingLights();
```

---

## 6. Configuration Constants

From `app/common/config.js`:

```javascript
CONFIG.DEPTH_OFFSET = 19.5;              // Shadow anchor Y offset (pixels)
CONFIG.TILESIZE = 95;                    // Tile size (pixels)
CONFIG.globalScale = 1.0;                // Global render scale

// Shadow defaults (from FX.js)
shadowIntensity: 0.15,                   // Base shadow opacity
shadowBlurShiftModifier: 1.0,            // Blur ramp exponent
shadowBlurIntensityModifier: 3.0,        // Blur radius in texels

// Lighting defaults
falloffModifier: 1.0,                    // Light falloff exponent
intensityModifier: 1.0,                  // Light intensity multiplier
ambientColor: [0.3, 0.3, 0.3],          // Scene ambient light
```

---

## 7. Mathematical Formulas

### 7.1 Shadow Cast Matrix (45°)

```
castMatrix = [
    1,     0,         0,          0
    skew,  cos(45°),  -sin(45°),  0
    0,     sin(45°),  cos(45°),   0
    0,     0,         0,          1
]

where cos(45°) = sin(45°) = 0.7071067811865475 = 1/√2
```

### 7.2 Blur Radius (in texture coordinates)

```
occluderDistPctY = clamp(max(v_position.y - u_anchor.y, 0) / u_size.x, 0, 1)
blurModifier = pow(occluderDistPctY, u_blurShiftModifier)
blurX = (1.0 / u_size.x) * u_blurIntensityModifier * blurModifier
blurY = (1.0 / u_size.y) * u_blurIntensityModifier * blurModifier
```

**At feet (y = anchor.y):** `occluderDistPctY = 0`, `blurModifier = 0`, `blur = 0` (sharp!)
**At head (y = anchor.y + size.x):** `occluderDistPctY = 1`, `blurModifier = 1`, `blur = blurIntensity/size`

### 7.3 Intensity Falloff

```
occluderDistPctX = min(abs(v_position.x - u_anchor.x) / (length(u_size) * 0.5), 1.0)
occluderDistPctY = min(max(v_position.y - u_anchor.y, 0) / u_size.x, 1.0)
intensityFadeX = pow(1.0 - occluderDistPctX, 1.25)
intensityFadeY = pow(1.0 - occluderDistPctY, 1.5)
intensity = intensityFadeX * intensityFadeY * u_intensity
```

### 7.4 Light Distance Attenuation

```
lightDist = length(lightPosition - castedAnchorPosition)
lightDistPct = pow(lightDist / lightRadius, 2.0)
lightDistPctInv = 1.0 - lightDistPct
```

### 7.5 Final Shadow Alpha

```
finalAlpha = min(1.0, lightDistPctInv * blurredAlpha * intensity * lightAlpha)
```

---

## 8. Architectural Mismatches

### 8.1 Coordinate System

| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Coordinates | Cocos2d modelview matrices | Direct NDC |
| Origin | Bottom-left | Top-left (SDL) |
| Y-axis | Up is positive | Down is positive |
| Pseudo-3D | y↔z swap for depth | Flat 2D |

### 8.2 Rendering Model

| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Light sources | Dynamic point lights | None (fixed sun) |
| Shadow direction | Cast from light position | Fixed 45° |
| Batching | Shared VBOs (BatchLights) | Per-draw VBO upload |
| Multi-pass | Lighting FBO + shadow pass | Single immediate pass |
| Texture | Composite pass texture | Direct spritesheet |

### 8.3 Vertex Data

| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Position varying | `v_position` = local sprite coords | `v_quadPos` = 0-1 normalized |
| Size uniform | `u_size` = sprite contentSize | `u_sprite_size` = frame size |
| Anchor uniform | `u_anchor` = (anchorX*width, 19.5) | Not passed |
| Light data | `a_originRadius` attribute | None |

### 8.4 Critical Differences Causing Previous Bugs

1. **v_position vs v_quadPos:** Duelyst uses local pixel coordinates for blur calculation, we used 0-1 normalized. This changes all distance calculations.

2. **u_size meaning:** Duelyst uses sprite contentSize (full sprite dimensions), we used frame size from atlas.

3. **Anchor calculation:** Duelyst uses `(anchorX * width, shadowOffset)` in pixel units, we computed differently.

4. **Blur at feet:** Duelyst's formula guarantees zero blur at feet because `occluderDistPctY = 0` when `v_position.y <= u_anchor.y`.

---

## 9. Implementation Plan

### Phase 1: Accurate Shadow Rendering (No Dynamic Lights)

#### 9.1 Shader Changes

**shadow.vert:**
```glsl
// Change from:
layout(location = 2) in vec2 a_quadPos;
layout(location = 1) out vec2 v_quadPos;

// To:
layout(location = 2) in vec2 a_localPos;  // Local sprite position in pixels
layout(location = 1) out vec2 v_position;  // For blur calculation
```

**shadow.frag:**
```glsl
// Add uniforms matching Duelyst
uniform vec2 u_size;      // Sprite content size in pixels
uniform vec2 u_anchor;    // (anchorX * width, shadowOffset)
uniform float u_intensity;
uniform float u_blurShiftModifier;
uniform float u_blurIntensityModifier;

// Use Duelyst's exact blur formula
vec2 anchorDiff = v_position - u_anchor;
float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);
float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);
float blurX = (1.0/u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
float blurY = (1.0/u_size.y * u_blurIntensityModifier) * occluderDistPctBlurModifier;
```

#### 9.2 C++ Changes

**ShadowVertex struct:**
```cpp
struct ShadowVertex {
    float x, y;           // NDC position
    float u, v;           // Texture coordinates
    float local_x, local_y;  // Local sprite position in pixels (NOT 0-1)
};
```

**ShadowUniforms struct:**
```cpp
struct ShadowUniforms {
    float opacity;            // External opacity control
    float intensity;          // 0.15 default
    float blur_shift;         // 1.0 default
    float blur_intensity;     // 3.0 default
    float size_x, size_y;     // Sprite content size
    float anchor_x, anchor_y; // (anchorX * width, SHADOW_OFFSET)
};
```

**draw_sprite_shadow() changes:**
```cpp
// Calculate local positions for vertices
// For a sprite of size (w, h) with anchor at (0.5, 0):
// Top-left local: (0, h)      -> after flip: (0, 0)
// Top-right local: (w, h)     -> after flip: (w, 0)
// Bottom-left local: (0, 0)   -> after flip: (0, h)
// Bottom-right local: (w, 0)  -> after flip: (w, h)

float anchor_x = 0.5f * src.w;  // Horizontal center
float anchor_y = SHADOW_OFFSET; // 19.5 pixels

ShadowVertex vertices[4] = {
    {x0, y0, u0, v0, 0.0f, 0.0f},           // top-left (feet)
    {x1, y1, u1, v0, src.w, 0.0f},          // top-right (feet)
    {x2, y2, u1, v1, src.w, src.h},         // bottom-right (head)
    {x3, y3, u0, v1, 0.0f, src.h},          // bottom-left (head)
};

ShadowUniforms uniforms = {
    opacity,
    0.15f,              // intensity
    1.0f,               // blur_shift
    3.0f,               // blur_intensity
    src.w, src.h,       // size
    anchor_x, anchor_y  // anchor
};
```

### Phase 2: Dynamic Lighting (Future)

Add `Light` class, `BatchLights`, per-sprite lighting accumulation FBO.

---

## Appendix A: File References

| Component | Duelyst Path |
|-----------|--------------|
| Light node | `app/view/nodes/fx/Light.js` |
| BatchLights | `app/view/fx/BatchLights.js` |
| BatchLighting | `app/view/fx/BatchLighting.js` |
| BaseSprite occlusion | `app/view/nodes/BaseSprite.js:1260-1327` |
| FX manager | `app/view/fx/FX.js` |
| ShadowVertex | `app/shaders/ShadowVertex.glsl` |
| ShadowHighQualityFragment | `app/shaders/ShadowHighQualityFragment.glsl` |
| ShadowLowQualityFragment | `app/shaders/ShadowLowQualityFragment.glsl` |
| LightingVertex | `app/shaders/LightingVertex.glsl` |
| LightingFragment | `app/shaders/LightingFragment.glsl` |
| MultipliedLightingFragment | `app/shaders/MultipliedLightingFragment.glsl` |
| Config | `app/common/config.js` |

## Appendix B: Sylvanshine Current State

| Component | Path |
|-----------|------|
| Shadow vertex shader | `shaders/shadow.vert` |
| Shadow fragment shader | `shaders/shadow.frag` |
| GPU renderer | `src/gpu_renderer.cpp` |
| Header | `include/gpu_renderer.hpp` |
| Shadow constants | `include/types.hpp` (SHADOW_OFFSET, etc.) |
