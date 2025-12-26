# Background Rendering Implementation Plan

Reimplementing Duelyst's parallax battle map system in Sylvanshine's SDL3 GPU renderer.

## Overview

Duelyst renders battle backgrounds using a three-layer parallax system with screen-relative positioning. Each battle map defines:
- **Background layer**: Distant scenery (sky, mountains, horizons)
- **Middleground layer**: Main battle arena surface
- **Foreground layer**: Overlapping elements (rocks, pillars, vegetation)

Layers are positioned relative to screen percentage (0.0-1.0), scaled relative to a reference sprite, and rendered behind/in front of the game board.

---

## Phase 1: Core Infrastructure

### 1.1 BackgroundLayer Struct

```cpp
// include/background_renderer.hpp

enum class LayerType {
    Background,   // Farthest (z-order -9999)
    Middleground, // Main arena (z-order 1)
    Foreground    // Closest, overlays board
};

struct BackgroundLayer {
    GPUTextureHandle texture;

    // Screen-relative positioning (0.0-1.0)
    // {0.5, 0.5} = centered, {0.0, 0.0} = bottom-left, {1.0, 1.0} = top-right
    Vec2 screen_position_pct;

    // Anchor point for sprite (0.0-1.0)
    // Determines which point of the sprite aligns to screen_position_pct
    Vec2 anchor;

    // Scale factor relative to middleground
    float scale_modifier;

    // Ambient light color offset (RGB, can be negative)
    SDL_FColor ambient_offset;

    // Render order within layer type
    int z_order;

    // Blend mode (normal, additive, etc.)
    SDL_BlendMode blend_mode;

    LayerType type;
};
```

### 1.2 BattleMapTemplate Class

```cpp
// include/battlemap_template.hpp

struct LightConfig {
    Vec2 offset;              // From screen center
    float radius;
    float intensity;
    SDL_FColor color;
    bool casts_shadows;
};

struct ParticleConfig {
    std::string type;         // "dust", "rays", "clouds", etc.
    Vec2 screen_position_pct;
    int max_particles;
    bool is_background;       // true = behind board, false = foreground
};

class BattleMapTemplate {
public:
    std::string name;
    std::vector<BackgroundLayer> layers;
    std::vector<LightConfig> lights;
    std::vector<ParticleConfig> particles;

    // Global settings
    bool has_sun_rays;
    float bloom_intensity;
    Vec2 wind_direction;

    static BattleMapTemplate load(const std::string& map_id);
};
```

### 1.3 BackgroundRenderer Class

```cpp
// include/background_renderer.hpp

class BackgroundRenderer {
public:
    bool init();
    void load_map(const BattleMapTemplate& map);
    void unload();

    // Render layers behind the board
    void render_background(const RenderConfig& config);

    // Render layers in front of the board
    void render_foreground(const RenderConfig& config);

    void update(float dt);  // For animated layers

private:
    std::vector<BackgroundLayer> background_layers_;
    std::vector<BackgroundLayer> foreground_layers_;

    // Reference sprite for scale calculations
    GPUTextureHandle middleground_ref_;
    float base_scale_;

    Vec2 calculate_screen_position(
        const BackgroundLayer& layer,
        const RenderConfig& config);

    float calculate_layer_scale(
        const BackgroundLayer& layer,
        const RenderConfig& config);
};
```

---

## Phase 2: Positioning System

Duelyst's screen-relative positioning ensures backgrounds scale and position correctly at any resolution.

### 2.1 Position Calculation

```cpp
Vec2 BackgroundRenderer::calculate_screen_position(
    const BackgroundLayer& layer,
    const RenderConfig& config)
{
    // Screen position from percentage
    float screen_x = layer.screen_position_pct.x * config.window_w;
    float screen_y = layer.screen_position_pct.y * config.window_h;

    // Offset by anchor point (anchor is in sprite-local 0-1 coordinates)
    float tex_w = static_cast<float>(layer.texture.width) * calculate_layer_scale(layer, config);
    float tex_h = static_cast<float>(layer.texture.height) * calculate_layer_scale(layer, config);

    screen_x -= tex_w * layer.anchor.x;
    screen_y -= tex_h * layer.anchor.y;

    return {screen_x, screen_y};
}
```

### 2.2 Scale Calculation

```cpp
float BackgroundRenderer::calculate_layer_scale(
    const BackgroundLayer& layer,
    const RenderConfig& config)
{
    // Base scale: fit middleground to window height
    float ref_height = static_cast<float>(middleground_ref_.height);
    float window_scale = config.window_h / ref_height;

    // Apply per-layer modifier
    return window_scale * layer.scale_modifier * config.scale;
}
```

### 2.3 Common Anchor Patterns

| Pattern | Position PCT | Anchor | Use Case |
|---------|-------------|--------|----------|
| Centered | {0.5, 0.5} | {0.5, 0.5} | Middleground arena |
| Top-center | {0.5, 1.0} | {0.5, 1.0} | Sky/horizon backgrounds |
| Bottom-left | {0.0, 0.0} | {0.0, 0.0} | Left foreground element |
| Bottom-right | {1.0, 0.0} | {1.0, 0.0} | Right foreground element |
| Top-right | {1.0, 1.0} | {1.0, 1.0} | Corner decoration |

---

## Phase 3: Render Pipeline Integration

### 3.1 Render Order in main.cpp

```cpp
void render(GameState& state, const RenderConfig& config) {
    g_gpu.begin_frame();

    // 1. Background layers (behind everything)
    state.background_renderer.render_background(config);

    // 2. Board + game elements (existing code)
    render_single_pass(state, config);

    // 3. Foreground layers (in front of units)
    state.background_renderer.render_foreground(config);

    // 4. UI overlay (existing code)
    render_ui(state, config);

    g_gpu.end_frame();
}
```

### 3.2 Layer Rendering

```cpp
void BackgroundRenderer::render_background(const RenderConfig& config) {
    // Sort by z-order
    std::vector<const BackgroundLayer*> sorted;
    for (const auto& layer : background_layers_) {
        sorted.push_back(&layer);
    }
    std::sort(sorted.begin(), sorted.end(),
        [](const auto* a, const auto* b) { return a->z_order < b->z_order; });

    for (const auto* layer : sorted) {
        render_layer(*layer, config);
    }
}

void BackgroundRenderer::render_layer(
    const BackgroundLayer& layer,
    const RenderConfig& config)
{
    if (!layer.texture) return;

    Vec2 pos = calculate_screen_position(layer, config);
    float scale = calculate_layer_scale(layer, config);

    SDL_FRect dst = {
        pos.x,
        pos.y,
        layer.texture.width * scale,
        layer.texture.height * scale
    };

    SDL_FRect src = {
        0, 0,
        static_cast<float>(layer.texture.width),
        static_cast<float>(layer.texture.height)
    };

    // Apply ambient color offset as tint
    SDL_FColor tint = {
        1.0f + layer.ambient_offset.r / 255.0f,
        1.0f + layer.ambient_offset.g / 255.0f,
        1.0f + layer.ambient_offset.b / 255.0f,
        1.0f
    };

    g_gpu.draw_sprite(layer.texture, src, dst, tint.a);
}
```

---

## Phase 4: Asset Pipeline

### 4.1 Resource Structure

```
dist/
├── backgrounds/
│   ├── battlemap0/
│   │   ├── middleground.png     # Main arena
│   │   ├── background.png       # Sky/distant
│   │   ├── foreground_001.png   # Left rocks
│   │   └── foreground_002.png   # Right rocks
│   ├── battlemap1/
│   │   └── ...
│   └── ...
├── particles/
│   ├── dust.png
│   └── rays.png
└── assets.json  # Updated manifest
```

### 4.2 Asset Manifest Entry

```json
{
  "backgrounds": {
    "battlemap0": {
      "middleground": "backgrounds/battlemap0/middleground.png",
      "background": "backgrounds/battlemap0/background.png",
      "foreground_001": "backgrounds/battlemap0/foreground_001.png",
      "foreground_002": "backgrounds/battlemap0/foreground_002.png"
    }
  }
}
```

### 4.3 Map Configuration File

```json
// dist/backgrounds/battlemap0/config.json
{
  "name": "Lyonar Highlands",
  "layers": [
    {
      "texture": "middleground.png",
      "type": "middleground",
      "position_pct": [0.5, 0.5],
      "anchor": [0.5, 0.5],
      "scale_modifier": 1.0,
      "z_order": 1
    },
    {
      "texture": "background.png",
      "type": "background",
      "position_pct": [0.5, 1.0],
      "anchor": [0.5, 1.0],
      "scale_modifier": 1.0,
      "z_order": -9999
    },
    {
      "texture": "foreground_001.png",
      "type": "foreground",
      "position_pct": [0.0, 0.0],
      "anchor": [0.0, 0.0],
      "scale_modifier": 1.0,
      "z_order": 0
    },
    {
      "texture": "foreground_002.png",
      "type": "foreground",
      "position_pct": [1.0, 0.0],
      "anchor": [1.0, 0.0],
      "scale_modifier": 1.0,
      "z_order": 0
    }
  ],
  "lights": [
    {
      "offset": [2340, -2268],
      "radius": 1500,
      "intensity": 1.0,
      "casts_shadows": true
    }
  ],
  "bloom_intensity": 0.1
}
```

---

## Phase 5: Duelyst Map Configurations

Reference configurations extracted from `BattleMap.js`:

### 5.1 BATTLEMAP0 (Lyonar Highlands)

```cpp
BattleMapTemplate battlemap0() {
    BattleMapTemplate map;
    map.name = "Lyonar Highlands";

    // Middleground (main arena)
    map.layers.push_back({
        .texture = load_texture("battlemap0_middleground.png"),
        .screen_position_pct = {0.5f, 0.5f},
        .anchor = {0.5f, 0.5f},
        .scale_modifier = 1.0f,
        .ambient_offset = {-50, -50, -50, 0},  // Darker ambient
        .z_order = 1,
        .type = LayerType::Middleground
    });

    // Background (sky)
    map.layers.push_back({
        .texture = load_texture("battlemap0_background.png"),
        .screen_position_pct = {0.5f, 1.0f},
        .anchor = {0.5f, 1.0f},
        .scale_modifier = 1.0f,
        .z_order = -9999,
        .type = LayerType::Background
    });

    // Foreground rocks (left)
    map.layers.push_back({
        .texture = load_texture("battlemap0_foreground_001.png"),
        .screen_position_pct = {0.0f, 0.0f},
        .anchor = {0.0f, 0.0f},
        .scale_modifier = 1.0f,
        .z_order = 0,
        .type = LayerType::Foreground
    });

    // Foreground rocks (right)
    map.layers.push_back({
        .texture = load_texture("battlemap0_foreground_002.png"),
        .screen_position_pct = {1.0f, 0.0f},
        .anchor = {1.0f, 0.0f},
        .scale_modifier = 1.0f,
        .z_order = 0,
        .type = LayerType::Foreground
    });

    // Light configuration
    map.lights.push_back({
        .offset = {CONFIG_WINDOW_W * 3.25f, -CONFIG_WINDOW_H * 3.15f},
        .radius = 1500.0f,
        .intensity = 1.0f,
        .casts_shadows = true
    });

    map.bloom_intensity = 0.1f;

    return map;
}
```

### 5.2 BATTLEMAP1 (Magmar Wastes)

```cpp
BattleMapTemplate battlemap1() {
    BattleMapTemplate map;
    map.name = "Magmar Wastes";

    map.layers.push_back({
        .texture = load_texture("battlemap1_middleground.png"),
        .screen_position_pct = {0.5f, 0.5f},
        .anchor = {0.5f, 0.5f},
        .ambient_offset = {-50, -50, -50, 0},
        .z_order = 1,
        .type = LayerType::Middleground
    });

    map.layers.push_back({
        .texture = load_texture("battlemap1_background.png"),
        .screen_position_pct = {0.5f, 1.0f},
        .anchor = {0.5f, 1.0f},
        .z_order = -9999,
        .type = LayerType::Background
    });

    // Light from above (positive Y offset)
    map.lights.push_back({
        .offset = {CONFIG_WINDOW_H * 3.25f, CONFIG_WINDOW_H * 3.75f},
        .radius = 1500.0f,
        .intensity = 1.0f,
        .casts_shadows = true
    });

    return map;
}
```

### 5.3 Key Light Position Patterns

| Map | Light X | Light Y | Shadow Direction |
|-----|---------|---------|------------------|
| BATTLEMAP0 | +3.25×W | -3.15×H | Down-right |
| BATTLEMAP1 | +3.25×H | +3.75×H | Up from bottom |
| BATTLEMAP3 | -3.25×H | +3.75×H | Up from left |

---

## Phase 6: Shader Integration

### 6.1 Background Sprite Shader

Simple textured quad with ambient tint support:

```glsl
// shaders/background.frag
#version 450

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform PushConstants {
    vec4 ambient_offset;  // RGB offset, A unused
    float opacity;
} pc;

void main() {
    vec4 color = texture(u_texture, v_texcoord);

    // Apply ambient offset (clamped to valid range)
    color.rgb = clamp(color.rgb + pc.ambient_offset.rgb / 255.0, 0.0, 1.0);
    color.a *= pc.opacity;

    o_color = color;
}
```

### 6.2 Additive Blend Mode for Glows

Some foreground elements (lava glow, magic effects) use additive blending:

```cpp
if (layer.blend_mode == SDL_BLENDMODE_ADD) {
    g_gpu.set_blend_mode_additive();
    g_gpu.draw_sprite(layer.texture, src, dst, layer.opacity);
    g_gpu.set_blend_mode_normal();
}
```

---

## Phase 7: Particle System (Optional Enhancement)

### 7.1 Dust Particles

```cpp
struct DustParticle {
    Vec2 position;
    Vec2 velocity;
    float lifetime;
    float age;
    float opacity;
};

class DustSystem {
public:
    void update(float dt, Vec2 wind_direction);
    void render(const RenderConfig& config);

private:
    std::vector<DustParticle> particles_;
    GPUTextureHandle dust_texture_;
    int max_particles_;
};
```

### 7.2 Sun Ray System

From Duelyst's implementation, sun rays are batch-rendered sprites:

```cpp
void render_sun_rays(const RenderConfig& config, Vec2 light_pos) {
    // Rays emanate from light position toward screen center
    Vec2 center = {config.window_w * 0.5f, config.window_h * 0.5f};
    Vec2 dir = normalize(center - light_pos);

    for (int i = 0; i < NUM_RAYS; i++) {
        float angle = (i / float(NUM_RAYS)) * 2 * M_PI;
        float ray_opacity = 0.1f + 0.05f * sin(time * 2.0f + angle);

        // Render ray sprite rotated toward light
        render_ray_sprite(light_pos, angle, ray_opacity, config);
    }
}
```

---

## Implementation Order

1. **Phase 1**: Core structs (`BackgroundLayer`, `BattleMapTemplate`)
2. **Phase 2**: Position/scale calculations
3. **Phase 3**: Render pipeline integration (hardcoded single map)
4. **Phase 4**: Asset pipeline for background textures
5. **Phase 5**: Multiple map support with configuration loading
6. **Phase 6**: Shader with ambient offset support
7. **Phase 7**: Particle effects (dust, rays)

**See also:** `random_map_selection.md` — Random background on game launch

---

## Asset Extraction Notes

Duelyst background assets are in:
- `app/resources/` (high-res source)
- Resource keys in `RSX` namespace (e.g., `RSX.battlemap0_middleground.img`)

For Sylvanshine:
1. Copy PNG assets to `dist/backgrounds/`
2. Update `build_assets.py` to include background textures in manifest
3. Scale appropriately (Duelyst assets are 2x for retina)

---

## References

- `app/view/layers/game/BattleMap.js` — Complete map configurations
- `app/view/nodes/map/GroundSprite.js` — Screen-relative positioning
- `app/view/nodes/map/EnvironmentSprite.js` — Foreground elements
- `duelyst_analysis/summaries/view.md` — Layer hierarchy overview
- `duelyst_analysis/summaries/cosmetics.md` — Visual customization

---

## Success Criteria

- [ ] Background renders behind game board
- [ ] Middleground fills screen at any resolution
- [ ] Foreground overlays game board correctly
- [ ] Layers scale proportionally with window
- [ ] Light positions match Duelyst presets
- [ ] Multiple maps can be loaded at runtime
