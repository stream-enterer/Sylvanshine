# Grid System Design Specification v2

Complete reimplementation of Duelyst's grid tile highlighting for Sylvanshine, designed to integrate cleanly with existing systems.

**Forensic Sources:**
- `duelyst_analysis/summaries/grid_rendering.md`
- `duelyst_analysis/flows/tile_interaction_flow.md`

**Sylvanshine Integration Points:**
- `src/grid_renderer.cpp` — Existing tile rendering (to be extended)
- `src/types.cpp` — Coordinate transforms (reuse exactly)
- `src/entity.cpp` — Shadow/sprite positioning reference
- `include/entity.hpp` — Existing timing constants

---

## 1. Design Principles

### 1.1 Integration Over Replacement

- **Extend `GridRenderer`**, don't create parallel `TileRenderer`
- **Extend `GameState`**, don't create duplicate `TileHighlightState`
- **Reuse existing transforms** — `board_to_screen_perspective()`, `transform_board_point()`
- **Reuse existing constants** — `FADE_FAST`, `FADE_MEDIUM`, `FADE_SLOW` from `entity.hpp`

### 1.2 Bitmap First, SDF Later

Start with Duelyst's actual tile PNG sprites. Only switch to procedural SDF if:
- Resolution scaling issues arise, OR
- Asset extraction proves problematic

### 1.3 Conservative Changes

- Keep current render order until proven necessary to change
- Keep current colors until Duelyst-style is explicitly requested
- Scale all Duelyst pixel values for Sylvanshine's coordinate system

---

## 2. Coordinate System Alignment

### 2.1 Critical Constants

```cpp
// From types.hpp (DO NOT CHANGE)
constexpr int TILE_SIZE = 48;
constexpr float SHADOW_OFFSET = 19.5f;  // Sprite feet position from bottom

// From perspective.hpp (DO NOT CHANGE)
constexpr float BOARD_X_ROTATION = 16.0f;   // Board/tiles use this
constexpr float ENTITY_X_ROTATION = 26.0f;  // Entity sprites use this (10° difference!)
```

### 2.2 The Perspective Mismatch Problem

**Board tiles** transform at 16°, **entity sprites** at 26°. This is intentional in Duelyst — sprites "pop" forward slightly. But it means:

- Tile highlights at `board_to_screen_perspective()` won't perfectly match sprite feet
- Selection boxes must account for this visual offset

**Solution:** Accept the offset as intentional design. Selection box highlights the *tile*, not the sprite.

### 2.3 Scale Factor Handling

All Duelyst pixel values must be scaled:

```cpp
// Duelyst used TILESIZE = 95, Sylvanshine uses 48 at 1x or 96 at 2x
constexpr float DUELYST_TILE = 95.0f;

inline float scale_from_duelyst(float duelyst_px, const RenderConfig& config) {
    return duelyst_px * (config.tile_size() / DUELYST_TILE);
}
```

### 2.4 Tile-to-Sprite Vertical Alignment

Entity `screen_pos` is at **feet position**. Tile highlights center on **tile center**.

```cpp
// Tile center in screen coords
Vec2 tile_center = transform_board_point(config,
    (pos.x + 0.5f) * config.tile_size(),
    (pos.y + 0.5f) * config.tile_size());

// Entity feet position (for reference)
Vec2 feet_pos = board_to_screen_perspective(config, pos);
// These differ due to perspective — tile_center is where highlight goes
```

---

## 3. State Integration

### 3.1 Extend GameState (main.cpp)

Add hover tracking to existing `GameState`:

```cpp
struct GameState {
    // ... existing fields ...

    // NEW: Hover state for tile system
    BoardPos hover_pos{-1, -1};
    bool hover_valid = false;
    bool was_hovering_on_board = false;  // For instant transition detection

    // NEW: Computed path to hover target
    std::vector<BoardPos> movement_path;

    // NEW: Blob opacity for dimming during hover
    float move_blob_opacity = 1.0f;
    float attack_blob_opacity = 1.0f;

    // NEW: Active fade animations
    std::vector<TileFadeAnim> tile_anims;
};
```

### 3.2 Tile Fade Animation

Integrate with existing update loop pattern:

```cpp
// In grid_renderer.hpp

// Target identifiers for fade animations (avoids raw pointers)
enum class FadeTarget {
    MoveBlobOpacity,
    AttackBlobOpacity,
};

struct TileFadeAnim {
    FadeTarget target;
    float from, to;
    float duration;
    float elapsed = 0.0f;

    bool update(float dt) {
        elapsed += dt;
        float t = std::min(elapsed / duration, 1.0f);
        return elapsed >= duration;
    }

    float current_value() const {
        float t = std::min(elapsed / duration, 1.0f);
        return from + (to - from) * t;
    }
};
```

Update in main loop:

```cpp
void update(GameState& state, float dt, const RenderConfig& config) {
    // ... existing update code ...

    // Update tile animations and apply values
    for (auto& anim : state.tile_anims) {
        anim.update(dt);
        float val = anim.current_value();
        switch (anim.target) {
            case FadeTarget::MoveBlobOpacity:
                state.move_blob_opacity = val;
                break;
            case FadeTarget::AttackBlobOpacity:
                state.attack_blob_opacity = val;
                break;
        }
    }

    // Remove completed animations
    state.tile_anims.erase(
        std::remove_if(state.tile_anims.begin(), state.tile_anims.end(),
            [](const TileFadeAnim& a) { return a.elapsed >= a.duration; }),
        state.tile_anims.end());
}
```

**Note:** Using enum instead of raw pointers ensures GameState can be safely moved/copied.

---

## 4. Pathfinding

### 4.1 Required Headers

Add to `grid_renderer.cpp`:
```cpp
#include <queue>
#include <unordered_map>
#include <algorithm>  // for std::reverse
```

### 4.2 Add Path Calculation

Currently missing — add to `grid_renderer.cpp`:

```cpp
// BFS pathfinding from start to goal, avoiding blocked tiles
std::vector<BoardPos> get_path_to(BoardPos start, BoardPos goal,
                                   const std::vector<BoardPos>& blocked) {
    if (start == goal) return {start};

    // BFS with parent tracking
    std::queue<BoardPos> frontier;
    std::unordered_map<int, BoardPos> came_from;  // key = y * BOARD_COLS + x

    auto key = [](BoardPos p) { return p.y * BOARD_COLS + p.x; };
    auto is_blocked = [&](BoardPos p) {
        for (const auto& b : blocked) if (b == p) return true;
        return false;
    };

    frontier.push(start);
    came_from[key(start)] = {-1, -1};

    const BoardPos dirs[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

    while (!frontier.empty()) {
        BoardPos current = frontier.front();
        frontier.pop();

        if (current == goal) {
            // Reconstruct path
            std::vector<BoardPos> path;
            BoardPos p = goal;
            while (p.x >= 0) {
                path.push_back(p);
                p = came_from[key(p)];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& d : dirs) {
            BoardPos next = {current.x + d.x, current.y + d.y};
            if (!next.is_valid()) continue;
            if (is_blocked(next) && next != goal) continue;
            if (came_from.count(key(next))) continue;

            frontier.push(next);
            came_from[key(next)] = current;
        }
    }

    return {};  // No path found
}
```

---

## 5. Configuration Constants

### 5.1 Timing (Reuse Existing)

```cpp
// From entity.hpp — DO NOT REDEFINE
constexpr float FADE_FAST = 0.2f;
constexpr float FADE_MEDIUM = 0.35f;
constexpr float FADE_SLOW = 1.0f;
```

### 5.2 Colors (Current Sylvanshine Values)

Keep existing colors for now to avoid visual identity change.
Add to `grid_renderer.hpp`:

```cpp
namespace TileColor {
    // Current Sylvanshine colors (keep for compatibility)
    constexpr SDL_FColor MOVE_CURRENT   = {1.0f, 1.0f, 1.0f, 200.0f/255.0f};
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 100.0f/255.0f, 100.0f/255.0f, 200.0f/255.0f};

    // Duelyst colors (for future use)
    constexpr SDL_FColor MOVE_DUELYST   = {0.941f, 0.941f, 0.941f, 1.0f};  // #F0F0F0
    constexpr SDL_FColor AGGRO_DUELYST  = {1.0f, 0.851f, 0.0f, 1.0f};      // #FFD900 yellow!

    // Path and hover
    constexpr SDL_FColor PATH  = {1.0f, 1.0f, 1.0f, 150.0f/255.0f};
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 200.0f/255.0f};
}
```

**Note:** Duelyst attack tiles are YELLOW (#FFD900), not red. Sylvanshine currently uses red. This is a deliberate deviation — changing would alter game feel significantly.

### 5.3 Opacity Values

```cpp
namespace TileOpacity {
    constexpr float FULL  = 200.0f / 255.0f;  // 0.784 — Selected/hover
    constexpr float DIM   = 127.0f / 255.0f;  // 0.498 — Blob during hover
    constexpr float FAINT = 75.0f / 255.0f;   // 0.294 — Passive hover
}
```

### 5.4 Scaled Duelyst Values

```cpp
namespace TileAnim {
    // These are Duelyst pixel values — must scale at runtime
    constexpr float PATH_ARC_DISTANCE_DUELYST = 47.5f;
    constexpr float PATH_FADE_DISTANCE_DUELYST = 40.0f;

    // Usage: scale_from_duelyst(PATH_ARC_DISTANCE_DUELYST, config)
}
```

---

## 6. Z-Order and Render Integration

### 6.1 Current Render Order (Preserve)

```cpp
void render_single_pass(GameState& state, const RenderConfig& config) {
    // 1. Grid lines
    state.grid_renderer.render(config);

    // 2. Move highlights (blob)
    state.grid_renderer.render_move_range(...);

    // 3. Attack highlights (blob)
    state.grid_renderer.render_attack_range(...);

    // 4. NEW: Path overlay
    state.grid_renderer.render_path(config, state.movement_path);

    // 5. NEW: Hover tile
    if (state.hover_valid) {
        state.grid_renderer.render_hover(config, state.hover_pos);
    }

    // 6. Shadows (Y-sorted) — KEEP AFTER TILES
    for (idx : render_order) {
        state.units[idx].render_shadow(config);
    }

    // 7. Sprites (Y-sorted)
    for (idx : render_order) {
        state.units[idx].render(config);
    }

    // ... rest unchanged ...
}
```

**Why shadows stay after tiles:** Shadows partially occlude tile highlight edges, which looks natural. Reversing would make shadows "cut out" of highlights unnaturally.

### 6.2 Simplified Z-Order

```cpp
// Within tile system only (all render before shadows/sprites)
enum class TileLayer {
    Grid,       // Grid lines
    MoveBlob,   // Movement range
    AttackBlob, // Attack range
    Path,       // Movement path arrow
    Hover,      // Mouse hover tile
};
// Removed: Assist (unused), Select (merged with hover), Card (not implemented)
```

---

## 7. Merged Tile Algorithm

### 7.1 Overview

Each tile in a contiguous blob renders as 4 corner quarter-tiles. This creates smooth blob boundaries.

```
Tile with all neighbors:     Edge tile:
┌─────┬─────┐                ┌─────┬─────┐
│ TL  │ TR  │                │(TL) │(TR) │  ← rounded corners
│ full│full │                │     │     │
├─────┼─────┤                ├─────┼─────┤
│ BL  │ BR  │                │(BL) │(BR) │
│full │full │                │     │     │
└─────┴─────┘                └─────┴─────┘
```

### 7.2 Neighbor Detection Per Corner

```cpp
struct CornerNeighbors {
    bool edge1;     // Adjacent on first edge
    bool diagonal;  // Diagonally adjacent
    bool edge2;     // Adjacent on second edge
};

// Check pattern for each corner:
//   TL: left(-1,0), top-left(-1,-1), top(0,-1)
//   TR: top(0,-1), top-right(+1,-1), right(+1,0)
//   BR: right(+1,0), bottom-right(+1,+1), bottom(0,+1)
//   BL: bottom(0,+1), bottom-left(-1,+1), left(-1,0)

CornerNeighbors get_corner_neighbors(BoardPos pos, int corner,
                                      const std::vector<BoardPos>& blob) {
    constexpr int offsets[4][3][2] = {
        {{-1, 0}, {-1,-1}, { 0,-1}},  // TL
        {{ 0,-1}, { 1,-1}, { 1, 0}},  // TR
        {{ 1, 0}, { 1, 1}, { 0, 1}},  // BR
        {{ 0, 1}, {-1, 1}, {-1, 0}},  // BL
    };

    auto in_blob = [&](int dx, int dy) {
        BoardPos check = {pos.x + dx, pos.y + dy};
        for (const auto& p : blob) if (p == check) return true;
        return false;
    };

    return {
        in_blob(offsets[corner][0][0], offsets[corner][0][1]),
        in_blob(offsets[corner][1][0], offsets[corner][1][1]),
        in_blob(offsets[corner][2][0], offsets[corner][2][1]),
    };
}
```

### 7.3 Sprite Variant Selection

```cpp
enum class CornerVariant {
    Solo,       // No neighbors — rounded corner
    Edge1,      // One edge neighbor
    Edge2,      // Other edge neighbor
    BothEdges,  // Both edges, no diagonal — inner corner
    Full,       // All three — solid fill
};

CornerVariant select_variant(CornerNeighbors n) {
    if (n.edge1 && n.edge2) {
        return n.diagonal ? CornerVariant::Full : CornerVariant::BothEdges;
    }
    if (n.edge1) return CornerVariant::Edge1;
    if (n.edge2) return CornerVariant::Edge2;
    return CornerVariant::Solo;
}
```

### 7.4 Corner Quad Geometry

Transform corner position to screen-space quad:

```cpp
// In grid_renderer.cpp
TransformedQuad get_corner_quad(const RenderConfig& config, BoardPos pos, int corner) {
    int ts = config.tile_size();
    int half = ts / 2;

    // Corner index: 0=TL, 1=TR, 2=BR, 3=BL
    // X offset: 0 for left corners (0,3), half for right corners (1,2)
    // Y offset: 0 for top corners (0,1), half for bottom corners (2,3)
    float x = static_cast<float>(pos.x * ts + ((corner == 1 || corner == 2) ? half : 0));
    float y = static_cast<float>(pos.y * ts + ((corner == 2 || corner == 3) ? half : 0));

    return transform_rect_perspective(x, y, static_cast<float>(half),
                                       static_cast<float>(half), persp_config);
}
```

### 7.5 Sprite Assets Required

Extract from Duelyst or create:
```
dist/sprites/tiles/
    tile_corner_solo.png      — Rounded quarter-circle
    tile_corner_edge1.png     — Half-pill shape
    tile_corner_edge2.png     — Half-pill shape (90° from edge1)
    tile_corner_both.png      — Inner corner notch
    tile_corner_full.png      — Solid quarter-square
```

Each sprite is quarter-tile size: `(TILE_SIZE/2) x (TILE_SIZE/2)` = 24x24 at 1x scale.

---

## 8. Hover State Machine

### 8.1 Input Handling

Mouse motion already updates `state.mouse_pos` in `handle_events()`.
Hover state updates happen in `update()` where `config` is available:

```cpp
// In handle_events() — already exists, no change needed:
else if (event.type == SDL_EVENT_MOUSE_MOTION) {
    state.mouse_pos = {static_cast<float>(event.motion.x),
                       static_cast<float>(event.motion.y)};
    // ... existing facing code ...
}

// NEW: Add to update() function in main.cpp:
void update(GameState& state, float dt, const RenderConfig& config) {
    // ... existing update code ...

    // Update hover state (config available here)
    update_hover_state(state, config);

    // Update tile animations...
}

void update_hover_state(GameState& state, const RenderConfig& config) {
    state.was_hovering_on_board = state.hover_valid;
    BoardPos new_hover = screen_to_board_perspective(config, state.mouse_pos);
    state.hover_valid = new_hover.is_valid();
    state.hover_pos = new_hover;

    // Update path if unit selected and hovering valid move target
    if (state.hover_valid && state.selected_unit_idx >= 0) {
        update_hover_path(state, config);
    } else if (!state.hover_valid && !state.movement_path.empty()) {
        // Clear path when leaving board
        state.movement_path.clear();
        start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                          state.move_blob_opacity, 1.0f, FADE_FAST);
    }
}
```

### 8.2 Hover Path Update

```cpp
void update_hover_path(GameState& state, const RenderConfig& config) {
    // Check if hover is in movement range
    bool in_move_range = false;
    for (const auto& t : state.reachable_tiles) {
        if (t == state.hover_pos) { in_move_range = true; break; }
    }

    if (in_move_range) {
        // Calculate path
        auto occupied = get_occupied_positions(state, state.selected_unit_idx);
        BoardPos start = state.units[state.selected_unit_idx].board_pos;
        state.movement_path = get_path_to(start, state.hover_pos, occupied);

        // Dim blob (instant if already on board, else animate)
        float fade_dur = state.was_hovering_on_board ? 0.0f : FADE_FAST;
        start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                          state.move_blob_opacity, TileOpacity::DIM, fade_dur);
    } else {
        // Clear path, restore blob
        state.movement_path.clear();
        if (state.move_blob_opacity < 1.0f) {
            start_opacity_fade(state, FadeTarget::MoveBlobOpacity,
                              state.move_blob_opacity, 1.0f, FADE_FAST);
        }
    }
}

void start_opacity_fade(GameState& state, FadeTarget target,
                        float from, float to, float duration) {
    if (duration <= 0.0f) {
        // Instant — apply directly
        switch (target) {
            case FadeTarget::MoveBlobOpacity:
                state.move_blob_opacity = to;
                break;
            case FadeTarget::AttackBlobOpacity:
                state.attack_blob_opacity = to;
                break;
        }
        return;
    }
    // Remove existing animation on this target
    state.tile_anims.erase(
        std::remove_if(state.tile_anims.begin(), state.tile_anims.end(),
            [target](const TileFadeAnim& a) { return a.target == target; }),
        state.tile_anims.end());

    state.tile_anims.push_back({target, from, to, duration, 0.0f});
}
```

### 8.3 Key Behavior: Instant Transitions Within Board

From Duelyst analysis: When mouse moves tile-to-tile while staying on board, transitions are instant (`fade_dur = 0.0`). This creates responsive feel.

Only when entering/exiting board does the 0.2s fade occur.

### 8.4 Clear State on Deselection

Modify existing `clear_selection()` in main.cpp:

```cpp
void clear_selection(GameState& state) {
    if (state.selected_unit_idx >= 0 &&
        state.selected_unit_idx < static_cast<int>(state.units.size())) {
        state.units[state.selected_unit_idx].restore_facing();
    }
    state.selected_unit_idx = -1;
    state.reachable_tiles.clear();
    state.attackable_tiles.clear();

    // NEW: Clear tile highlight state
    state.movement_path.clear();
    state.move_blob_opacity = 1.0f;
    state.attack_blob_opacity = 1.0f;
    // Remove any pending fade animations
    state.tile_anims.clear();
}
```

---

## 9. Path Rendering

### 9.1 Segment Types

```cpp
enum class PathSegment {
    Start,              // First tile (unit position)
    Straight,           // Middle, no turn
    Corner,             // 90° turn
    CornerFlipped,      // Opposite rotation turn
    End,                // Final tile (arrow head)
};
```

### 9.2 Segment Selection

```cpp
PathSegment select_path_segment(const std::vector<BoardPos>& path, int idx) {
    if (idx == 0) return PathSegment::Start;
    if (idx == path.size() - 1) return PathSegment::End;

    // Direction vectors
    BoardPos prev = path[idx - 1];
    BoardPos curr = path[idx];
    BoardPos next = path[idx + 1];

    int dx_in = curr.x - prev.x;
    int dy_in = curr.y - prev.y;
    int dx_out = next.x - curr.x;
    int dy_out = next.y - curr.y;

    // Same direction = straight
    if (dx_in == dx_out && dy_in == dy_out) return PathSegment::Straight;

    // Corner — cross product determines flip
    int cross = dx_in * dy_out - dy_in * dx_out;
    return cross > 0 ? PathSegment::Corner : PathSegment::CornerFlipped;
}
```

### 9.3 Segment Rotation

```cpp
float path_segment_rotation(BoardPos from, BoardPos to) {
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    // Return degrees for sprite rotation
    if (dx > 0) return 0.0f;    // Right
    if (dx < 0) return 180.0f;  // Left
    if (dy > 0) return 90.0f;   // Down
    return 270.0f;              // Up
}
```

### 9.4 Path Sprite Assets

```
dist/sprites/tiles/
    path_start.png      — Short stub at unit
    path_straight.png   — Full-width line segment
    path_corner.png     — L-shaped curve
    path_end.png        — Arrow head
```

---

## 10. GridRenderer Extensions

### 10.1 New Methods

```cpp
// In grid_renderer.hpp
class GridRenderer {
public:
    // ... existing methods ...

    // NEW: Render movement path
    void render_path(const RenderConfig& config,
                     const std::vector<BoardPos>& path);

    // NEW: Render hover highlight
    void render_hover(const RenderConfig& config, BoardPos pos);

    // NEW: Render move range with opacity
    void render_move_range_alpha(const RenderConfig& config,
                                  const std::vector<BoardPos>& tiles,
                                  float opacity);

private:
    // NEW: Tile sprite atlas (loaded on init)
    GPUTextureHandle tile_atlas;

    // Sprite rects within atlas
    SDL_Rect corner_rects[5];  // CornerVariant: Solo, Edge1, Edge2, BothEdges, Full
    SDL_Rect path_rects[5];    // PathSegment: Start, Straight, Corner, CornerFlipped, End
};
```

### 10.2 Helper: Add Quad Vertices

```cpp
// In grid_renderer.cpp
void add_quad_vertices(std::vector<ColorVertex>& vertices,
                       const TransformedQuad& quad, SDL_FColor color) {
    // Convert to NDC and add 4 vertices (2 triangles via index buffer)
    // Quad corners: 0=TL, 1=TR, 2=BR, 3=BL
    for (int i = 0; i < 4; i++) {
        vertices.push_back({
            quad.corners[i].x, quad.corners[i].y,
            color.r, color.g, color.b, color.a
        });
    }
}
```

### 10.3 Batched Rendering

Instead of one draw call per corner (wasteful), batch all corners:

```cpp
void GridRenderer::render_move_range_alpha(const RenderConfig& config,
                                            const std::vector<BoardPos>& tiles,
                                            float opacity) {
    if (tiles.empty()) return;

    // Build vertex list for all corners
    std::vector<ColorVertex> vertices;
    vertices.reserve(tiles.size() * 4 * 4);  // 4 corners, 4 verts each

    SDL_FColor color = TileColor::MOVE_CURRENT;
    color.a *= opacity;

    for (const auto& pos : tiles) {
        for (int corner = 0; corner < 4; corner++) {
            auto neighbors = get_corner_neighbors(pos, corner, tiles);
            auto variant = select_variant(neighbors);

            // Get corner screen quad
            auto quad = get_corner_quad(config, pos, corner);

            // Add 4 vertices for this corner
            add_quad_vertices(vertices, quad, color);
        }
    }

    // Single batched draw
    g_gpu.draw_colored_batch(vertices);
}
```

### 10.4 Batched Draw in GPURenderer

Add to `gpu_renderer.hpp`:

```cpp
void draw_colored_batch(const std::vector<ColorVertex>& vertices);
```

Implementation uploads all vertices in one transfer, draws with single call.

---

## 11. GPU Renderer Efficiency

### 11.1 Problem: Current Pattern is Wasteful

Every `draw_*` call currently:
1. Creates transfer buffer
2. Maps memory
3. Copies vertices
4. Ends render pass
5. Begins copy pass
6. Uploads
7. Ends copy pass
8. Begins new render pass
9. Binds pipeline
10. Draws
11. Releases transfer buffer

For 40 tiles × 4 corners = 160 draw calls = 160× overhead.

### 11.2 Solution: Pre-allocated Batch Buffer

```cpp
// In GPURenderer
struct BatchState {
    std::vector<ColorVertex> vertices;
    size_t vertex_count = 0;
    bool batch_active = false;
};
BatchState tile_batch;

void begin_tile_batch();
void add_to_tile_batch(const ColorVertex* verts, size_t count);
void flush_tile_batch();
```

Usage:
```cpp
g_gpu.begin_tile_batch();
for (const auto& pos : tiles) {
    // ... compute vertices ...
    g_gpu.add_to_tile_batch(corner_verts, 4);
}
g_gpu.flush_tile_batch();  // Single upload + draw
```

### 11.3 Vertex Buffer Sizing

Max tiles: 45 (9×5 board)
Max corners: 45 × 4 = 180
Max vertices: 180 × 4 = 720
Max size: 720 × sizeof(ColorVertex) = 720 × 24 = 17,280 bytes

Pre-allocate 20KB buffer — plenty for all tile rendering.

---

## 12. File Changes

### 12.1 Modified Files

```
include/grid_renderer.hpp
    + FadeTarget enum
    + TileFadeAnim struct
    + CornerVariant enum
    + PathSegment enum
    + render_path(), render_hover(), render_move_range_alpha()
    + tile_atlas, corner_rects[], path_rects[]
    + get_corner_quad()

src/grid_renderer.cpp
    + #include <queue>, <unordered_map>, <algorithm>
    + get_path_to()
    + get_corner_neighbors()
    + select_variant()
    + get_corner_quad()
    + select_path_segment()
    + path_segment_rotation()
    + render_path()
    + render_hover()
    + render_move_range_alpha()
    + Batched rendering implementation

include/gpu_renderer.hpp
    + BatchState struct
    + begin_tile_batch(), add_to_tile_batch(), flush_tile_batch()
    + draw_colored_batch()

src/gpu_renderer.cpp
    + Batch buffer management
    + draw_colored_batch() implementation

src/main.cpp
    GameState additions:
        + hover_pos, hover_valid, was_hovering_on_board
        + movement_path
        + move_blob_opacity, attack_blob_opacity
        + tile_anims

    Function additions/modifications:
        + update_hover_state()
        + update_hover_path()
        + start_opacity_fade()
        ~ update() — add hover state update, tile animation update
        ~ clear_selection() — clear path and reset opacities
```

### 12.2 New Assets

```
dist/sprites/tiles/
    tile_corner_solo.png
    tile_corner_edge1.png
    tile_corner_edge2.png
    tile_corner_both.png
    tile_corner_full.png
    path_start.png
    path_straight.png
    path_corner.png
    path_end.png
```

### 12.3 Shader Path

Shaders go to `dist/shaders/` (not `data/shaders/`):
```
dist/shaders/
    tile.vert.spv      — If needed for textured tiles
    tile.frag.spv
```

For initial implementation, reuse existing `color.vert`/`color.frag` for solid tiles.

---

## 13. Implementation Phases

### Phase 1: Foundation (No Visual Change)
- [ ] Add hover tracking to GameState
- [ ] Add `get_path_to()` pathfinding function
- [ ] Add TileFadeAnim and animation update loop
- [ ] Add batch rendering infrastructure to GPURenderer
- [ ] Test: hover_pos updates correctly, path calculation works

### Phase 2: Basic Hover
- [ ] Add `render_hover()` — single tile highlight at mouse
- [ ] Integrate into render loop after attack range
- [ ] Test: white square follows mouse on board

### Phase 3: Path Rendering
- [ ] Add path sprite assets
- [ ] Implement `render_path()` with segment selection
- [ ] Connect hover to path display
- [ ] Test: path shows from selected unit to hover tile

### Phase 4: Blob Dimming
- [ ] Add opacity parameter to `render_move_range()`
- [ ] Implement fade animations
- [ ] Dim blob when hovering valid move target
- [ ] Test: blob dims smoothly, restores on exit

### Phase 5: Merged Tile Corners (Optional)
- [ ] Add corner sprite assets
- [ ] Implement corner neighbor detection
- [ ] Replace solid quads with corner-based rendering
- [ ] Test: blob edges have smooth rounded corners

---

## 14. Testing Checklist

### Coordinate Alignment
- [ ] Hover highlight aligns with grid lines
- [ ] Path segments connect at tile centers
- [ ] At 1x scale (1280×720): tiles align correctly
- [ ] At 2x scale (1920×1080): tiles align correctly

### State Machine
- [ ] Hovering on board sets hover_valid = true
- [ ] Moving off board sets hover_valid = false
- [ ] Transition within board is instant (no flicker)
- [ ] Transition entering board has 0.2s fade

### Path Calculation
- [ ] Path avoids occupied tiles
- [ ] Path finds shortest route (BFS)
- [ ] Path clears when hovering non-reachable tile
- [ ] Path updates instantly when moving within board

### Performance
- [ ] No per-frame allocations during steady state
- [ ] Batch draw reduces draw calls vs current approach
- [ ] Frame time stable with full board highlighted

### Visual Consistency
- [ ] Tile colors match current game (white move, red attack)
- [ ] Shadows render on top of tile highlights (current behavior)
- [ ] HP bars position unchanged
- [ ] Entity sprites position unchanged

---

## 15. Deferred Features

These are documented from Duelyst analysis but NOT implemented in this phase:

### 15.1 Selection Box Pulse
Duelyst pulses selection box scale 0.85–1.0. Defer until selection UX is designed.

### 15.2 Attack Path Arc Animation
Animated projectile arc for ranged attacks. Defer until ranged units exist.

### 15.3 Move/Attack Seam Sprites
Special corner sprites where move and attack blobs meet. Defer until merged corners work.

### 15.4 Duelyst Color Scheme
Yellow attack tiles (#FFD900) instead of red. Defer until art direction decided.

### 15.5 Card Play Tiles
Tile highlights for summoning units. Defer until card system exists.

---

## Appendix A: Duelyst Source Cross-Reference

| This Document | Duelyst Source |
|---------------|----------------|
| §5.2 Colors | `config.js:824-843` |
| §5.3 Opacity | `config.js:528-531` |
| §5.1 Timing | `config.js:550-567` (via entity.hpp) |
| §7 Merged Tiles | `TileLayer.js:213-269` |
| §8 Hover State | `Player.js:1557-1841` |
| §9 Path Segments | `Player.js:2003-2069` |

## Appendix B: Coordinate Transform Reference

```cpp
// EXISTING — DO NOT MODIFY
// types.cpp
Vec2 board_to_screen(config, pos);              // Flat board → screen
Vec2 board_to_screen_perspective(config, pos);  // With 16° rotation
BoardPos screen_to_board(config, screen);       // Inverse flat
BoardPos screen_to_board_perspective(config, screen);  // Inverse with perspective

// grid_renderer.cpp
Vec2 transform_board_point(config, board_x, board_y);  // Uses 16° rotation
```

All tile rendering MUST use `transform_board_point()` to match grid lines.

## Appendix C: Render Order Diagram

```
Frame rendering order (back to front):

1. Clear to background color
2. Grid lines (GridRenderer::render)
3. Move blob tiles (render_move_range_alpha)
4. Attack blob tiles (render_attack_range)
5. Path segments (render_path)
6. Hover tile (render_hover)
─── TILE SYSTEM ENDS ───
7. Entity shadows (Y-sorted)
8. Entity sprites (Y-sorted)
9. Active FX
10. HP bars
11. Floating text
12. Turn indicator / game over overlay
```

Tiles intentionally render BEFORE shadows so shadows partially occlude highlight edges.
