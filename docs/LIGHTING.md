# Lighting Pipeline

Sylvanshine's lighting system creates the perceptual impression of a sun casting light from different positions. The system derives from Duelyst's rendering pipeline but currently only uses a subset of the infrastructure.

## Table of Contents

1. [What's Actually Used](#whats-actually-used)
2. [Orphaned Infrastructure](#orphaned-infrastructure)
3. [Lighting Presets](#lighting-presets)
4. [Active Preset Parameters](#active-preset-parameters)
5. [Shadow System](#shadow-system)
6. [Post-Processing Pipeline](#post-processing-pipeline)
7. [Shader Architecture](#shader-architecture)
8. [Code Locations](#code-locations)

---

## What's Actually Used

During gameplay, the following systems are **active**:

| System | Status | Purpose |
|--------|--------|---------|
| Scene Light (PointLight) | **ACTIVE** | Controls shadow direction and skew |
| SDF Shadow Rendering | **ACTIVE** | Renders unit shadows |
| Bloom Pipeline | **ACTIVE** | Glow effect on bright areas |
| Vignette | **ACTIVE** | Corner darkening |
| Surface Composite | **ACTIVE** | Tone mapping + gamma correction |

### What Preset Parameters Actually Affect

| Parameter | Effect |
|-----------|--------|
| `light_x_mult`, `light_y_mult` | Shadow direction (left/right/up/down) |
| `use_height_for_x` | Shadow angle calculation mode |
| `shadow_intensity` | Shadow darkness |
| `bloom_threshold` | What brightness triggers bloom |
| `bloom_intensity` | How strong the bloom glow is |
| `light_opacity` | Multiplied into shadow attenuation |

### What Preset Parameters Are IGNORED

| Parameter | Why Unused |
|-----------|------------|
| `ambient_r`, `ambient_g`, `ambient_b` | Only used by dynamic lighting system, which has no lights |

---

## Orphaned Infrastructure

The following systems exist but are **never executed**:

| System | Why Orphaned |
|--------|--------------|
| Dynamic Lighting (`lighting.lights`) | `add_dynamic_light()` never called |
| `draw_lit_sprite()` | Falls back to `draw_sprite()` since lights empty |
| `lighting.frag` shader | No lights to render |
| `lit_sprite.frag` shader | Never used |
| Tone Curve | `enable_tone_curve = false`, never enabled |
| Radial Blur | `enable_radial_blur = false`, never triggered |
| Per-pixel light accumulation | Comment says "TODO" in `draw_lit_sprite()` |

### The Lighting Deception

```cpp
// In draw_lit_sprite() - gpu_renderer.cpp:2308
if (!fx_config.enable_lighting || lighting.lights.empty()) {
    // No dynamic lighting, use regular sprite draw
    draw_sprite(texture, src, dst, flip_x, opacity);  // ← ALWAYS takes this path
    return;
}
```

Despite `enable_lighting = true` by default, `lighting.lights` is always empty because nothing calls `add_dynamic_light()`. Sprites render with `draw_sprite()`, not `draw_lit_sprite()`.

---

## Lighting Presets

There are **10 lighting presets** (indices 0-9), selectable at runtime via number keys. Each preset simulates a different battlefield environment with distinct sun positions and atmospheric properties.

| Index | Name | Sun Direction | Character |
|-------|------|---------------|-----------|
| 0 | Sylvanshine Default | Upper-left (close) | Point light for testing |
| 1 | Lyonar Highlands | Upper-right | Bright, high contrast |
| 2 | Songhai Temple | Right | Eastern sunrise feel |
| 3 | Vetruvian Dunes | Right | Desert, high shadow intensity |
| 4 | Abyssian Depths | Left | Dark, ominous |
| 5 | Magmar Peaks | Left | Volcanic, lower shadow |
| 6 | Vanar Wastes | Left | Icy, lowest shadow |
| 7 | Shimzar Jungle | Upper-left | Dim canopy light |
| 8 | Shadow Creep | Left-low | Low altitude, dim |
| 9 | Redrock Canyon | Right | Warm ambient tint |

---

## Active Preset Parameters

Each `LightingPreset` struct contains 10 parameters, but only **7 are used**:

```cpp
struct LightingPreset {
    const char* name;
    float light_x_mult;      // ✓ USED - Shadow direction
    float light_y_mult;      // ✓ USED - Shadow direction
    bool use_height_for_x;   // ✓ USED - Shadow angle calculation
    float light_opacity;     // ✓ USED - Shadow attenuation
    float shadow_intensity;  // ✓ USED - Shadow darkness
    float ambient_r, ambient_g, ambient_b;  // ✗ UNUSED - No dynamic lights
    float bloom_threshold;   // ✓ USED - Bloom extraction
    float bloom_intensity;   // ✓ USED - Bloom strength
};
```

### Detailed Preset Values

Columns affecting gameplay are **bold**. "Ambient RGB" is grayed out as unused.

| Preset | **X Mult** | **Y Mult** | **Height-X** | **Opacity** | **Shadow** | ~~Ambient RGB~~ | **Bloom Thresh** | **Bloom Int** |
|--------|--------|--------|----------|---------|--------|-------------|--------------|-----------|
| 0: Default | 0.30 | 0.20 | No | 1.00 | 0.60 | ~~(89, 89, 89)~~ | 0.60 | 2.50 |
| 1: Lyonar | 3.25 | -3.15 | No | 1.00 | 1.00 | ~~(94, 94, 94)~~ | 0.50 | 2.76 |
| 2: Songhai | 3.25 | 3.75 | Yes | 1.00 | 1.00 | ~~(89, 89, 89)~~ | 0.50 | 2.76 |
| 3: Vetruvian | 3.25 | 3.75 | Yes | 1.00 | 1.10 | ~~(89, 89, 89)~~ | 0.50 | 2.55 |
| 4: Abyssian | -3.25 | 3.75 | Yes | 1.00 | 1.00 | ~~(89, 89, 89)~~ | 0.52 | 2.74 |
| 5: Magmar | -3.25 | 3.75 | Yes | 1.00 | 0.85 | ~~(94, 94, 94)~~ | 0.50 | 2.76 |
| 6: Vanar | -3.25 | 3.75 | Yes | 1.00 | 0.75 | ~~(89, 89, 89)~~ | 0.50 | 2.50 |
| 7: Shimzar | -1.25 | 3.15 | No | 0.78 | 1.00 | ~~(94, 94, 94)~~ | 0.50 | 2.76 |
| 8: Shadow | -3.25 | 1.50 | No | 0.78 | 1.00 | ~~(94, 94, 94)~~ | 0.50 | 2.76 |
| 9: Redrock | 2.95 | 3.15 | No | 0.78 | 0.80 | ~~(110, 94, 94)~~ | 0.50 | 2.60 |

### Light Position Calculation

For preset 0 (Sylvanshine Default):
```cpp
light.x = window_w * light_x_mult;
light.y = window_h * light_y_mult;
light.radius = window_w * 0.8f;  // Close point light
```

For presets 1-9 (Duelyst-style):
```cpp
float base_x = use_height_for_x ? window_h : window_w;
light.x = window_w * 0.5f + base_x * light_x_mult;
light.y = window_h * 0.5f - window_h * light_y_mult;
light.radius = 95000.0f;  // TILESIZE * 1000 = sun-like distance
```

The enormous radius (95000 pixels) simulates parallel sun rays - all sprites on the battlefield receive nearly identical light direction.

---

## Shadow System

The scene light position controls shadow projection. This is the **primary use** of the lighting presets.

### Shadow Direction

```cpp
// In draw_sdf_shadow() - gpu_renderer.cpp:2122
float depth_diff = active_light->y - feet_pos.y;
bool shadow_below = (depth_diff - SHADOW_OFFSET) >= 0.0f;
float shadow_dir = shadow_below ? 1.0f : -1.0f;
```

- **Light above sprite** (depth_diff >= 19.5): Shadow projects **downward**
- **Light below sprite** (depth_diff < 19.5): Shadow projects **upward**

### Shadow Skew

```cpp
// Horizontal skew based on light direction
if (std::abs(light_dir_y) > 0.001f) {
    skew = -light_dir_x / std::abs(light_dir_y) * 0.5f;
}
```

- **Light to the right**: Shadows skew left
- **Light to the left**: Shadows skew right
- **Light overhead**: Shadows cast straight (minimal skew)

### Shadow Intensity Attenuation

```cpp
// Quadratic falloff from light source
float lightDistPct = pow(light_dist / active_light->radius, 2.0f);
float lightDistPctInv = max(0.0f, 1.0f - lightDistPct);
lightDistPctInv *= active_light->a * active_light->intensity;
```

The final shadow fragment uses:
```glsl
fragColor = vec4(0.0, 0.0, 0.0, u_lightDistPctInv * alpha * intensity * u_opacity);
```

### Shadow Penumbra (Progressive Blur)

The legacy shadow shader (`shadow.frag`) implements Duelyst's penumbra:

```glsl
// Blur increases with distance from feet (anchor)
float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);
float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);

// 7x7 box blur with distance-based kernel size
float blurX = (uvSpan.x / u_size.x * u_blurIntensityModifier) * occluderDistPctBlurModifier;
```

- **At feet**: blur = 0 (sharp edge, contact hardening)
- **Away from feet**: blur increases (soft penumbra)

---

## Post-Processing Pipeline

### What Actually Runs

```
render_multi_pass()
     │
     ├─► Grid/Background → Surface FBO
     ├─► Shadows → Surface FBO (if enable_shadows)
     │
     ├─► end_surface_pass()
     │
     ├─► Bloom Pipeline (if enable_bloom):        ← ACTIVE
     │     1. Highpass (extract bright pixels)
     │     2. Horizontal blur
     │     3. Vertical blur
     │     4. Temporal accumulation
     │
     ├─► execute_post_processing():
     │     - Radial blur (if strength > 0)        ← NEVER (strength = 0)
     │     - Tone curve (if enable_tone_curve)    ← NEVER (disabled)
     │     - Vignette (if enable_vignette)        ← ACTIVE
     │
     ├─► composite_to_screen():                   ← ACTIVE
     │     - Add bloom (additive)
     │     - Reinhard tone mapping
     │     - Gamma correction
     │
     └─► Sprites → Swapchain directly (bypass all post-processing)
```

**Key insight**: Sprites render AFTER composite_to_screen(), directly to swapchain. They bypass bloom, vignette, and tone mapping entirely. Only shadows go through the post-processing chain.

### Bloom Parameters

```cpp
float bloom_threshold = 0.6f;   // Luminance cutoff
float bloom_intensity = 2.5f;   // Brightness multiplier
float bloom_transition = 0.15f; // Temporal smoothing
float bloom_scale = 0.5f;       // Resolution scale
```

### Vignette Parameters

```cpp
float vignette_intensity = 0.3f;  // Corner darkness
float vignette_radius = 0.7f;     // Effect start distance
float vignette_softness = 0.4f;   // Falloff gradient
```

### FXConfig (Complete Settings)

Located in `include/gpu_renderer.hpp:169`:

```cpp
struct FXConfig {
    // Ambient light
    float ambient_r, ambient_g, ambient_b;

    // Light modifiers
    float falloff_modifier = 1.0f;
    float intensity_modifier = 1.0f;

    // Shadow settings
    float shadow_intensity = 0.35f;
    float shadow_blur_shift = 1.0f;
    float shadow_blur_intensity = 3.0f;

    // Bloom
    float bloom_threshold = 0.6f;
    float bloom_intensity = 2.5f;
    float bloom_transition = 0.15f;
    float bloom_scale = 0.5f;

    // Post-processing
    float exposure = 1.0f;
    float gamma = 2.2f;

    // Radial blur
    float radial_blur_strength = 0.0f;
    float radial_blur_center_x = 0.5f;
    float radial_blur_center_y = 0.5f;
    float radial_blur_samples = 8.0f;

    // Tone curve
    float tone_intensity = 1.0f;
    float tone_contrast = 1.0f;
    float tone_brightness = 0.0f;
    float tone_saturation = 1.0f;

    // Vignette
    float vignette_intensity = 0.3f;
    float vignette_radius = 0.7f;
    float vignette_softness = 0.4f;

    // Feature toggles
    bool enable_bloom = true;
    bool enable_lighting = true;
    bool enable_shadows = true;
    bool enable_vignette = true;
    bool enable_tone_curve = false;
    bool enable_radial_blur = false;

    // Shadow type
    ShadowType shadow_type = ShadowType::SDF;

    // SDF shadow
    float sdf_penumbra_scale = 0.25f;
    float sdf_max_raymarch = 0.3f;
    float sdf_raymarch_steps = 12.0f;
};
```

---

## Shader Architecture

### Vertex Shaders

| Shader | Status | Purpose |
|--------|--------|---------|
| `sprite.vert` | **ACTIVE** | Standard sprite rendering |
| `sdf_shadow.vert` | **ACTIVE** | SDF shadow projection |
| `fullscreen.vert` | **ACTIVE** | Post-processing passes |
| `shadow.vert` | ORPHANED | Legacy box blur shadow (SDF used instead) |
| `lighting.vert` | ORPHANED | Per-light accumulation (no lights) |

### Fragment Shaders

| Shader | Status | Purpose |
|--------|--------|---------|
| `sprite.frag` | **ACTIVE** | Basic sprite with opacity |
| `dissolve.frag` | **ACTIVE** | Death dissolve effect |
| `sdf_shadow.frag` | **ACTIVE** | SDF-based soft shadow |
| `highpass.frag` | **ACTIVE** | Bloom extraction |
| `blur.frag` | **ACTIVE** | Gaussian blur (9-tap) |
| `bloom.frag` | **ACTIVE** | Temporal accumulation |
| `surface_composite.frag` | **ACTIVE** | Final composite with tone mapping |
| `vignette.frag` | **ACTIVE** | Corner darkening |
| `shadow.frag` | ORPHANED | Legacy box blur (SDF used instead) |
| `lighting.frag` | ORPHANED | Light contribution (no lights added) |
| `lit_sprite.frag` | ORPHANED | Sprite + light map (never used) |
| `tone_curve.frag` | ORPHANED | Color grading (disabled) |
| `radial_blur.frag` | ORPHANED | Zoom effect (never triggered) |

---

## Code Locations

### Active Code

| Component | File | Line |
|-----------|------|------|
| Preset definitions | `src/main.cpp` | 99-129 |
| apply_lighting_preset() | `src/main.cpp` | 133-177 |
| render_multi_pass() | `src/main.cpp` | 1086-1152 |
| SDF shadow geometry | `src/gpu_renderer.cpp` | 2061-2200 |
| execute_bloom_pass() | `src/gpu_renderer.cpp` | 1576 |
| composite_to_screen() | `src/gpu_renderer.cpp` | (composite logic) |
| FXConfig struct | `include/gpu_renderer.hpp` | 169-227 |

### Orphaned Code

| Component | File | Why Orphaned |
|-----------|------|--------------|
| LightingManager | `include/lighting.hpp` | lights vector never populated |
| draw_lit_sprite() | `src/gpu_renderer.cpp:2301` | Falls back to draw_sprite |
| accumulate_sprite_light() | `src/gpu_renderer.cpp:2332` | Never called |
| apply_tone_curve() | `src/gpu_renderer.cpp:2418` | enable_tone_curve = false |
| apply_radial_blur() | `src/gpu_renderer.cpp:2391` | strength = 0 |

---

## Runtime Controls

| Key | Action |
|-----|--------|
| 0-9 | Switch lighting preset |
| L | Toggle lighting (has no effect - lights empty) |

Preset changes are logged to console:
```
=== Lighting Preset 1: Lyonar Highlands ===
  Light pos: (2756, -1685), radius: 95000
  Shadow intensity: 1.00
  Ambient: (94, 94, 94)    ← Logged but not used
```

---

## Summary: The Effective Pipeline

What **actually happens** each frame:

1. **Shadows** render to surface FBO using scene light direction
2. **Bloom** extracts bright pixels, blurs, accumulates
3. **Vignette** darkens corners
4. **Composite** applies tone mapping + gamma to shadows
5. **Sprites** render directly to swapchain (no lighting applied)

The "sun position" from presets only affects **shadow direction**. Sprites themselves are rendered flat with no lighting modulation.
