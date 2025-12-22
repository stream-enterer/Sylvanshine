# Lighting System

Sylvanshine uses a time-of-day based sun positioning system that controls SDF shadow projection. The system was simplified from Duelyst's multi-pass rendering pipeline to a clean single-pass architecture.

## Architecture Overview

```
render_single_pass()
    │
    ├─► Clear swapchain
    ├─► Draw grid/background
    ├─► Draw SDF shadows (for each entity)
    ├─► Draw sprites (for each entity)
    └─► Present
```

All rendering goes directly to the swapchain. There are no intermediate framebuffers, no post-processing effects, and no multi-pass compositing.

## Sun Position Calculator

The sun position is computed from a time-of-day value (0-24 hours):

```cpp
Vec2 sun_position_from_time(float hour, int window_w, int window_h) {
    // Normalize hour to 0-24 range
    while (hour < 0.0f) hour += 24.0f;
    while (hour >= 24.0f) hour -= 24.0f;

    // Sun arc: 6am = horizon left, 12pm = zenith, 6pm = horizon right
    float angle = ((hour - 6.0f) / 12.0f) * M_PI;

    // Large orbit radius for parallel rays (sun-like)
    float orbit_radius = window_h * 3.5f;

    return {
        window_w * 0.5f + std::cos(angle) * orbit_radius,
        window_h * 0.5f - std::sin(angle) * orbit_radius
    };
}
```

The large orbit radius (3.5× window height) ensures nearly parallel light rays across the battlefield, simulating distant sunlight.

## Lighting Presets

Presets are selected via number keys 0-9:

| Key | Name | Time | Shadow Intensity |
|-----|------|------|------------------|
| 0 | Dawn | 6:30 | 0.50 |
| 1 | Morning | 9:00 | 0.75 |
| 2 | Noon | 12:00 | 1.00 |
| 3 | Afternoon | 15:00 | 0.90 |
| 4 | Dusk | 18:00 | 0.60 |
| 5 | Night | 21:00 | 0.30 |
| 6 | Midnight | 0:00 | 0.20 |
| 7 | Pre-dawn | 4:00 | 0.35 |
| 8 | Late Morning | 10:30 | 0.85 |
| 9 | Golden Hour | 17:00 | 0.70 |

### Preset Definition

```cpp
struct LightingPreset {
    const char* name;
    float time_of_day;        // 0-24 hours
    float shadow_intensity;   // Shadow darkness multiplier
};
```

### Applying a Preset

```cpp
void apply_lighting_preset(int idx, const RenderConfig& config) {
    const auto& preset = g_lighting_presets[idx];

    Vec2 sun_pos = sun_position_from_time(
        preset.time_of_day, config.window_w, config.window_h);

    PointLight sun;
    sun.x = sun_pos.x;
    sun.y = sun_pos.y;
    sun.radius = 95000.0f;  // Far enough for parallel rays
    sun.intensity = 1.0f;
    sun.a = 1.0f;

    g_gpu.set_scene_light(sun);
    g_gpu.fx_config.shadow_intensity = preset.shadow_intensity;
}
```

## Shadow System

Shadows use SDF (Signed Distance Field) rendering with CPU-computed geometry.

### Shadow Geometry (CPU)

The `draw_sdf_shadow()` function computes shadow quad vertices based on light position:

1. **Direction**: Light above sprite → shadow below; light below → shadow above
2. **Skew**: Horizontal offset based on light's X position relative to sprite
3. **Stretch**: Shadow lengthens when sun is low on horizon

```cpp
// Determine shadow direction
float depth_diff = light.y - feet_pos.y;
bool shadow_below = (depth_diff - SHADOW_OFFSET) >= 0.0f;
float shadow_dir = shadow_below ? 1.0f : -1.0f;

// Calculate skew from light direction
float skew = -light_dir_x / std::abs(light_dir_y) * 0.5f;

// Stretch based on light altitude
float altitude = std::abs(light_dir_y);
float stretch = std::min(1.0f / std::sqrt(altitude), 1.6f);
```

### Shadow Rendering (GPU)

The SDF shadow shader (`sdf_shadow.frag`) renders soft shadows using the pre-computed SDF atlas:

- Samples the SDF texture to determine distance from sprite silhouette
- Applies penumbra softness based on distance from anchor point
- Uses light intensity for opacity falloff

### Shadow Parameters

```cpp
struct FXConfig {
    float shadow_intensity = 0.35f;      // Base shadow darkness
    float sdf_penumbra_scale = 0.25f;    // Penumbra softness
    float sdf_max_raymarch = 0.3f;       // (shader compat)
    float sdf_raymarch_steps = 12.0f;    // (shader compat)
    bool enable_shadows = true;
};
```

## Active Shaders

| Shader | Purpose |
|--------|---------|
| `sprite.vert` | Standard sprite vertex transform |
| `sprite.frag` | Sprite rendering with opacity |
| `dissolve.frag` | Death/spawn dissolve effect |
| `sdf_shadow.vert` | Shadow quad vertex transform |
| `sdf_shadow.frag` | SDF-based soft shadow |
| `color.vert` | Grid/UI vertex transform |
| `color.frag` | Solid color rendering |

## Code Locations

| Component | File | Description |
|-----------|------|-------------|
| `sun_position_from_time()` | `src/main.cpp` | Time → screen position |
| `LightingPreset` | `src/main.cpp` | Preset definitions |
| `apply_lighting_preset()` | `src/main.cpp` | Apply preset to renderer |
| `draw_sdf_shadow()` | `src/gpu_renderer.cpp` | Shadow geometry + rendering |
| `FXConfig` | `include/gpu_renderer.hpp` | Shadow configuration |
| `PointLight` | `include/gpu_renderer.hpp` | Light source struct |

## Runtime Controls

| Key | Action |
|-----|--------|
| 0-9 | Switch lighting preset |

Preset changes are logged:
```
=== Lighting Preset: Morning ===
  Time: 9.0h, Sun pos: (1847, -2156)
  Shadow intensity: 0.75
```

## Design Rationale

The simplified system was created by removing unused Duelyst infrastructure:

**Removed:**
- Multi-pass rendering (bloom, vignette, tone mapping)
- Dynamic lighting system (per-sprite light accumulation)
- Legacy box-blur shadows
- 16 orphaned shader files

**Kept:**
- SDF shadow rendering (high quality, single pass)
- Time-based sun positioning (intuitive, clean API)
- Scene light for shadow direction control

The result is a ~940 line reduction in `gpu_renderer.cpp` (44% smaller) while maintaining visual quality for the shadow system.
