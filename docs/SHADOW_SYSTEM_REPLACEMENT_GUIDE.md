# Shadow System Replacement Guide

Complete reference for replacing the Sylvanshine shadow system. Contains all integration points, data structures, and rendering flow.

## System Overview

The shadow system renders dynamic silhouette shadows for entities. Shadows are:
- Skewed/stretched based on light position
- Progressively blurred (sharp at feet, soft at head)
- Attenuated by distance from light source

## Rendering Flow

### Per-Frame Render Loop (main.cpp)

```
1. g_gpu.begin_frame()
2. g_gpu.begin_surface_pass()

3. For each entity (sorted by Y for depth):
   a. entity.render_shadow(config)  ← SHADOW PASS
   b. entity.render(config)         ← SPRITE PASS

4. g_gpu.end_surface_pass()
5. g_gpu.execute_bloom_pass()
6. g_gpu.composite_to_screen()
7. g_gpu.end_frame()
```

### Shadow Render Path (entity.cpp → gpu_renderer.cpp)

```cpp
// Entity::render_shadow() calls:
1. draw_sprite_to_pass(texture, src, sprite_w, sprite_h)
   → Renders sprite to per-sprite FBO (RenderPass)
   → UV 0-1 covers entire sprite (not atlas)

2. draw_shadow_from_pass(sprite_pass, sprite_w, sprite_h, feet_pos, scale, flip_x, opacity)
   → Calculates skew/stretch from light position
   → Builds transformed quad vertices
   → Draws to surface pass with shadow shader
```

## Key Data Structures

### PointLight (gpu_renderer.hpp:28-37)
```cpp
struct PointLight {
    float x, y;          // Screen position
    float radius;        // Light radius in pixels (default 285)
    float intensity;     // Light intensity (higher dims lower intensity lights)
    float r, g, b, a;    // Light color
    bool casts_shadows;  // Whether this light affects shadow rendering
};
```

### ShadowVertex (gpu_renderer.hpp:16-20)
```cpp
struct ShadowVertex {
    float x, y;      // NDC position
    float u, v;      // Texture coordinates (0-1 for per-sprite FBO)
    float lx, ly;    // Local sprite position in PIXELS (for blur calculation)
};
```

### ShadowPerPassUniforms (gpu_renderer.hpp:107-118)
```cpp
struct ShadowPerPassUniforms {
    float opacity;                  // External opacity control
    float intensity;                // Shadow darkness (default 0.15)
    float blur_shift_modifier;      // Blur gradient steepness (default 1.0)
    float blur_intensity_modifier;  // Blur amount (default 3.0)
    float size_x, size_y;           // Sprite content size in pixels
    float anchor_x, anchor_y;       // Anchor point (for blur calculation)
    float light_dist_pct_inv;       // Light distance attenuation (1=full, 0=none)
    float render_scale;             // Render scale for scale-independent blur
    float _pad1, _pad2;             // Alignment padding
};
```

### FXConfig (gpu_renderer.hpp:144-194)
Shadow-relevant fields:
```cpp
struct FXConfig {
    float shadow_intensity = 0.15f;      // Shadow darkness
    float shadow_blur_shift = 1.0f;      // Blur gradient
    float shadow_blur_intensity = 3.0f;  // Blur amount
    bool enable_shadows = true;          // Global toggle
    bool use_multipass = true;           // Per-sprite FBO vs atlas sampling
};
```

### SpriteProperties (sprite_properties.hpp:5-35)
```cpp
struct SpriteProperties {
    bool casts_shadows = true;       // Whether this sprite casts shadows
    float shadow_offset = 0.0f;      // Y offset for shadow anchor
    float shadow_intensity = 0.15f;  // Per-sprite shadow darkness override
};
```

## Integration Points

### 1. Entity Integration (entity.hpp/cpp)

Each Entity has:
```cpp
SpriteProperties sprite_props;  // Controls shadow casting
GPUTextureHandle spritesheet;   // Sprite texture atlas
Animation* current_anim;        // Current animation (has frame rects)
Vec2 screen_pos;                // Screen position (feet position)
bool flip_x;                    // Horizontal flip
float opacity;                  // Entity opacity
```

Entity::render_shadow() is the entry point:
```cpp
void Entity::render_shadow(const RenderConfig& config) const {
    if (!sprite_props.casts_shadows) return;

    const AnimFrame& frame = current_anim->frames[frame_idx];
    SDL_FRect src = { frame.rect.x, frame.rect.y, frame.rect.w, frame.rect.h };

    Vec2 shadow_feet_pos = { screen_pos.x, screen_pos.y };
    float shadow_alpha = 0.78f * opacity;  // SHADOW_OPACITY

    // Per-sprite FBO approach
    RenderPass* sprite_pass = g_gpu.draw_sprite_to_pass(
        spritesheet, src, frame.rect.w, frame.rect.h);

    g_gpu.draw_shadow_from_pass(
        sprite_pass, frame.rect.w, frame.rect.h,
        shadow_feet_pos, config.scale, flip_x, shadow_alpha);
}
```

### 2. Lighting Integration (gpu_renderer.hpp/cpp)

The scene light is set via:
```cpp
void GPURenderer::set_scene_light(const PointLight& light);
void GPURenderer::clear_scene_light();
```

Lighting presets are in main.cpp:
```cpp
struct LightingPreset {
    float light_x, light_y;      // Relative to screen center
    float radius;
    float shadow_intensity;
    // ...
};

void apply_lighting_preset(int idx) {
    PointLight light;
    light.x = screen_center_x + preset.light_x * scale;
    light.y = screen_center_y + preset.light_y * scale;
    // ...
    g_gpu.set_scene_light(light);
}
```

### 3. RenderPass Pool (render_pass.hpp)

Per-sprite shadow rendering uses pooled FBOs:
```cpp
RenderPass* PassManager::acquire_sprite_shadow_pass(Uint32 w, Uint32 h);
void PassManager::reset_sprite_pass_pools();  // Called each frame
```

### 4. Pipeline (gpu_renderer.cpp)

The shadow pipeline uses:
- Vertex shader: `shaders/shadow.vert`
- Fragment shader: `shaders/shadow_perpass.frag`
- Pipeline handle: `shadow_perpass_pipeline`

## GPURenderer Shadow API

### draw_sprite_to_pass()
```cpp
RenderPass* GPURenderer::draw_sprite_to_pass(
    const GPUTextureHandle& texture,  // Sprite atlas texture
    const SDL_FRect& src,             // Source rect in atlas
    Uint32 sprite_w, Uint32 sprite_h  // Actual sprite dimensions
);
```
Renders sprite to a per-sprite FBO. Returns the RenderPass for subsequent shadow drawing.

### draw_shadow_from_pass()
```cpp
void GPURenderer::draw_shadow_from_pass(
    RenderPass* sprite_pass,          // From draw_sprite_to_pass()
    Uint32 sprite_w, Uint32 sprite_h, // Actual sprite dimensions
    Vec2 feet_pos,                    // Screen position of sprite feet
    float scale,                      // Render scale (config.scale)
    bool flip_x,                      // Horizontal flip
    float opacity,                    // Shadow opacity (0-1)
    const PointLight* light = nullptr // Optional light override
);
```

### Legacy API (atlas-based, has blur issues)
```cpp
void GPURenderer::draw_sprite_shadow(
    const GPUTextureHandle& texture,
    const SDL_FRect& src,
    Vec2 feet_pos,
    float scale,
    bool flip_x,
    float opacity,
    const PointLight* light = nullptr
);
```

## Shader Interface

### Vertex Shader Input (shadow.vert)
```glsl
layout(location = 0) in vec2 a_position;   // NDC position
layout(location = 1) in vec2 a_texCoord;   // UV coordinates (0-1)
layout(location = 2) in vec2 a_localPos;   // Local position in PIXELS
```

### Fragment Shader (shadow_perpass.frag)

Uniforms (binding 3):
```glsl
layout(set = 3, binding = 0) uniform ShadowUniforms {
    float u_opacity;
    float u_intensity;
    float u_blurShiftModifier;
    float u_blurIntensityModifier;
    vec2 u_size;                  // Sprite size in pixels
    vec2 u_anchor;                // Anchor point (anchorX*width, SHADOW_OFFSET)
    float u_lightDistPctInv;      // Light attenuation (1=full, 0=none)
    float u_renderScale;
    float _pad1, _pad2;
};
```

Texture (binding 2):
```glsl
layout(set = 2, binding = 0) uniform sampler2D u_texture;  // Per-sprite FBO
```

Output:
```glsl
fragColor = vec4(0.0, 0.0, 0.0, alpha * intensity * opacity);
```

## Geometry Calculations (gpu_renderer.cpp ~line 1775)

### Constants
```cpp
constexpr float COS_45 = 0.7071067811865475f;
constexpr float SHADOW_OFFSET = 19.5f;  // Feet offset from sprite bottom
```

### Depth-Based Flip
```cpp
float sprite_depth = feet_pos.y;
float light_depth = light->y;
float depth_diff = sprite_depth - light_depth;
float flip = (depth_diff - SHADOW_OFFSET) < 0.0f ? -1.0f : 1.0f;
// flip=1: shadow extends DOWN (light behind sprite)
// flip=-1: shadow extends UP (light in front of sprite)
```

### 3D Direction Vector
```cpp
float dx = feet_pos.x - light->x;
float dy = feet_pos.y - light->y;
float light_altitude = std::sqrt(light->radius) * 6.0f * scale;
float dist_3d = std::sqrt(dx*dx + light_altitude*light_altitude + dy*dy);
float dir_x = dx / dist_3d;
float dir_y = -light_altitude / dist_3d;  // Sprite below light
float dir_z = dy / dist_3d;               // Depth direction
```

### Skew (horizontal tilt)
```cpp
float skew_angle = std::atan2(dir_x, -dir_y) * flip;
float skew = std::tan(skew_angle) * 0.5f;
```

### Stretch (shadow length)
```cpp
float depth_angle = std::atan2(std::abs(dir_z), -dir_y);
float tan_depth = std::tan(depth_angle);
float altitudeModifier = std::pow(1.0f / std::abs(tan_depth), 0.35f) * 1.25f;
float skewAbs = std::max(std::abs(skew), 0.1f);
float skewModifier = std::min(std::pow(skewAbs, 0.1f) / skewAbs, 1.0f);
float stretch = std::min(skewModifier * altitudeModifier, 1.6f);
```

### Shadow Dimensions
```cpp
float shadow_w = sprite_w * scale;
float shadow_h = sprite_h * scale * COS_45 * stretch * flip;
```

### Vertex Construction
```cpp
// Quad corners (screen coordinates)
float shadow_center_x = feet_pos.x + x_correction_pixels;  // CURRENT BUG: x_correction not quite right
float shadow_top_y = (flip > 0) ? feet_pos.y - feet_offset : feet_pos.y;
float half_w = shadow_w * 0.5f;
float skew_offset_bottom = shadow_h * skew;

// Top vertices (at feet level, no skew)
float tl_x = shadow_center_x - half_w;
float tr_x = shadow_center_x + half_w;
float tl_y = tr_y = shadow_top_y;

// Bottom vertices (at shadow tip, skewed)
float bl_x = shadow_center_x - half_w + skew_offset_bottom;
float br_x = shadow_center_x + half_w + skew_offset_bottom;
float bl_y = br_y = shadow_top_y + shadow_h;

// Convert to NDC
float ndc_x = (screen_x / swapchain_w) * 2.0f - 1.0f;
float ndc_y = 1.0f - (screen_y / swapchain_h) * 2.0f;
```

## File Locations

| Component | File | Key Lines |
|-----------|------|-----------|
| Shadow entry point | `src/entity.cpp` | `render_shadow()` ~214 |
| Sprite to FBO | `src/gpu_renderer.cpp` | `draw_sprite_to_pass()` ~1655 |
| Shadow geometry | `src/gpu_renderer.cpp` | `draw_shadow_from_pass()` ~1751 |
| Pipeline creation | `src/gpu_renderer.cpp` | `create_multipass_pipelines()` ~470 |
| Vertex shader | `shaders/shadow.vert` | All |
| Fragment shader | `shaders/shadow_perpass.frag` | All |
| Data structures | `include/gpu_renderer.hpp` | PointLight, ShadowVertex, etc. |
| RenderPass pool | `include/render_pass.hpp` | PassManager |
| Sprite properties | `include/sprite_properties.hpp` | SpriteProperties |
| Lighting presets | `src/main.cpp` | `g_lighting_presets[]` ~100 |

## Known Issues

**Horizontal offset**: Shadow base doesn't perfectly align with sprite feet. The `x_correction` from Duelyst's shader is applied but the scale factor is wrong. See `docs/SHADOW_HORIZONTAL_OFFSET_HANDOFF.md` for details.

## To Replace the System

1. **Keep the same API**: `draw_sprite_to_pass()` + `draw_shadow_from_pass()` or equivalent
2. **Use the existing uniforms struct** or update `ShadowPerPassUniforms`
3. **Maintain the rendering order**: Shadows before sprites in render loop
4. **Honor `sprite_props.casts_shadows`** in Entity::render_shadow()
5. **Read light from `scene_light`** or provide override in draw call
6. **Use the RenderPass pool** for per-sprite FBOs (or manage your own)

## Duelyst Reference

Original Duelyst shaders (read-only):
- `app/shaders/ShadowVertex.glsl` - Cast matrix, skew/stretch
- `app/shaders/ShadowHighQualityFragment.glsl` - 7x7 blur
- `app/view/nodes/fx/Light.js` - Light altitude formula (line 319)
