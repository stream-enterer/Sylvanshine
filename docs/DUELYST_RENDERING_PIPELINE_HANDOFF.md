# Duelyst Rendering Pipeline: Complete Handoff Document

**Date:** 2025-12-21
**Purpose:** Comprehensive specification for migrating Sylvanshine's rendering to match Duelyst's multi-pass pipeline
**Status:** SPECIFICATION - Ready for Implementation

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Render Pass System](#3-render-pass-system)
4. [FX Manager](#4-fx-manager)
5. [Lighting System](#5-lighting-system)
6. [Shadow System](#6-shadow-system)
7. [Bloom Pipeline](#7-bloom-pipeline)
8. [Sprite System](#8-sprite-system)
9. [Shader Catalog](#9-shader-catalog)
10. [Configuration Constants](#10-configuration-constants)
11. [Implementation Checklist](#11-implementation-checklist)
12. [Migration Strategy](#12-migration-strategy)

---

## 1. Executive Summary

### Current Sylvanshine State
- Single-pass rendering directly to swapchain
- Atlas-based sprite textures
- Simplified shadow shader (no dynamic lighting)
- No bloom, depth buffer, or post-processing

### Target Duelyst State
- **14+ render passes** (FBOs)
- Per-sprite texture compositing for shadows/lighting
- Dynamic point light system with shadow casting
- Full post-processing pipeline (bloom, radial blur, tone curve, color grading)
- Depth buffer for pseudo-3D sorting and effects

### Why This Matters
Duelyst's visual quality comes from its multi-pass compositing. Each sprite is rendered to its own texture before shadow/lighting calculations, then composited. This is why our atlas-based shadows don't match - we're skipping the intermediate pass.

---

## 2. Architecture Overview

### Duelyst Rendering Stack

```
┌─────────────────────────────────────────────────────────────────┐
│                        SCREEN OUTPUT                            │
└─────────────────────────────────────────────────────────────────┘
                                ▲
                                │
┌─────────────────────────────────────────────────────────────────┐
│                    POST-PROCESSING                              │
│  ┌─────────┐  ┌──────────┐  ┌───────────┐  ┌────────────────┐  │
│  │ RadialB │→ │ ToneCurve│→ │GradientMap│→ │ Final Composite│  │
│  └─────────┘  └──────────┘  └───────────┘  └────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                ▲
                                │
┌─────────────────────────────────────────────────────────────────┐
│                      BLOOM PIPELINE                             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────────────────┐  │
│  │Highpass │→ │  Blur   │→ │ Bloom   │→ │ BloomCompositeA/B │  │
│  └─────────┘  └─────────┘  └─────────┘  └───────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                ▲
                                │
┌─────────────────────────────────────────────────────────────────┐
│                    SURFACE COMPOSITE                            │
│  ┌───────────────┐                    ┌───────────────┐        │
│  │  SurfaceA     │ ◄──────────────────│  SurfaceB     │        │
│  │ (double-buf)  │                    │ (double-buf)  │        │
│  └───────────────┘                    └───────────────┘        │
│         ▲                                     ▲                 │
│         │                                     │                 │
│    ┌────┴────┐                          ┌────┴────┐            │
│    │ Sprites │                          │ Shadows │            │
│    │ + Light │                          │ + Light │            │
│    └─────────┘                          └─────────┘            │
└─────────────────────────────────────────────────────────────────┘
                                ▲
                                │
┌─────────────────────────────────────────────────────────────────┐
│                  PER-SPRITE COMPOSITING                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ For each sprite that casts shadows or receives light:    │  │
│  │   1. Render sprite to its own FBO (UV 0-1)               │  │
│  │   2. Render shadow using sprite FBO + light data         │  │
│  │   3. Render lit sprite using light accumulation          │  │
│  │   4. Composite to surface                                │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                ▲
                                │
┌─────────────────────────────────────────────────────────────────┐
│                      DEPTH BUFFER                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Sprites write depth for pseudo-3D sorting              │   │
│  │  Used by: DepthTest shader, distortion effects          │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Key Insight
**Each shadow-casting sprite gets its own temporary FBO.** This is why Duelyst shadows look correct - the shadow shader samples from a texture where UV 0-1 covers the entire sprite, not a small region of an atlas.

---

## 3. Render Pass System

### Source: `app/view/fx/RenderPass.js`

A RenderPass is a WebGL FBO wrapper that provides:
- Framebuffer + texture creation
- Viewport management
- Projection matrix handling
- Quad rendering helpers

### All Render Passes (14)

| Pass Name | Purpose | Scale | Format | Depth/Stencil |
|-----------|---------|-------|--------|---------------|
| `cache` | Cached screen contents | 1.0 | RGBA8 | No |
| `screen` | Main screen buffer | 1.0 | RGBA8 | No |
| `blurComposite` | Blur composite | 1.0 | RGBA8 | No |
| `surfaceA` | Surface double-buffer A | 1.0 | RGBA8 | No |
| `surfaceB` | Surface double-buffer B | 1.0 | RGBA8 | No |
| `depth` | Depth buffer (packed RGBA) | 1.0 | RGBA8 | No |
| `highpass` | Bloom extraction | 0.5 | RGBA8 | No |
| `blur` | Bloom blur | 0.5 | RGBA8 | No |
| `bloom` | Bloom accumulation | 0.5 | RGBA8 | No |
| `bloomCompositeA` | Bloom double-buffer A | 0.5 | RGBA8 | No |
| `bloomCompositeB` | Bloom double-buffer B | 0.5 | RGBA8 | No |
| `radialBlur` | Screen radial blur | 1.0 | RGBA8 | No |
| `toneCurve` | Color grading | 1.0 | RGBA8 | No |
| `gradientColorMap` | Color mapping | 1.0 | RGBA8 | No |

### Per-Sprite Passes (Dynamic)
Created on-demand for sprites that need compositing:
- Light accumulation pass per occluding sprite
- Shadow pass per shadow-casting sprite

---

## 4. FX Manager

### Source: `app/view/fx/FX.js`

The FX singleton manages all visual effects and rendering state.

### Tracked Objects

```javascript
lights: [],              // All point lights
shadowCastingLights: [], // Lights that cast shadows
occluders: [],           // Sprites that receive light
shadowCasters: [],       // Sprites that cast shadows
decals: [],              // Ground decals
distortions: [],         // Screen distortion effects
```

### Batch Systems

| Batch | Purpose | Attributes |
|-------|---------|------------|
| `BatchLights` | Batches all light sources | 13 floats/vertex: pos(3), color(4), origin(4), radius(1), custom(1) |
| `BatchLighting` | Per-sprite lighting calc | 5 floats/vertex: pos(3), uv(2) |
| `BatchShadowCastingLights` | Shadow-casting lights | Same as BatchLights |

### FX Parameters

| Parameter | Default | Purpose |
|-----------|---------|---------|
| `ambientLightColor` | {r:89, g:89, b:89} | Base ambient when no lights |
| `falloffModifier` | 1.0 | Light spread within radius |
| `intensityModifier` | 1.0 | Light brightness on facing pixels |
| `shadowIntensity` | 0.15 | Shadow darkness |
| `shadowBlurShiftModifier` | 1.0 | Blur gradient steepness |
| `shadowBlurIntensityModifier` | 3.0 | Blur amount |
| `bloomThreshold` | 0.6 | Bloom extraction cutoff |
| `bloomIntensity` | 2.5 | Bloom brightness multiplier |
| `bloomTransition` | 0.15 | Bloom temporal smoothing |
| `bloomScale` | 0.5 | Bloom FBO scale |

---

## 5. Lighting System

### Source: `app/view/nodes/fx/Light.js`, `app/view/fx/BatchLights.js`

### Light Properties

```javascript
{
  position: cc.p(x, y),       // Screen position
  radius: 285,                // Light radius (CONFIG.TILESIZE * 3.0)
  intensity: 2.0,             // Light intensity (normalized internally)
  color: cc.color(r, g, b),   // Light color
  castsShadows: true,         // Whether light creates shadows
  fadeWithDistance: true,     // Distance attenuation
  isPrimaryLight: false       // Main scene light flag
}
```

### Lighting Shader Flow

1. **LightingVertex.glsl** - Transforms occluder vertices with depth
2. **LightingFragment.glsl** - Calculates per-pixel light contribution
3. **MultipliedLightingFragment.glsl** - Composites lighting with sprite diffuse

### Light Distance Attenuation Formula

```glsl
// In LightingFragment.glsl / ShadowHighQualityFragment.glsl
vec3 lightDiff = v_mv_lightPosition - v_mv_castedAnchorPosition;
float lightDist = length(lightDiff);
float lightDistPct = pow(lightDist / v_mv_lightRadius, 2.0);
float lightDistPctInv = 1.0 - lightDistPct;  // 1 at center, 0 at edge
```

### Lighting Accumulation Process

```
For each occluding sprite:
  1. Create/reuse light accumulation FBO
  2. Clear to ambient color
  3. For each affecting light:
     - Bind LightingVertex + LightingFragment
     - Render sprite quad with light data
     - Additive blend to accumulation FBO
  4. Use accumulation texture when drawing sprite
     - Bind MultipliedLightingFragment
     - Multiply sprite diffuse by light accumulation
```

---

## 6. Shadow System

### Source: `app/shaders/ShadowVertex.glsl`, `app/shaders/ShadowHighQualityFragment.glsl`

### Shadow Casting Process

```
For each shadow-casting sprite:
  For each shadow-casting light:
    1. Render sprite to temporary FBO (UV 0-1 = sprite)
    2. Calculate shadow projection matrix from light angle
    3. Bind ShadowVertex + ShadowHighQualityFragment
    4. Render shadow with:
       - Progressive blur (sharp at feet, soft at head)
       - Light distance attenuation
       - Intensity falloff from anchor point
```

### Key Shadow Uniforms

| Uniform | Type | Purpose |
|---------|------|---------|
| `u_size` | vec2 | Sprite content size in pixels |
| `u_anchor` | vec2 | (anchorX * width, shadowOffset) |
| `u_intensity` | float | Shadow darkness (default 0.15) |
| `u_blurShiftModifier` | float | Blur gradient steepness |
| `u_blurIntensityModifier` | float | Blur amount (default 3.0) |

### Shadow Vertex Transformation

```glsl
// 45 degree shadow cast matrix
mat4 castMatrix = mat4(
  1.0, 0.0, 0.0, 0.0,
  skew, 0.7071 * flip, -0.7071 * flip, 0.0,
  0.0, 0.7071 * flip, 0.7071 * flip, 0.0,
  0.0, 0.0, 0.0, 1.0
);

// Stretch based on light altitude
float altitudeModifier = pow(1.0 / abs(tan(abs(atan(mv_anchorDir.y, mv_anchorDir.z)))), 0.35) * 1.25;
```

### Why Our Current Shadows Don't Work

| Issue | Duelyst | Sylvanshine |
|-------|---------|-------------|
| Texture source | Per-sprite FBO (UV 0-1) | Atlas region (~10% of texture) |
| OOB sampling | Clamps to sprite edge | Returns 0 (dilutes blur) |
| Light data | Per-vertex from BatchLights | Per-sprite uniform |
| Shadow orientation | Dynamic from light angle | Fixed 45° |

---

## 7. Bloom Pipeline

### Source: `app/view/fx/FX.js` (createPasses, bloom methods)

### Pipeline Stages

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────────┐
│ Surface  │────▶│ Highpass │────▶│   Blur   │────▶│    Bloom     │
│ (input)  │     │(extract) │     │(gaussian)│     │ (accumulate) │
└──────────┘     └──────────┘     └──────────┘     └──────────────┘
                                                          │
                                                          ▼
┌──────────────────┐     ┌──────────────────┐     ┌──────────────┐
│ BloomCompositeB  │◀────│ BloomCompositeA  │◀────│    Blend     │
│  (prev frame)    │     │  (curr frame)    │     │ (temporal)   │
└──────────────────┘     └──────────────────┘     └──────────────┘
```

### Bloom Shaders

| Shader | Purpose |
|--------|---------|
| HighpassFragment | Extract pixels above threshold |
| BlurFragment (generated) | Gaussian blur kernel |
| BloomFragment | Temporal accumulation with decay |
| SurfaceFragment | Final composite with bloom |

### Bloom Constants

```javascript
CONFIG.BLOOM_DEFAULT = 0.7;      // Default bloom amount
CONFIG.BLOOM_GLOW = 1.0;         // Bloom for glow effects
CONFIG.BLOOM_MAX = 1.0;          // Maximum bloom
```

---

## 8. Sprite System

### Source: `app/view/nodes/BaseSprite.js`

### Sprite Properties Affecting Rendering

| Property | Type | Default | Purpose |
|----------|------|---------|---------|
| `occludes` | bool | false | Receives light |
| `castsShadows` | bool | false | Casts shadows |
| `needsDepthDraw` | bool | false | Write to depth buffer |
| `needsDepthTest` | bool | false | Read depth buffer |
| `depthOffset` | float | 0.0 | Artificial depth adjustment |
| `depthModifier` | float | 0.0 | 0=facing screen, 1=flat on ground |
| `shadowOffset` | float | 0.0 | Feet position from sprite bottom |
| `lightMapScale` | float | 0.5 | Light accumulation FBO scale |
| `ambientLightColor` | color | black | Per-sprite ambient |
| `staticShadow` | string | null | Static shadow sprite (no dynamic) |

### Composite Components

BaseSprite uses component system for multi-pass rendering:

| Component | Purpose |
|-----------|---------|
| `CompositePass` | General composite pass |
| `CompositeHorizontalPass` | Horizontal blur pass |
| `CompositeVerticalPass` | Vertical blur pass |
| `CompositeVerticalBeforePass` | Pre-vertical effects |
| `CompositeVerticalAfterPass` | Post-vertical effects |

### Per-Sprite Lighting Integration

```javascript
// In BaseSprite render command
if (this.occludes && lights.length > 0) {
  // 1. Get or create light accumulation pass
  let lightPass = this.getLightAccumulationPass();

  // 2. Render lights to accumulation
  lightPass.beginWithClear(ambient.r, ambient.g, ambient.b, 255);
  for (let light of lights) {
    batchLighting.renderWithLights(1, lightIndex);
  }
  lightPass.end();

  // 3. Draw sprite with light map
  gl.bindTexture(gl.TEXTURE_2D, lightPass.getTexture().getName());
  // Use MultipliedLightingFragment shader
}
```

---

## 9. Shader Catalog

### All 96 Shaders by Category

#### Core Rendering (6)
| Shader | Purpose |
|--------|---------|
| PosTexVertex | Basic position + texture |
| PosTexColorVertex | Position + texture + color |
| PosVertex | Position only |
| ColorizeFragment | Apply color tint |
| MonochromeFragment | Grayscale conversion |
| TintingFragment | Color tinting |

#### Lighting & Shadows (6)
| Shader | Purpose |
|--------|---------|
| LightingVertex | Transform occluders with depth |
| LightingFragment | Per-pixel light calculation |
| MultipliedLightingFragment | Composite sprite + light |
| ShadowVertex | Shadow projection transform |
| ShadowHighQualityFragment | 7x7 blur shadow (49 samples) |
| ShadowLowQualityFragment | 3x3 blur shadow (9 samples) |

#### Depth System (6)
| Shader | Purpose |
|--------|---------|
| DepthVertex | Depth buffer write |
| DepthFragment | Pack depth to RGBA |
| DepthTestVertex | Depth test transform |
| DepthTestFragment | Depth comparison |
| DepthUnpack (helper) | Unpack RGBA to depth |
| DepthPack (helper) | Pack depth to RGBA |

#### Bloom & Blur (6)
| Shader | Purpose |
|--------|---------|
| HighpassFragment | Extract bright pixels |
| BloomFragment | Temporal bloom accumulation |
| RadialBlurFragment | Screen radial blur |
| ExtrusionBlurFragment | Directional extrusion blur |
| SurfaceFragment | Final surface composite |
| FXAAFragment | Anti-aliasing |

#### Dissolve & Transitions (4)
| Shader | Purpose |
|--------|---------|
| DissolveFragment | Death/despawn effect |
| DissolveFromCenterWithDiscFragment | Radial dissolve |
| GradientColorMapFragment | Color grade transition |
| ToneCurveFragment | Tone mapping |

#### FX Effects (30+)
| Category | Examples |
|----------|----------|
| Glow | GlowFragment, GlowVertex, GlowNoiseFragment |
| Chromatic | ChromaticFragment, ChromaticVertex, ChromaticFlareFragment |
| Fire | FireRingFragment, FireLinearWaveFragment |
| Distortion | DistortionFragment, ShockwaveFragment, VortexFragment |
| Noise | Noise2D, Noise3D, NoiseFBM, NoiseAbsFBM |
| Special | EnergyBallFragment, CausticFragment, LensFlareFragment |

#### Helpers (20+)
| Helper | Purpose |
|--------|---------|
| Noise2D/Noise3D | Procedural noise |
| Fresnel | Edge glow calculation |
| Reflection | Environment reflection |
| Refraction | Light bending |
| AdjustLevels | Levels adjustment |
| SeededRandom | Deterministic random |

---

## 10. Configuration Constants

### Source: `app/common/config.js`

### Rendering Constants

```javascript
// Board
CONFIG.TILESIZE = 95;            // Tile size in pixels
CONFIG.BOARDROW = 5;             // Board height
CONFIG.BOARDCOL = 9;             // Board width

// Depth
CONFIG.DEPTH_OFFSET = 19.5;      // Z-ordering offset
CONFIG.ENTITY_XYZ_ROTATION = {x:26, y:0, z:0};  // Isometric tilt

// Animation Timing
CONFIG.FADE_FAST_DURATION = 0.2;
CONFIG.FADE_MEDIUM_DURATION = 0.35;
CONFIG.FADE_SLOW_DURATION = 1.0;
CONFIG.ANIMATE_FAST_DURATION = 0.2;
CONFIG.ANIMATE_MEDIUM_DURATION = 0.35;
CONFIG.ANIMATE_SLOW_DURATION = 1.0;

// Effects
CONFIG.BLOOM_DEFAULT = 0.7;
CONFIG.HIGH_DAMAGE = 7;          // Screen shake threshold
CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_DURATION = 0.35;
CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_STRENGTH = 20.0;

// Particles
CONFIG.MAX_FX_PER_EVENT = 5;
CONFIG.PARTICLE_SEQUENCE_DELAY = 0.5;

// Audio
CONFIG.DEFAULT_SFX_VOLUME = 0.3;
CONFIG.DEFAULT_MUSIC_VOLUME = 0.04;
```

---

## 11. Implementation Checklist

### Phase 1: Core Infrastructure
- [ ] **RenderPass class** - FBO wrapper with begin/end/clear
- [ ] **Pass manager** - Create and manage all 14+ passes
- [ ] **Double-buffering** - Surface A/B, BloomComposite A/B
- [ ] **Viewport management** - Per-pass viewport switching

### Phase 2: Depth System
- [ ] **Depth pass** - RGBA-packed depth buffer
- [ ] **DepthVertex shader** - Write depth values
- [ ] **DepthFragment shader** - Pack depth to RGBA
- [ ] **DepthTest shader** - Read and compare depth
- [ ] **Sprite depth properties** - depthOffset, depthModifier

### Phase 3: Lighting System
- [ ] **Light class** - Position, radius, color, intensity
- [ ] **BatchLights** - Batch all lights for GPU
- [ ] **BatchLighting** - Per-sprite lighting calculation
- [ ] **LightingVertex shader** - Transform with depth
- [ ] **LightingFragment shader** - Per-pixel light calc
- [ ] **MultipliedLightingFragment** - Composite sprite + light
- [ ] **Per-sprite light accumulation FBO**
- [ ] **Ambient light support**

### Phase 4: Shadow System
- [ ] **Per-sprite shadow FBO** - Render sprite to own texture
- [ ] **ShadowVertex shader** - Dynamic shadow projection
- [ ] **ShadowHighQualityFragment** - 7x7 progressive blur
- [ ] **Light-to-shadow data flow** - Position, radius, intensity
- [ ] **Shadow sorting** - Render behind sprites

### Phase 5: Bloom Pipeline
- [ ] **Highpass pass** - Extract bright pixels
- [ ] **Blur pass** - Gaussian blur
- [ ] **Bloom pass** - Accumulate bloom
- [ ] **Temporal smoothing** - BloomComposite A/B ping-pong
- [ ] **SurfaceFragment** - Final composite

### Phase 6: Post-Processing
- [ ] **Radial blur** - Screen blur effect
- [ ] **Tone curve** - Color grading
- [ ] **Gradient color map** - Color transitions
- [ ] **FXAA** - Anti-aliasing (optional)

### Phase 7: Sprite Integration
- [ ] **BaseSprite properties** - occludes, castsShadows, etc.
- [ ] **Composite components** - Multi-pass sprite rendering
- [ ] **Dissolve effect** - Death animation
- [ ] **Levels adjustment** - Per-sprite color control

---

## 12. Migration Strategy

### Recommended Order

1. **Keep current rendering working** - Don't break existing functionality
2. **Add RenderPass infrastructure** - Can coexist with current system
3. **Implement depth buffer** - Needed for lighting/shadows
4. **Implement lighting** - Core visual improvement
5. **Implement per-sprite shadow FBO** - Fixes current shadow issues
6. **Add bloom** - Major visual polish
7. **Add post-processing** - Final touches

### Key Files to Reference

| Duelyst File | Purpose |
|--------------|---------|
| `app/view/fx/RenderPass.js` | FBO wrapper implementation |
| `app/view/fx/FX.js` | Central FX manager |
| `app/view/fx/BatchLights.js` | Light batching |
| `app/view/fx/BatchLighting.js` | Per-sprite lighting |
| `app/view/nodes/BaseSprite.js` | Sprite rendering |
| `app/view/nodes/fx/Light.js` | Light source class |
| `app/shaders/*.glsl` | All 96 shaders |

### Testing Strategy

For each phase:
1. Implement the feature
2. Toggle between old/new rendering
3. Compare visual output
4. Measure performance impact
5. Commit when stable

### Performance Considerations

| Concern | Mitigation |
|---------|------------|
| Many FBOs | Reuse passes, limit per-sprite FBOs |
| Shadow overdraw | Batch shadows, frustum cull |
| Bloom cost | Scale to 0.5, skip if disabled |
| Shader switches | Sort by shader, batch draws |

---

## Appendix A: Quick Reference

### Render Order

```
1. Clear screen
2. Render depth buffer (sprites with needsDepthDraw)
3. For each shadow-casting sprite:
   a. Render sprite to temp FBO
   b. Render shadow to surface
4. For each occluding sprite:
   a. Accumulate lights to temp FBO
   b. Render sprite with lighting
5. Render non-lit sprites
6. Run bloom pipeline
7. Run post-processing
8. Output to screen
```

### Essential Formulas

**Light attenuation:**
```glsl
lightDistPctInv = 1.0 - pow(distance / radius, 2.0)
```

**Shadow blur gradient:**
```glsl
occluderDistPctY = min(max(anchorDiff.y, 0.0) / size.x, 1.0)
blurModifier = pow(occluderDistPctY, blurShiftModifier)
```

**Shadow intensity falloff:**
```glsl
intensityFadeX = pow(1.0 - occluderDistPctX, 1.25)
intensityFadeY = pow(1.0 - occluderDistPctY, 1.5)
```

---

*Document generated from Duelyst source analysis via duelyst-analyzer skill*
