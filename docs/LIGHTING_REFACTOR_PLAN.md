# Lighting System Surgical Replacement

## Context

The codebase has a partially-implemented Duelyst lighting port where ~60% is dead code. SDF shadows work correctly and must be preserved. Goal: remove dead infrastructure and add dynamic sun positioning.

## Current State (Verified)

### Live Render Path
```
main.cpp:render_multi_pass()
  → entity.cpp:render_shadow()
    → gpu_renderer.cpp:draw_sdf_shadow()
      → binds sdf_shadow_pipeline
      → pushes SDFShadowUniforms
```

### Live Shaders (8)
- `sprite.vert`, `sprite.frag` - Sprite rendering
- `sdf_shadow.vert`, `sdf_shadow.frag` - SDF shadow rendering
- `color.vert`, `color.frag` - Grid/UI quads
- `dissolve.frag` - Death effect
- `fullscreen.vert` - Post-process base (keep for future)

### Dead Shaders (16)
- `shadow.vert`, `shadow.frag` - Legacy box blur
- `shadow_perpass.frag` - Per-sprite FBO path
- `lighting.vert`, `lighting.frag` - Dynamic lighting (never called)
- `lit_sprite.frag` - Light accumulation (never called)
- `highpass.frag`, `blur.frag`, `bloom.frag` - Bloom pipeline
- `surface_composite.frag` - Tone mapping
- `vignette.frag` - Corner darkening
- `radial_blur.frag`, `tone_curve.frag` - Already orphaned
- `copy.frag` - Already orphaned
- `depth.vert`, `depth.frag` - Never loaded

### Critical Architecture Insight

The SDF shadow shader does NOT receive light direction. All shadow geometry (skew, flip, stretch) is computed on the CPU in `draw_sdf_shadow()` and baked into quad vertices. The shader only receives:
- Pre-transformed quad vertices
- `u_lightIntensity` for distance-based fade
- `u_lightDir` (declared but unused)

**Interface contract:**
```cpp
// This is ALL that drives shadow direction
PointLight scene_light;
scene_light.x = <screen X pixels>;
scene_light.y = <screen Y pixels>;
scene_light.radius = 95000.0f;  // Far = parallel rays
scene_light.intensity = 1.0f;
scene_light.a = 1.0f;           // Opacity
g_gpu.set_scene_light(scene_light);
```

---

## Phase 1: Sun Position Calculator

Add time-of-day to screen-space sun position conversion.

```cpp
// In main.cpp or new sun.hpp
Vec2 sun_position_from_time(float hour, int window_w, int window_h) {
    // hour: 0-24, where 6=sunrise, 12=noon, 18=sunset
    float angle = ((hour - 6.0f) / 12.0f) * M_PI;  // 0 at 6am, PI at 6pm
    float radius = window_h * 3.5f;  // Far enough for parallel rays
    return {
        window_w * 0.5f + std::cos(angle) * radius,
        window_h * 0.5f - std::sin(angle) * radius
    };
}
```

## Phase 2: Dead Code Excision

### Shaders to Delete
```
shaders/shadow.vert
shaders/shadow.frag
shaders/shadow_perpass.frag
shaders/lighting.vert
shaders/lighting.frag
shaders/lit_sprite.frag
shaders/highpass.frag
shaders/blur.frag
shaders/bloom.frag
shaders/surface_composite.frag
shaders/vignette.frag
shaders/radial_blur.frag
shaders/tone_curve.frag
shaders/copy.frag
shaders/depth.vert
shaders/depth.frag
```

### Pipelines to Remove (gpu_renderer.cpp)
- `shadow_pipeline` - lines 335-386
- `shadow_perpass_pipeline` - lines 603-645
- `highpass_pipeline` - lines 502-513
- `blur_pipeline` - lines 515-524
- `bloom_pipeline` - lines 526-535
- `composite_pipeline` - lines 537-547
- `copy_pipeline` - lines 491-500
- `lit_sprite_pipeline` - never created, remove declaration
- `lighting_pipeline` - never created, remove declaration
- `radial_blur_pipeline` - lines 699-709
- `tone_curve_pipeline` - lines 711-718
- `vignette_pipeline` - lines 720-727
- `sprite_offscreen_pipeline` - lines 551-601

### Functions to Remove (gpu_renderer.cpp)
- `draw_sprite_shadow()` - lines 1093-1286
- `draw_shadow_from_pass()` - lines 1797-2058
- `draw_sprite_to_pass()` - used only by draw_shadow_from_pass
- `execute_bloom_pass()` - lines 1576-1670
- `composite_to_screen()` - if bloom removed
- `apply_radial_blur()` - lines 2391-2416
- `apply_tone_curve()` - lines 2418-2446
- `apply_vignette()` - lines 2448-2474
- `execute_post_processing()` - stub or remove
- `blit_pass()` - lines 1560-1574
- `draw_lit_sprite()` - lines 2301-2330
- `accumulate_sprite_light()` - lines 2332-2355
- `add_dynamic_light()`, `remove_dynamic_light()`, `clear_dynamic_lights()`

### Classes to Remove
- `LightingManager` (include/lighting.hpp, src/lighting.cpp if exists)

### FXConfig Fields to Remove
```cpp
// Remove these from FXConfig:
float ambient_r, ambient_g, ambient_b;  // Never used
float falloff_modifier, intensity_modifier;  // Never used
float shadow_blur_shift, shadow_blur_intensity;  // Legacy shadow only
float bloom_threshold, bloom_intensity, bloom_transition, bloom_scale;
float radial_blur_strength, radial_blur_center_x/y, radial_blur_samples;
float tone_intensity, tone_contrast, tone_brightness, tone_saturation;
float vignette_intensity, vignette_radius, vignette_softness;
bool enable_bloom, enable_lighting, enable_vignette, enable_tone_curve, enable_radial_blur;
ShadowType shadow_type;  // Hardcode to SDF
```

### FXConfig Fields to Keep
```cpp
struct FXConfig {
    float shadow_intensity = 0.35f;
    float sdf_penumbra_scale = 0.25f;
    float sdf_max_raymarch = 0.3f;      // Unused but in shader struct
    float sdf_raymarch_steps = 12.0f;   // Unused but in shader struct
    bool enable_shadows = true;
    bool use_multipass = true;  // May still be needed for render structure
    float exposure = 1.0f;   // Keep for future
    float gamma = 2.2f;      // Keep for future
};
```

## Phase 3: Simplify Presets

Replace raw position multipliers with time-of-day:

```cpp
struct LightingPreset {
    const char* name;
    float time_of_day;        // 0-24 hours
    float shadow_intensity;   // 0-1
    // Remove: light_x_mult, light_y_mult, use_height_for_x, light_opacity
    // Remove: ambient_r/g/b, bloom_threshold, bloom_intensity
};

static const LightingPreset g_lighting_presets[] = {
    {"Dawn",        6.0f,  0.60f},
    {"Morning",     9.0f,  0.85f},
    {"Noon",       12.0f,  1.00f},
    {"Afternoon",  15.0f,  0.90f},
    {"Dusk",       18.0f,  0.70f},
    {"Night",      22.0f,  0.40f},
};
```

## Phase 4: Integration

```cpp
void apply_lighting_preset(int preset_idx, const RenderConfig& config) {
    const auto& preset = g_lighting_presets[preset_idx];

    Vec2 sun_pos = sun_position_from_time(preset.time_of_day,
                                           config.window_w, config.window_h);

    PointLight sun;
    sun.x = sun_pos.x;
    sun.y = sun_pos.y;
    sun.radius = 95000.0f;
    sun.intensity = 1.0f;
    sun.a = 1.0f;
    g_gpu.set_scene_light(sun);

    g_gpu.fx_config.shadow_intensity = preset.shadow_intensity;
}
```

---

## Execution Order

1. **Phase 1**: Add sun calculator (additive, no breakage)
2. **Phase 2**: Delete dead code (incremental, test after each deletion)
3. **Phase 3**: Simplify presets
4. **Phase 4**: Wire together

## Risk Mitigation

- Delete shaders first (safest - just removes files)
- Remove pipeline creation next (will cause null checks to skip)
- Remove functions last (may have subtle callers)
- Keep `use_multipass` flag initially - render loop structure depends on it
- Test build after each major deletion
