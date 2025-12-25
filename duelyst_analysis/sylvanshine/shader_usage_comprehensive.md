# Duelyst Shader System: Comprehensive Usage Documentation

**Location:** `app/shaders/`
**Total Shaders:** 97 files (72 main shaders + 25 helpers)

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Shader Loading System](#shader-loading-system)
3. [Render Pipeline](#render-pipeline)
4. [Fragment Shaders](#fragment-shaders)
5. [Vertex Shaders](#vertex-shaders)
6. [Helper Modules](#helper-modules)
7. [FX Sprite Integration](#fx-sprite-integration)
8. [Post-Processing Pipeline](#post-processing-pipeline)
9. [Uniform Reference](#uniform-reference)

---

## Architecture Overview

### Technology Stack
- **Language:** GLSL (WebGL 1.0 compatible)
- **Build Tool:** glslify (modular GLSL bundling)
- **Runtime:** Cocos2d-html5 shader cache (`cc.shaderCache`)
- **Rendering:** WebGL with multi-pass framebuffer compositing

### Directory Structure
```
app/shaders/
├── *Fragment.glsl        # Pixel shaders (45 files)
├── *Vertex.glsl          # Vertex shaders (15 files)
├── helpers/              # Reusable GLSL modules (25 files)
│   ├── Noise*.glsl       # Procedural noise functions
│   ├── Depth*.glsl       # Depth buffer utilities
│   ├── Distortion.glsl   # Refraction/reflection
│   └── Pos*Vertex.glsl   # Standard vertex shaders
└── blurShaderGenerator.coffee  # Dynamic blur shader factory
```

---

## Shader Loading System

### Registration (FX.js)
Shaders are registered into `cc.shaderCache` with program keys. The FX system initializes all shaders at startup:

```javascript
// app/view/fx/FX.js:196-301
_initShaderUniforms() {
  const { shaderCache } = cc;

  const bloomProgram = shaderCache.programForKey('Bloom');
  bloomProgram.use();
  bloomProgram.setUniformLocationWith1f(bloomProgram.loc_transition, 0.15);
  // ... more uniform initialization
}
```

### Key Shader Programs Registered
| Program Key | Fragment Shader | Vertex Shader | Purpose |
|-------------|-----------------|---------------|---------|
| `Bloom` | BloomFragment | PosTexColorVertex | Temporal bloom accumulation |
| `Highpass` | HighpassFragment | PosTexColorVertex | Brightness threshold filter |
| `Glow` | GlowFragment | GlowVertex | Unit/card glow outline |
| `GlowNoise` | GlowNoiseFragment | GlowVertex | Animated noise glow |
| `Highlight` | HighlightFragment | HighlightVertex | Selection highlight |
| `Lighting` | LightingFragment | LightingVertex | Dynamic point lights |
| `Dissolve` | DissolveFragment | PosTexColorVertex | Death/spawn dissolve |
| `Distortion` | DistortionFragment | DistortionVertex | Screen-space refraction |
| `ShadowLowQuality` | ShadowLowQualityFragment | ShadowVertex | Fast shadow projection |
| `ShadowHighQuality` | ShadowHighQualityFragment | ShadowVertex | Detailed shadow blur |

---

## Render Pipeline

### Multi-Pass Architecture
The FX system uses 14 render passes for compositing:

```javascript
// app/view/fx/FX.js:320-351
createPasses() {
  this.passes = {
    cache:           // Temporary buffer
    screen:          // Main screen buffer
    blurComposite:   // Blur effect accumulation
    surfaceA/B:      // Ping-pong surface buffers
    depth:           // Depth buffer (RGBA packed)
    highpass:        // Bloom threshold extraction
    blur:            // Gaussian blur intermediate
    bloom:           // Final bloom result
    bloomCompositeA/B: // Temporal bloom blending
    radialBlur:      // God rays / radial blur
    toneCurve:       // Color grading
    gradientColorMap: // Faction color mapping
  };
}
```

### Rendering Flow
```
Scene Graph
    ↓
[Depth Pass] → depth buffer (packed RGBA)
    ↓
[Surface Pass] → surfaceA/B ping-pong
    ↓
[Lighting Pass] → light accumulation map
    ↓
[Shadow Pass] → shadow projection
    ↓
[Highpass] → extract bright pixels
    ↓
[Blur] → Gaussian blur
    ↓
[Bloom Composite] → temporal accumulation
    ↓
[Distortion Pass] → screen-space refraction
    ↓
[Tone Curve] → color grading
    ↓
[FXAA] → anti-aliasing
    ↓
Final Output
```

---

## Fragment Shaders

### Dissolve Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `DissolveFragment` | Death/despawn animation | `u_seed`, `u_frequency`, `u_amplitude`, `u_time`, `u_vignetteStrength`, `u_edgeFalloff` |
| `DissolveFromCenterWithDiscFragment` | Centered disc dissolve | `u_phase`, `u_time`, `u_texResolution` |

**Algorithm (DissolveFragment.glsl):**
```glsl
// Vignette from center
float noiseCenterDist = length(v_texCoord - vec2(0.5)) * 5.0 * offset;
float noiseVignette = max(0.0, 1.0 - pow(noiseCenterDist, 6)) * u_vignetteStrength;

// FBM noise with time progression
float noise = getNoiseFBM(vec2(u_seed) + v_texCoord, 0.0, u_frequency,
                          max(0.0, u_amplitude + u_time - noiseVignette));

// Edge burn effect
float noiseEdge = fract(1.0 - smoothstep(noiseRangeMin, noiseRangeMax + u_edgeFalloff, noise));
```

### Fire/Energy Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `FireRingFragment` | Circular fire expansion | `u_phase`, `u_time`, `u_color`, `u_texResolution` |
| `FireRingFlareWarpedFragment` | Warped fire ring | Same + noise warping |
| `FireLinearWaveFragment` | Linear fire wave | `u_phase`, `u_time`, `u_resolution` |
| `EnergyBallFragment` | Animated energy sphere | `u_time`, `u_timeScale`, `u_noiseLevel`, `u_texResolution` |
| `RiftFireFragment` | Dimensional rift fire | `u_time`, `u_texResolution` |

**Constants:**
```glsl
#define COMPLEXITY 2.0      // Fire noise octaves
#define INNER_RADIUS 0.75   // Energy ball core
#define OUTER_RADIUS 0.9    // Energy ball edge
#define NUM_STEPS 20        // Ray march steps
```

### Flare/Glow Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `ChromaticFlareFragment` | Chromatic explosion | `u_phase`, `u_time`, `u_size`, `u_frequency`, `u_amplitude` |
| `FbmPolarFlareFragment` | Polar coordinate flare | `u_phase`, `u_time`, `u_size`, `u_flareColor` |
| `FbmPolarFlareWipeFragment` | Flare wipe animation | `u_phase`, `u_time`, `u_size` |
| `HorizontalGlowFlareFragment` | Horizontal burst | `u_phase`, `u_time`, `u_texResolution` |
| `RarityFlareFragment` | Card rarity indicator | `u_phase`, `u_time`, `u_texResolution` |
| `LensFlareFragment` | Classic lens flare | `u_time`, `u_texResolution`, `u_origin`, `u_rampThreshold` |
| `WispLensFlareFragment` | Wisp-style flare | `u_time`, `u_pulseRate`, `u_armLength`, `u_wispSize`, `u_flareSize` |

### Distortion Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `DistortionFragment` | Screen-space refraction | `u_depthMap`, `u_refractMap`, `u_resolution`, `u_refraction`, `u_reflection`, `u_fresnelBias` |
| `ShockwaveFragment` | Impact shockwave | Same + `u_amplitude`, `u_time` |
| `VortexFragment` | Whirlpool distortion | Same + `u_radius`, `u_frequency` |
| `WaterFragment` | Water surface | Same + `u_frequency`, `u_amplitude` |

**Distortion Algorithm:**
```glsl
// Depth test to avoid distorting foreground objects
bool depthTestFailed = getDepthTestFailed(u_depthMap, screenPos, depth);

// Fresnel-based blend of refraction and reflection
vec4 distortColor = getDistortionColor(
  u_refractMap, u_resolution,
  normal, u_refraction, u_reflection, u_fresnelBias
);
```

### Lighting Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `LightingFragment` | Point light accumulation | `u_falloffModifier`, `u_intensityModifier` |
| `ShadowLowQualityFragment` | Fast shadow | `u_size`, `u_anchor`, `u_intensity`, `u_blurShiftModifier` |
| `ShadowHighQualityFragment` | Quality shadow | Same + more blur samples |
| `ShadowBlobFragment` | Soft blob shadow | `u_time`, `u_color`, `u_texResolution` |

### Post-Processing
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `BloomFragment` | Temporal bloom composite | `u_previousBloom`, `u_transition` |
| `HighpassFragment` | Brightness extraction | `u_threshold`, `u_intensity` |
| `FXAAFragment` | Anti-aliasing | `u_resolution` |
| `ToneCurveFragment` | Color grading | `u_toneCurveTexture`, `u_amount` |
| `GradientColorMapFragment` | Faction colors | `u_fromColor*`, `u_toColor*`, `u_phase` |
| `RadialBlurFragment` | God rays | `u_origin`, `u_strength`, `u_deadZone`, `u_ramp`, `u_decay` |
| `LevelsFragment` | Level adjustment | `u_inBlack`, `u_inWhite`, `u_inGamma`, `u_outBlack`, `u_outWhite` |

### Highlight/Glow
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `GlowFragment` | Simple color glow | `u_color` |
| `GlowNoiseFragment` | Animated noise glow | `u_resolution`, `u_color`, `u_frequency`, `u_amplitude`, `u_time` |
| `HighlightFragment` | Selection highlight | `u_color`, `u_threshold`, `u_intensity`, `u_brightness`, level controls |
| `GlowImageMapControlFragment` | Image-based glow | `u_time`, `u_texResolution`, `u_color`, `u_intensity`, `u_gamma` |
| `GlowImageMapRippleFragment` | Rippling glow | `u_time`, `u_texResolution`, `u_intensity` |

### Utility/Special
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `DepthFragment` | Pack depth to RGBA | `u_resolution` |
| `DepthTestFragment` | Test against depth | `u_depthMap`, `u_resolution` |
| `DepthDebugFragment` | Visualize depth | None |
| `MaskFragment` | Texture masking | `u_maskMap`, `u_maskRect` |
| `ColorizeFragment` | Simple colorization | None (uses v_fragmentColor) |
| `MonochromeFragment` | Grayscale conversion | None |
| `TintingFragment` | Color tinting | `u_tint` |
| `TimerFragment` | Circular progress | `u_progress`, `u_startingAngle`, `u_edgeGradientFactor` |

### Card Effects
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `CardAngledGradientShine` | Card shine sweep | `u_phase`, `u_time`, `u_intensity`, `u_texResolution` |
| `CoreGemFragment` | Card core gem | `u_time`, `u_texResolution`, `u_cubeMap`, `u_gemSeed` |
| `CoreGemEdgesAndColorizeFragment` | Gem edge colorization | `u_texResolution`, `u_colorBlackPoint`, `u_colorMidPoint` |

### Noise/Procedural
| Shader | Purpose | Key Uniforms |
|--------|---------|--------------|
| `CausticFragment` | Water caustics | `u_time`, `u_color`, `u_texResolution` |
| `CausticPrismaticGlowFragment` | Prismatic caustics | `u_time` |
| `VoronoiPrismaticFragment` | Voronoi pattern | `u_phase` |
| `LensNoiseFragment` | Lens noise overlay | `u_time`, `u_texResolution`, `u_flareAmount` |
| `WhiteCloudVignetteFragment` | Cloud vignette | `u_time`, `u_texResolution`, `u_vignetteAmount`, `u_noiseAmount` |
| `FbmNoiseGradientMask` | FBM gradient masking | `u_time`, `u_texResolution` |
| `FbmNoiseRays` | FBM light rays | `u_time`, `u_texResolution` |
| `RiftLineFragment` | Rift tear line | `u_time`, `u_texResolution`, `u_progress` |

---

## Vertex Shaders

| Shader | Purpose | Attributes |
|--------|---------|------------|
| `GlowVertex` | Animated glow geometry | `a_position`, `a_texCoord` |
| `HighlightVertex` | Pulsing highlight | `a_position`, `a_texCoord`, `a_color` |
| `LightingVertex` | Light volume transform | `a_position`, `a_color`, `a_originRadius` |
| `ShadowVertex` | Shadow projection | `a_position`, `a_texCoord`, `a_color`, `a_originRadius` |
| `DistortionVertex` | Distortion positioning | `a_position`, `a_texCoord`, `a_color` |
| `DepthVertex` | Depth value encoding | `a_position`, `a_texCoord` |
| `DepthTestVertex` | Depth comparison | `a_position`, `a_texCoord`, `a_color` |
| `ChromaticVertex` | Chromatic aberration | `a_position`, `a_texCoord` |
| `ScatterOcclusionVertex` | Light occlusion | `a_position`, `a_texCoord`, `a_color`, `a_originRadius` |
| `ScatterRaysVertex` | God ray positioning | `a_position`, `a_texCoord`, `a_originRadius` |

### Standard Vertex Templates
| Shader | Attributes | Use Case |
|--------|------------|----------|
| `PosVertex` | `a_position` | Fullscreen quads |
| `PosTexVertex` | `a_position`, `a_texCoord` | Simple textured |
| `PosTexColorVertex` | `a_position`, `a_texCoord`, `a_color` | Textured + vertex color |

---

## Helper Modules

### Noise System

| Helper | Function | Algorithm |
|--------|----------|-----------|
| `Noise2D.glsl` | `getNoise2D(vec2)` | 2D Simplex noise (Ashima Arts) |
| `Noise3D.glsl` | `getNoise3D(vec3)` | 3D Simplex noise (Ashima Arts) |
| `NoiseFBM.glsl` | `getNoiseFBM(vec2, time, freq, amp)` | 2-octave Fractional Brownian Motion |
| `NoiseAbsFBM.glsl` | `getNoiseAbsFBM(...)` | Absolute value FBM (ridged) |
| `NoiseRotatedFBM.glsl` | `getNoiseRotatedFBM(...)` | Time-rotated FBM |
| `NoiseRotatedFBMFromTexture.glsl` | `getNoiseRotatedFBMFromTexture(...)` | Texture-seeded rotated FBM |

**Octave Variants (NoiseAbsFBMOctaves1-6.glsl):**
Pre-configured FBM with 1-6 octaves for performance/quality tradeoff.

**Simplex Noise (Noise2D.glsl):**
```glsl
// Ian McEwan, Ashima Arts - MIT License
vec3 permute(in vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float getNoise2D(in vec2 v) {
  // Skew space to triangular grid
  vec2 i = floor(v + dot(v, C.yy));
  // Unskew back + sample 3 corners
  // ... returns -1 to 1 gradient noise
  return 130.0 * dot(m, g);
}
```

### Depth System

| Helper | Function | Purpose |
|--------|----------|---------|
| `DepthPack.glsl` | `getPackedDepth(float)` | Encode depth to RGBA8 |
| `DepthUnpack.glsl` | `getUnpackedDepth(vec4)` | Decode RGBA8 to depth |
| `DepthTest.glsl` | `getDepthTestFailed(sampler2D, vec2, float)` | Compare against depth buffer |

**Depth Packing (WebGL 1.0 workaround for no depth textures):**
```glsl
vec4 getPackedDepth(float depth) {
  const vec4 bitShift = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
  const vec4 bitMask = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
  vec4 res = fract(depth * bitShift);
  res -= res.xxyz * bitMask;
  return res;
}
```

### Optical Effects

| Helper | Function | Purpose |
|--------|----------|---------|
| `Fresnel.glsl` | `getFresnel(vec3, vec3, float)` | Edge glow factor |
| `Reflection.glsl` | `getReflectionColor(sampler2D, vec2, vec3)` | Reflect sample |
| `Refraction.glsl` | `getRefractionColor(sampler2D, vec2, vec3, float)` | Refract sample |
| `Distortion.glsl` | `getDistortionColor(...)` | Combined refract+reflect+fresnel |
| `DistortionFromNormalMap.glsl` | `getDistortionColorFromNormalMap(...)` | Normal-map based distortion |

**Fresnel Approximation:**
```glsl
float getFresnel(vec3 normal, vec3 eye, float bias) {
  return bias + (1.0 - bias) * pow(1.0 + dot(normal, eye), 5.0);
}
```

### Utility

| Helper | Function | Purpose |
|--------|----------|---------|
| `AdjustLevels.glsl` | `adjustLevels(...)` | Photoshop-style levels |
| `SeededRandom.glsl` | `SeededRandom(vec2)` | Deterministic pseudo-random |

---

## FX Sprite Integration

### FX Sprite Classes Using Shaders

| FX Class | Shader Key | Effect Description |
|----------|------------|-------------------|
| `FXCardShineSprite` | `CardAngledGradientShine` | Animated card shine sweep |
| `FXCausticSprite` | `Caustic` | Water caustic light patterns |
| `FXChromaticFlareSprite` | `ChromaticFlare` | Chromatic explosion effect |
| `FXDissolveWithDiscFromCenterSprite` | `DissolveWithDiscFromCenter` | Centered disc dissolve |
| `FXDistortionSprite` | `Distortion` | Screen-space refraction |
| `FXEnergyBallSprite` | `EnergyBall` | Animated energy sphere |
| `FXFbmNoiseGradientMaskedSprite` | `FbmNoiseGradientMask` | FBM noise with gradient |
| `FXFbmNoiseRaysSprite` | `FbmNoiseRays` | FBM light rays |
| `FXFbmPolarFlareSprite` | `FbmPolarFlare` | Polar coordinate flare |
| `FXFbmPolarFlareWipeSprite` | `FbmPolarFlareWipe` | Animated polar wipe |
| `FXFireLinearWaveSprite` | `FireLinearWave` | Linear fire wave |
| `FXFireRingSprite` | `FireRing` | Circular fire expansion |
| `FXFireRingFlareWarpedSprite` | `FireRingFlareWarped` | Warped fire ring |
| `FXGlowImageMap` | `GlowImageMapControl` | Image-controlled glow |
| `FXHorizontalGlowFlareSprite` | `HorizontalGlowFlare` | Horizontal glow burst |
| `FXLensFlareSprite` | `WispLensFlare` | Animated lens flare |
| `FXProceduralDistortionSprite` | `Distortion` | Procedural distortion |
| `FXRarityFlareSprite` | `RarityFlare` | Card rarity indicator |
| `FXRiftFireSprite` | `RiftFire` | Dimensional rift fire |
| `FXRiftLineSprite` | `RiftLine` | Rift tear line |
| `FXRipplingGlowImageMapSprite` | `GlowImageMapRipple` | Rippling glow effect |
| `FXShadowBlobSprite` | `ShadowBlob` | Soft unit shadow |
| `FXShockwaveSprite` | `Shockwave` | Impact shockwave |
| `FXTimerSprite` | `Timer` | Circular progress indicator |
| `FXVortexSprite` | `Vortex` | Whirlpool distortion |
| `FXWhiteCloudVignette` | `WhiteCloudVignette` | Cloud vignette overlay |

### Non-Shader FX Classes
These FX sprites use standard rendering without custom shaders:
- `FXBoidSprite` - Flock particle
- `FXChainSprite` - Chain/lightning connector
- `FXDecalSprite` - Board decal
- `FXEnergyBeamSprite` - Beam projectile
- `FXFlockSprite` - Flock controller
- `FXProjectileSprite` - Projectile base
- `FXRaySprite` / `FXRay001-003Sprite` - Light ray particles
- `FXSprite` - Base class

### Shader Integration Pattern
```javascript
// Example: FXDistortionSprite.js
const FXDistortionSprite = FXSprite.extend({
  shaderKey: 'Distortion',  // Registered shader program

  ctor(options) {
    this._super(options);
    // Get shader uniform locations
    const program = cc.shaderCache.programForKey(this.shaderKey);
    this.loc_resolution = program.getUniformLocation('u_resolution');
    this.loc_refraction = program.getUniformLocation('u_refraction');
    // ...
  },

  updateTweenAction(dt) {
    // Update uniforms each frame
    program.setUniformLocationWith2f(this.loc_resolution, width, height);
    program.setUniformLocationWith1f(this.loc_time, FX.getTime());
  }
});
```

---

## Post-Processing Pipeline

### Bloom Pipeline
```
Surface → [Highpass] → [Blur H] → [Blur V] → [BloomComposite] → Output
                                                    ↑
                                              Previous Frame
```

**HighpassFragment.glsl:**
```glsl
// Extract pixels above threshold
color.rgb = max(((step(u_threshold, color.rgb) * color.rgb)
                 - vec3(u_threshold)) * u_intensity, 0.0);
```

**BloomFragment.glsl (Temporal Accumulation):**
```glsl
float transition = max(0.05, u_transition);  // Min transition rate
float decay = transition * 0.1;               // Decay ratio
float boost = 1.0 + (0.1 / decay / 20.0) * 0.5;

vec4 previousColor = texture2D(u_previousBloom, v_texCoord);
previousColor = max(vec4(0.0), previousColor - vec4(decay));

gl_FragColor = mix(previousColor, color * boost, transition);
```

### Blur Generation
Dynamic blur shaders are generated at runtime:

```coffeescript
# app/shaders/helpers/blurShaderGenerator.coffee
class BlurShaderGenerator
  generate: (samples, radius) ->
    # Generates optimized Gaussian blur with variable samples
    # Returns fragment shader source
```

Blur presets:
- `BlurWeak` - 5 samples, small radius
- `BlurMedium` - 9 samples, medium radius
- `BlurStrong` - 13 samples, large radius
- `BlurExtreme` - 17 samples, maximum radius

---

## Uniform Reference

### Global Uniforms (set by FX system)

| Uniform | Type | Source | Description |
|---------|------|--------|-------------|
| `u_time` | float | `FX.getTime()` | Monotonic time (starts at 1000.0) |
| `u_resolution` | vec2 | `cc.winSize` | Screen dimensions |
| `u_depthMap` | sampler2D | `passes.depth` | Packed depth buffer |
| `u_refractMap` | sampler2D | `passes.surfaceA/B` | Surface for refraction |

### Bloom Uniforms
| Uniform | Default | Range | Effect |
|---------|---------|-------|--------|
| `u_threshold` | 0.6 | 0-1 | Brightness threshold for bloom |
| `u_intensity` | 2.5 | 1-∞ | Bloom brightness multiplier |
| `u_transition` | 0.15 | 0-1 | Temporal blend rate |

### Lighting Uniforms
| Uniform | Default | Range | Effect |
|---------|---------|-------|--------|
| `u_falloffModifier` | 1.0 | 0-∞ | Light spread distance |
| `u_intensityModifier` | 1.0 | 0-∞ | Light brightness |
| `u_depthRange` | winSize.height | - | Depth buffer range |

### Shadow Uniforms
| Uniform | Default | Range | Effect |
|---------|---------|-------|--------|
| `u_intensity` | 0.15 | 0-∞ | Shadow darkness |
| `u_blurShiftModifier` | 1.0 | 1-∞ | Blur start distance |
| `u_blurIntensityModifier` | 3.0 | 0-∞ | Blur strength |

### Distortion Uniforms
| Uniform | Default | Range | Effect |
|---------|---------|-------|--------|
| `u_refraction` | varies | 0-1 | Refraction strength |
| `u_reflection` | varies | 0-1 | Reflection strength |
| `u_fresnelBias` | varies | 0-1 | Edge reflection bias |
| `u_amplitude` | varies | 0-∞ | Distortion intensity |

### Dissolve Uniforms
| Uniform | Default | Range | Effect |
|---------|---------|-------|--------|
| `u_seed` | 0.5 | 0-1 | Noise pattern seed |
| `u_frequency` | 15.0 | 0-∞ | Noise scale (higher = smaller) |
| `u_amplitude` | 0.5 | 0-1 | Noise strength |
| `u_vignetteStrength` | 1.0 | 0-1 | Center preservation |
| `u_edgeFalloff` | 0.25 | 0-1 | Burn edge width |

### Phase-Based Animation
Many effects use `u_phase` (0.0 to 1.0) for animation progression:
- `0.0` = Effect start
- `0.5` = Effect midpoint
- `1.0` = Effect complete

---

## Shader Dependency Graph

```
Noise2D ←─── NoiseRotatedFBM
   ↓
Noise3D ←─┬─ NoiseAbsFBM ←── NoiseAbsFBMOctaves1-6
          ├─ FbmNoiseGradientMask
          ├─ FbmNoiseRays
          ├─ FbmPolarFlare*
          ├─ Fire* (Ring, Wave, Rift)
          ├─ GlowImageMap*
          ├─ HorizontalGlowFlare
          ├─ LensNoise
          ├─ RarityFlare
          ├─ ShadowBlob
          └─ WhiteCloudVignette

NoiseFBM ←─┬─ Dissolve
           ├─ GlowNoise
           ├─ RiftLine
           ├─ Vortex
           └─ Water

Fresnel ──┬── Distortion ←─┬─ DistortionFromNormalMap
Refraction┘               │
Reflection ───────────────┘
                          ↓
                    Distortion* (Shockwave, Vortex, Water)

DepthPack ←─── Depth
DepthUnpack ←─┬─ DepthTest ←─┬─ Distortion
              │               ├─ Shockwave
              │               ├─ Vortex
              │               └─ Water
              └─ DepthDebug
```

---

## Usage Statistics

| Category | Count | Examples |
|----------|-------|----------|
| Fragment Shaders | 45 | Dissolve, Fire, Glow, Distortion |
| Vertex Shaders | 15 | Glow, Highlight, Lighting, Shadow |
| Helper Modules | 25 | Noise, Depth, Fresnel, Levels |
| FX Classes Using Shaders | 26 | FXFireRingSprite, FXDistortionSprite |
| Render Passes | 14 | Bloom, Depth, Surface, etc. |
| Shader Programs | ~35 | Registered in cc.shaderCache |

---

## Performance Considerations

1. **Noise Complexity:** Use octave variants (1-6) to balance quality vs. performance
2. **Depth Testing:** Required for proper distortion compositing, adds overhead
3. **Bloom Scale:** `bloomScale` (default 0.5) reduces bloom pass resolution
4. **Shadow Quality:** `ShadowLowQuality` vs `ShadowHighQuality` trade blur samples
5. **Temporal Effects:** Bloom uses frame-to-frame accumulation for smooth transitions
6. **glslify:** Bundles helpers at build time, no runtime include overhead

---

*Generated from Duelyst codebase analysis. Source files in `app/shaders/`.*
