# Grid System Reimplementation Design Spec

This document specifies the reimplementation of Duelyst's grid rendering system in Sylvanshine's SDL3 GPU pipeline.

**Ground Truth Reference:** `duelyst_analysis/summaries/grid_rendering.md`

---

## 1. Scope

### Systems to Implement

| System | Duelyst Source | Priority |
|--------|----------------|----------|
| Tile texture rendering | TileLayer, TileMapScaledSprite | P0 |
| Merged tile algorithm | TileLayer:213-388 | P0 |
| Hover highlight | Player.js:1557-1841 | P1 |
| Movement range display | Player.js:1374-1438 | P1 |
| Attack range display | Player.js:1391-1458 | P1 |
| Tile-based path rendering | Player.js:1972-2080 | P2 |
| Direct arc path (AttackPathSprite) | AttackPathSprite.js | P3 |
| Audio feedback | Player.js:616,973,1079,1166 | P3 |

### Out of Scope (This Phase)
- Network hover synchronization
- Depth shader system (already have working depth via compositor)
- Spell effect tiles

---

## 2. Existing Sylvanshine Infrastructure

### Current GridRenderer (`include/grid_renderer.hpp`)
```cpp
struct GridRenderer {
    int fb_width, fb_height;
    PerspectiveConfig persp_config;

    bool init(const RenderConfig& config);
    void render(const RenderConfig& config);                    // Line grid
    void render_highlight(const RenderConfig& config, BoardPos pos, SDL_FColor color);
    void render_move_range(const RenderConfig& config, BoardPos center, int range, const std::vector<BoardPos>& occupied);
    void render_attack_range(const RenderConfig& config, const std::vector<BoardPos>& attackable_tiles);
};
```

### Current GPURenderer Capabilities
- `draw_sprite()` - textured quads
- `draw_quad_colored()` - solid color quads
- `draw_quad_transformed()` - perspective-transformed quads (4 corners)
- `draw_line()` - line segments
- Texture atlas loading via `load_texture()`

### Current Constants (`include/types.hpp`)
```cpp
constexpr int BOARD_COLS = 9;
constexpr int BOARD_ROWS = 5;
constexpr int TILE_SIZE = 48;  // Base (scaled for resolution)
```
> **Ground Truth:** Duelyst uses TILESIZE=95, we use 48 with 2x scale = 96 (close match)

---

## 3. New Data Structures

### 3.1 Tile Atlas (`include/tile_atlas.hpp`)

```cpp
#pragma once
#include "gpu_renderer.hpp"
#include <string>
#include <unordered_map>

// UV rectangle in normalized coordinates [0,1]
struct AtlasFrame {
    float u0, v0, u1, v1;  // UV bounds
    float offset_x, offset_y;  // Center offset (from ground truth: plist offset field)
    int source_width, source_height;  // Original size before trim
    bool rotated;  // 90° CW rotation in atlas
};

struct TileAtlas {
    GPUTextureHandle texture;
    std::unordered_map<std::string, AtlasFrame> frames;

    bool load(const char* texture_path, const char* manifest_path);
    const AtlasFrame* get_frame(const std::string& name) const;
};
```

> **Ground Truth Reference:** Section 13 "Sprite Atlas Format"
> - Atlas size: 607×254 (Duelyst), we'll create equivalent
> - Frame format: `{{x,y},{w,h}}` with offset and rotation

### 3.2 Tile Highlight State (`include/tile_highlight.hpp`)

```cpp
#pragma once
#include "types.hpp"
#include <vector>

enum class TileHighlightType {
    None,
    Hover,          // Mouse over empty tile
    Selected,       // Selected unit's tile
    Move,           // Reachable movement tile
    Attack,         // Attackable tile
    AttackTarget,   // Tile with attackable enemy
    Card,           // Valid card play location
};

struct TileHighlight {
    BoardPos pos;
    TileHighlightType type;
    float opacity;  // 0.0-1.0
};

// Corner connectivity for merged tiles
// Ground Truth: Section 7 "Merged Tile Corner Values"
// Encoding: bits for neighbors (left, top-left, top, etc.)
struct MergedTileCorner {
    bool edge_left;    // '1' in Duelyst encoding
    bool corner;       // '2' in Duelyst encoding
    bool edge_top;     // '3' in Duelyst encoding
};

struct TileHighlightState {
    std::vector<TileHighlight> highlights;
    BoardPos hover_pos;
    bool hover_valid;

    void clear();
    void set_hover(BoardPos pos);
    void clear_hover();
    void add_move_range(const std::vector<BoardPos>& tiles);
    void add_attack_range(const std::vector<BoardPos>& tiles);
};
```

> **Ground Truth Reference:** Section 8 "Tile Highlight Color Configuration"
> - TILE_SELECT_OPACITY = 200/255 = 0.784
> - TILE_HOVER_OPACITY = 200/255 = 0.784
> - TILE_DIM_OPACITY = 127/255 = 0.498
> - TILE_FAINT_OPACITY = 75/255 = 0.294

### 3.3 Tile Colors (`include/tile_colors.hpp`)

```cpp
#pragma once
#include <SDL3/SDL.h>

// Ground Truth Reference: Section 8 "Tile Highlight Color Configuration"
namespace TileColors {
    // Movement
    constexpr SDL_FColor MOVE = {240.f/255, 240.f/255, 240.f/255, 1.0f};       // #F0F0F0
    constexpr SDL_FColor MOVE_ALT = {1.0f, 1.0f, 1.0f, 1.0f};                  // #FFFFFF

    // Attack/Aggro
    constexpr SDL_FColor AGGRO = {255.f/255, 217.f/255, 0.f/255, 1.0f};        // #FFD900
    constexpr SDL_FColor AGGRO_ALT = {1.0f, 1.0f, 1.0f, 1.0f};                 // #FFFFFF
    constexpr SDL_FColor AGGRO_OPPONENT = {210.f/255, 40.f/255, 70.f/255, 1.0f}; // #D22846

    // Selection
    constexpr SDL_FColor SELECT = {1.0f, 1.0f, 1.0f, 1.0f};                    // #FFFFFF
    constexpr SDL_FColor SELECT_OPPONENT = {210.f/255, 40.f/255, 70.f/255, 1.0f}; // #D22846

    // Hover
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 1.0f};                     // #FFFFFF
    constexpr SDL_FColor HOVER_OPPONENT = {210.f/255, 40.f/255, 70.f/255, 1.0f}; // #D22846

    // Card play
    constexpr SDL_FColor CARD_PLAYER = {255.f/255, 217.f/255, 0.f/255, 1.0f};  // #FFD900
    constexpr SDL_FColor CARD_OPPONENT = {210.f/255, 40.f/255, 70.f/255, 1.0f}; // #D22846
}

namespace TileOpacity {
    constexpr float SELECT = 200.f/255;   // 0.784
    constexpr float HOVER = 200.f/255;    // 0.784
    constexpr float DIM = 127.f/255;      // 0.498
    constexpr float FAINT = 75.f/255;     // 0.294
}
```

---

## 4. Merged Tile Algorithm

### 4.1 Overview

Each highlighted tile is rendered as 4 corner pieces. Each corner connects to adjacent tiles.

> **Ground Truth Reference:** Section 7 "Merged Tile Corner Values" and Section 10 "Tile Sprite Classes"

### 4.2 Corner Piece Selection

```
Corner encoding (per corner of a tile):
- Check 3 neighbors: edge1, diagonal, edge2
- Pattern determines sprite variant:
  - ""      → tile_merged_large_0      (isolated corner)
  - "1"     → tile_merged_large_01     (edge neighbor only)
  - "3"     → tile_merged_large_03     (other edge only)
  - "13"    → tile_merged_large_013    (both edges, no diagonal)
  - "123"   → tile_merged_large_0123   (fully connected)
  - "_seam" → tile_merged_large_0_seam (adjacent different type)
```

### 4.3 Corner Rotation

| Corner | Rotation |
|--------|----------|
| Top-Left | 0° |
| Top-Right | 90° |
| Bottom-Right | 180° |
| Bottom-Left | 270° |

### 4.4 Implementation

```cpp
// In tile_renderer.cpp

enum class CornerPosition { TL, TR, BR, BL };

struct CornerNeighbors {
    bool edge1;      // First edge neighbor
    bool diagonal;   // Diagonal neighbor
    bool edge2;      // Second edge neighbor
};

// Get neighbors for a corner
CornerNeighbors get_corner_neighbors(
    const std::vector<BoardPos>& tiles,
    BoardPos pos,
    CornerPosition corner
) {
    // TL: check left(-1,0), top-left(-1,+1), top(0,+1)
    // TR: check top(0,+1), top-right(+1,+1), right(+1,0)
    // BR: check right(+1,0), bottom-right(+1,-1), bottom(0,-1)
    // BL: check bottom(0,-1), bottom-left(-1,-1), left(-1,0)
    // ...
}

// Select sprite frame based on neighbor pattern
std::string select_corner_frame(CornerNeighbors n, bool is_hover) {
    const char* prefix = is_hover ? "tile_merged_hover_" : "tile_merged_large_";

    if (n.edge1 && n.diagonal && n.edge2) return prefix + "0123";
    if (n.edge1 && n.edge2)               return prefix + "013";
    if (n.edge1)                          return prefix + "01";
    if (n.edge2)                          return prefix + "03";
    return prefix + "0";
}

float get_corner_rotation(CornerPosition corner) {
    switch (corner) {
        case CornerPosition::TL: return 0.0f;
        case CornerPosition::TR: return 90.0f;
        case CornerPosition::BR: return 180.0f;
        case CornerPosition::BL: return 270.0f;
    }
}
```

---

## 5. Tile Renderer Interface

### 5.1 New TileRenderer Struct (`include/tile_renderer.hpp`)

```cpp
#pragma once
#include "types.hpp"
#include "tile_atlas.hpp"
#include "tile_highlight.hpp"
#include "perspective.hpp"

struct TileRenderer {
    TileAtlas atlas;
    PerspectiveConfig persp_config;

    bool init(const RenderConfig& config);

    // Render all current highlights
    void render(const RenderConfig& config, const TileHighlightState& state);

    // Render single tile (for testing/debug)
    void render_tile(const RenderConfig& config, BoardPos pos,
                     const std::string& frame_name, SDL_FColor tint, float opacity);

    // Render merged highlight region
    void render_merged_highlights(const RenderConfig& config,
                                  const std::vector<BoardPos>& tiles,
                                  SDL_FColor color, float opacity, bool is_hover);

    // Render hover tile
    void render_hover(const RenderConfig& config, BoardPos pos, SDL_FColor color);

private:
    Vec2 transform_tile_corner(const RenderConfig& config,
                               BoardPos pos, CornerPosition corner);
    void render_corner_piece(const RenderConfig& config,
                            BoardPos pos, CornerPosition corner,
                            const std::string& frame, SDL_FColor tint, float opacity);
};
```

### 5.2 Integration with Existing GridRenderer

Option A: **Replace GridRenderer** with TileRenderer
Option B: **Compose** - GridRenderer uses TileRenderer internally

**Recommended: Option B**
```cpp
struct GridRenderer {
    TileRenderer tile_renderer;  // NEW
    // ... existing fields ...

    void render(const RenderConfig& config);  // Now uses tile_renderer
};
```

---

## 6. GPU Pipeline Changes

### 6.1 New Shader: Tinted Sprite (`shaders/tinted_sprite.vert/frag`)

Required for color-tinted tile sprites.

```glsl
// tinted_sprite.frag
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_tint;

layout(set = 1, binding = 0) uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 tex = texture(u_texture, v_uv);
    // Multiply RGB by tint, preserve alpha
    fragColor = vec4(tex.rgb * v_tint.rgb, tex.a * v_tint.a);
}
```

### 6.2 New Vertex Format

```cpp
struct TintedSpriteVertex {
    float x, y;      // Position
    float u, v;      // UV
    float r, g, b, a; // Tint color
};
```

### 6.3 Pipeline Creation

Add to `GPURenderer`:
```cpp
SDL_GPUGraphicsPipeline* tinted_sprite_pipeline = nullptr;

// In create_pipelines():
// Create pipeline with TintedSpriteVertex layout
```

---

## 7. Asset Requirements

### 7.1 Tile Atlas

Create `dist/tiles/tiles_board.png` with frames:

| Frame | Size | Purpose |
|-------|------|---------|
| `tile_hover` | 128×128 | Single tile hover |
| `tile_large` | 128×128 | Single tile highlight |
| `tile_merged_large_0` | 64×64 | Corner: isolated |
| `tile_merged_large_01` | 64×64 | Corner: edge1 |
| `tile_merged_large_03` | 64×64 | Corner: edge2 |
| `tile_merged_large_013` | 64×64 | Corner: both edges |
| `tile_merged_large_0123` | 64×64 | Corner: full |
| `tile_merged_large_0_seam` | 64×64 | Corner: seam |
| `tile_merged_hover_*` | 64×64 | Hover variants (same set) |

### 7.2 Atlas Manifest

Create `dist/tiles/tiles_board.json`:
```json
{
  "texture": "tiles_board.png",
  "size": [512, 256],
  "frames": {
    "tile_hover": {"rect": [0, 0, 128, 128], "offset": [0, 0], "rotated": false},
    "tile_large": {"rect": [128, 0, 128, 128], "offset": [0, 0], "rotated": false},
    "tile_merged_large_0": {"rect": [256, 0, 64, 64], "offset": [0, 0], "rotated": false},
    ...
  }
}
```

---

## 8. Implementation Slices

### Slice 1: Tile Atlas Loading
**Files:** `tile_atlas.hpp`, `tile_atlas.cpp`
**Test:** Load atlas, verify frame lookup
**Verification:** Print frame UVs, confirm texture loads

### Slice 2: Single Tile Rendering
**Files:** `tile_renderer.hpp`, `tile_renderer.cpp`
**Test:** Render single `tile_hover` at board position
**Verification:** Visual - tile appears at correct position with perspective

### Slice 3: Tinted Sprite Pipeline
**Files:** `shaders/tinted_sprite.vert`, `shaders/tinted_sprite.frag`, `gpu_renderer.cpp`
**Test:** Render tile with color tint
**Verification:** Visual - tile shows correct color multiplication

### Slice 4: Merged Tile Algorithm (Logic Only)
**Files:** `tile_renderer.cpp`
**Test:** Unit test corner neighbor detection
**Verification:** Print selected frames for known tile configurations

### Slice 5: Merged Tile Rendering
**Files:** `tile_renderer.cpp`
**Test:** Render 3x3 block of highlighted tiles
**Verification:** Visual - corners connect correctly

### Slice 6: Hover Integration
**Files:** `grid_renderer.cpp` (modify), `tile_highlight.hpp`
**Test:** Mouse hover shows tile highlight
**Verification:** Interactive - hover tile follows mouse

### Slice 7: Movement Range Display
**Files:** `grid_renderer.cpp` (modify)
**Test:** Select unit, show movement range as merged tiles
**Verification:** Interactive - movement range displays correctly

### Slice 8: Attack Range Display
**Files:** `grid_renderer.cpp` (modify)
**Test:** Select unit, show attack range with different color
**Verification:** Interactive - attack tiles show in aggro color

---

## 9. File Manifest

### New Files
```
include/
  tile_atlas.hpp
  tile_renderer.hpp
  tile_highlight.hpp
  tile_colors.hpp

src/
  tile_atlas.cpp
  tile_renderer.cpp

shaders/
  tinted_sprite.vert
  tinted_sprite.frag

dist/tiles/
  tiles_board.png
  tiles_board.json
```

### Modified Files
```
include/
  grid_renderer.hpp  (add TileRenderer member)
  gpu_renderer.hpp   (add tinted_sprite_pipeline)

src/
  grid_renderer.cpp  (use TileRenderer for highlights)
  gpu_renderer.cpp   (create tinted sprite pipeline)
```

---

## 10. Timing Constants

> **Ground Truth Reference:** Section 14 "Animation Timing Constants"

```cpp
namespace TileAnim {
    constexpr float FADE_FAST = 0.2f;
    constexpr float FADE_MEDIUM = 0.35f;
    constexpr float FADE_SLOW = 1.0f;
}
```

---

## 11. Z-Order

> **Ground Truth Reference:** Section 9 "Player Tile Z-Order Hierarchy"

Tile highlights render BEFORE entities (lower Z):
```cpp
enum class TileZOrder {
    Board = 1,
    Move = 2,
    Assist = 3,
    Aggro = 4,
    AttackableTarget = 5,
    Card = 6,
    Select = 7,
    Path = 8,
    MouseOver = 9,
};
```

In practice for Sylvanshine: render tiles in `render_single_pass()` before sprites.

---

## 12. Testing Checklist

### Slice 1: Tile Atlas
- [ ] Atlas texture loads without error
- [ ] Frame lookup returns correct UVs for "tile_hover"
- [ ] Frame lookup returns nullptr for unknown frame

### Slice 2: Single Tile
- [ ] Tile renders at (0,0) board position
- [ ] Tile renders at (8,4) board position (far corner)
- [ ] Perspective transform matches existing grid lines

### Slice 3: Tinted Sprite
- [ ] White tint = original colors
- [ ] Red tint = red-tinted tile
- [ ] 50% opacity = translucent tile

### Slice 4: Merged Algorithm
- [ ] Single tile → 4 "0" corners
- [ ] 2 horizontal tiles → correct "01"/"03" pattern
- [ ] 2x2 block → all "0123" corners on interior

### Slice 5: Merged Rendering
- [ ] Corners visually connect
- [ ] No gaps between corner pieces
- [ ] Rotation correct for each corner

### Slice 6-8: Integration
- [ ] Hover tracks mouse correctly
- [ ] Movement range excludes occupied tiles
- [ ] Attack range shows correct color
- [ ] Colors match Duelyst values

---

## Appendix A: Ground Truth Cross-Reference

| This Spec Section | Ground Truth Section |
|-------------------|---------------------|
| 3.2 TileHighlight | §8 Color Config, §9 Z-Order |
| 3.3 TileColors | §8 Color Config |
| 4 Merged Algorithm | §7 Corner Values, §10 Sprite Classes |
| 6.1 Tinted Shader | §13 Atlas Format (tint application) |
| 7 Assets | §13 Atlas Format, Complete Frame Table |
| 10 Timing | §14 Animation Timing |
| 11 Z-Order | §9 Z-Order Hierarchy |
