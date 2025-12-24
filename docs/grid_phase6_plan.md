# Phase 6: Duelyst-Accurate Tile Rendering

Complete implementation guide for fixing tile rendering to match Duelyst's visual system.

## Goals

1. **Fix triangle artifacts** — Rotate corner quadrants (TL=0°, TR=90°, BR=180°, BL=270°)
2. **Fix thin movement arrow** — Pre-scale path sprites, add FromStart variants
3. **Add target reticle** — Render glow/reticle at path destination
4. **Add floor grid** — Semi-transparent dark tiles with gaps

## Prerequisites

### Corner/Tile Assets (app/resources/tiles/)
- `tile_merged_large_0.png` (64×64) — isolated corner
- `tile_merged_large_01.png` (64×64) — edge1 neighbor
- `tile_merged_large_03.png` (64×64) — edge2 neighbor
- `tile_merged_large_013.png` (64×64) — both edges
- `tile_merged_large_0123.png` (64×64) — all neighbors
- `tile_board.png` (128×128) — floor tile
- `tile_hover.png` (128×128) — hover highlight

### Path Assets (app/resources/tiles/)
- `tile_path_move_start.png` (128×128, content 71×12)
- `tile_path_move_straight.png` (128×128, content 128×12)
- `tile_path_move_straight_from_start.png` (128×128)
- `tile_path_move_corner.png` (128×128, content 70×70)
- `tile_path_move_corner_from_start.png` (128×128)
- `tile_path_move_end.png` (128×128, content 82×60)
- `tile_path_move_end_from_start.png` (128×128)

### Target Assets
- `tile_glow.png` (128×128) — move target glow
- `tile_large.png` (128×128) — attack target reticle (future use)

---

## Step 1: Asset Pipeline (build_assets.py)

Add tile scaling function to generate resolution-specific sprites.

```python
from PIL import Image
from pathlib import Path

def resize_image(src_path, dst_path, width, height):
    """High-quality image resize using Lanczos filter."""
    img = Image.open(src_path).convert('RGBA')
    resized = img.resize((width, height), Image.LANCZOS)
    resized.save(dst_path)

def generate_scaled_tiles():
    """Generate tile sprites for each scale level."""
    src_dir = Path("app/resources/tiles")

    for scale in [1, 2]:
        dst_dir = Path(f"dist/resources/tiles/s{scale}")
        dst_dir.mkdir(parents=True, exist_ok=True)

        tile_size = 48 * scale      # 48 or 96
        quarter_size = tile_size // 2  # 24 or 48

        # Corner sprites (64×64 source → quarter_size)
        corner_map = {
            'tile_merged_large_0.png': 'corner_0.png',
            'tile_merged_large_01.png': 'corner_01.png',
            'tile_merged_large_03.png': 'corner_03.png',
            'tile_merged_large_013.png': 'corner_013.png',
            'tile_merged_large_0123.png': 'corner_0123.png',
        }
        for src_name, dst_name in corner_map.items():
            src = src_dir / src_name
            if src.exists():
                resize_image(src, dst_dir / dst_name, quarter_size, quarter_size)

        # Floor and hover tiles (128×128 → tile_size)
        for src_name, dst_name in [('tile_board.png', 'floor.png'),
                                    ('tile_hover.png', 'hover.png'),
                                    ('tile_glow.png', 'target.png')]:
            src = src_dir / src_name
            if src.exists():
                resize_image(src, dst_dir / dst_name, tile_size, tile_size)

        # Path sprites (128×128 → tile_size)
        path_map = {
            'tile_path_move_start.png': 'path_start.png',
            'tile_path_move_straight.png': 'path_straight.png',
            'tile_path_move_straight_from_start.png': 'path_straight_from_start.png',
            'tile_path_move_corner.png': 'path_corner.png',
            'tile_path_move_corner_from_start.png': 'path_corner_from_start.png',
            'tile_path_move_end.png': 'path_end.png',
            'tile_path_move_end_from_start.png': 'path_end_from_start.png',
        }
        for src_name, dst_name in path_map.items():
            src = src_dir / src_name
            if src.exists():
                resize_image(src, dst_dir / dst_name, tile_size, tile_size)

        print(f"Generated scale {scale} tiles in {dst_dir}")
```

**Output structure:**
```
dist/resources/tiles/
  s1/                          s2/
    corner_0.png (24×24)         corner_0.png (48×48)
    corner_01.png                corner_01.png
    corner_03.png                corner_03.png
    corner_013.png               corner_013.png
    corner_0123.png              corner_0123.png
    floor.png (48×48)            floor.png (96×96)
    hover.png                    hover.png
    target.png                   target.png
    path_start.png               path_start.png
    path_straight.png            path_straight.png
    path_straight_from_start.png path_straight_from_start.png
    path_corner.png              path_corner.png
    path_corner_from_start.png   path_corner_from_start.png
    path_end.png                 path_end.png
    path_end_from_start.png      path_end_from_start.png
```

---

## Step 2: Header Updates (include/grid_renderer.hpp)

### 2.1 Remove obsolete declarations

Delete:
```cpp
enum class CornerVariant { ... };
CornerVariant select_variant(CornerNeighbors n);
```

### 2.2 Update GridRenderer struct

**Replace** corner and path texture members:

```cpp
struct GridRenderer {
    int fb_width;
    int fb_height;
    PerspectiveConfig persp_config;

    // Scale-specific textures (pre-scaled, no runtime scaling)
    GPUTextureHandle floor_tile;
    GPUTextureHandle hover_tile;
    GPUTextureHandle target_tile;

    // Corner textures (quarter tile size)
    GPUTextureHandle corner_0;
    GPUTextureHandle corner_01;
    GPUTextureHandle corner_03;
    GPUTextureHandle corner_013;
    GPUTextureHandle corner_0123;
    bool corner_textures_loaded = false;

    // Path textures (tile size)
    GPUTextureHandle path_start;
    GPUTextureHandle path_straight;
    GPUTextureHandle path_straight_from_start;
    GPUTextureHandle path_corner;
    GPUTextureHandle path_corner_from_start;
    GPUTextureHandle path_end;
    GPUTextureHandle path_end_from_start;
    bool path_textures_loaded = false;

    bool init(const RenderConfig& config);
    void render(const RenderConfig& config);
    void render_floor_grid(const RenderConfig& config);
    void render_highlight(const RenderConfig& config, BoardPos pos, SDL_FColor color);
    void render_move_range_alpha(const RenderConfig& config,
                                  const std::vector<BoardPos>& tiles, float opacity);
    void render_attack_range(const RenderConfig& config,
                             const std::vector<BoardPos>& attackable_tiles);
    void render_path(const RenderConfig& config, const std::vector<BoardPos>& path);
    void render_hover(const RenderConfig& config, BoardPos pos);
    void render_target(const RenderConfig& config, BoardPos pos);  // NEW

private:
    Vec2 transform_board_point(const RenderConfig& config, float board_x, float board_y);
    void render_path_segment(const RenderConfig& config, BoardPos pos,
                             const GPUTextureHandle& texture, float rotation_deg);
    void render_corner_quad_rotated(const RenderConfig& config, BoardPos pos, int corner,
                                     const GPUTextureHandle& texture, SDL_FColor tint);
    const GPUTextureHandle& get_corner_texture(CornerNeighbors n);
    const GPUTextureHandle& get_path_texture(PathSegment seg, bool from_start);
};
```

---

## Step 3: Implementation (src/grid_renderer.cpp)

### 3.1 Update init() — Load scale-specific assets

**Replace** the entire init function:

```cpp
bool GridRenderer::init(const RenderConfig& config) {
    int ts = config.tile_size();
    fb_width = BOARD_COLS * ts;
    fb_height = BOARD_ROWS * ts;
    persp_config = PerspectiveConfig::for_board(config);

    std::string prefix = "dist/resources/tiles/s" + std::to_string(config.scale) + "/";

    // Floor and special tiles
    floor_tile = g_gpu.load_texture((prefix + "floor.png").c_str());
    hover_tile = g_gpu.load_texture((prefix + "hover.png").c_str());
    target_tile = g_gpu.load_texture((prefix + "target.png").c_str());

    // Corner tiles
    corner_0 = g_gpu.load_texture((prefix + "corner_0.png").c_str());
    corner_01 = g_gpu.load_texture((prefix + "corner_01.png").c_str());
    corner_03 = g_gpu.load_texture((prefix + "corner_03.png").c_str());
    corner_013 = g_gpu.load_texture((prefix + "corner_013.png").c_str());
    corner_0123 = g_gpu.load_texture((prefix + "corner_0123.png").c_str());
    corner_textures_loaded = corner_0 && corner_01 && corner_03 &&
                              corner_013 && corner_0123;

    // Path tiles
    path_start = g_gpu.load_texture((prefix + "path_start.png").c_str());
    path_straight = g_gpu.load_texture((prefix + "path_straight.png").c_str());
    path_straight_from_start = g_gpu.load_texture((prefix + "path_straight_from_start.png").c_str());
    path_corner = g_gpu.load_texture((prefix + "path_corner.png").c_str());
    path_corner_from_start = g_gpu.load_texture((prefix + "path_corner_from_start.png").c_str());
    path_end = g_gpu.load_texture((prefix + "path_end.png").c_str());
    path_end_from_start = g_gpu.load_texture((prefix + "path_end_from_start.png").c_str());
    path_textures_loaded = path_start && path_straight && path_end;

    SDL_Log("Grid renderer initialized: scale=%d, corners=%s, paths=%s",
            config.scale,
            corner_textures_loaded ? "OK" : "MISSING",
            path_textures_loaded ? "OK" : "MISSING");
    return true;
}
```

### 3.2 Add render_floor_grid()

```cpp
void GridRenderer::render_floor_grid(const RenderConfig& config) {
    if (!floor_tile) return;

    int ts = config.tile_size();
    SDL_FColor floor_tint = {0.0f, 0.0f, 0.0f, 0.08f};  // 8% opacity black

    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            float tx = static_cast<float>(x * ts);
            float ty = static_cast<float>(y * ts);

            Vec2 tl = transform_board_point(config, tx, ty);
            Vec2 tr = transform_board_point(config, tx + ts, ty);
            Vec2 br = transform_board_point(config, tx + ts, ty + ts);
            Vec2 bl = transform_board_point(config, tx, ty + ts);

            SDL_FRect src = {0, 0, (float)floor_tile.width, (float)floor_tile.height};
            g_gpu.draw_sprite_transformed(floor_tile, src, tl, tr, br, bl, floor_tint.a);
        }
    }
}
```

### 3.3 Add get_corner_texture()

```cpp
const GPUTextureHandle& GridRenderer::get_corner_texture(CornerNeighbors n) {
    if (!n.edge1 && !n.edge2) return corner_0;
    if (n.edge1 && !n.edge2) return corner_01;
    if (!n.edge1 && n.edge2) return corner_03;
    if (n.edge1 && n.edge2 && !n.diagonal) return corner_013;
    return corner_0123;
}
```

### 3.4 Replace render_corner_quad with render_corner_quad_rotated

**Delete** old `render_corner_quad`, **add**:

```cpp
void GridRenderer::render_corner_quad_rotated(const RenderConfig& config, BoardPos pos,
                                               int corner, const GPUTextureHandle& texture,
                                               SDL_FColor tint) {
    int ts = config.tile_size();
    int half = ts / 2;

    // Corner base position: 0=TL, 1=TR, 2=BR, 3=BL
    float base_x = pos.x * ts + ((corner == 1 || corner == 2) ? half : 0);
    float base_y = pos.y * ts + ((corner == 2 || corner == 3) ? half : 0);

    // Center of quarter-tile
    float cx = base_x + half * 0.5f;
    float cy = base_y + half * 0.5f;

    // Rotation: TL=0°, TR=90°, BR=180°, BL=270°
    float rotation_deg = corner * 90.0f;
    float rad = rotation_deg * (M_PI / 180.0f);
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);
    float h = half * 0.5f;

    auto rotate = [&](float rx, float ry) -> Vec2 {
        return {cx + rx * cos_r - ry * sin_r, cy + rx * sin_r + ry * cos_r};
    };

    Vec2 tl_b = rotate(-h, -h);
    Vec2 tr_b = rotate(h, -h);
    Vec2 br_b = rotate(h, h);
    Vec2 bl_b = rotate(-h, h);

    Vec2 tl = transform_board_point(config, tl_b.x, tl_b.y);
    Vec2 tr = transform_board_point(config, tr_b.x, tr_b.y);
    Vec2 br = transform_board_point(config, br_b.x, br_b.y);
    Vec2 bl = transform_board_point(config, bl_b.x, bl_b.y);

    SDL_FRect src = {0, 0, (float)texture.width, (float)texture.height};
    g_gpu.draw_sprite_transformed(texture, src, tl, tr, br, bl, tint.a);
}
```

### 3.5 Replace render_move_range_alpha

```cpp
void GridRenderer::render_move_range_alpha(const RenderConfig& config,
                                            const std::vector<BoardPos>& tiles,
                                            float opacity) {
    if (tiles.empty()) return;

    SDL_FColor color = TileColor::MOVE_CURRENT;
    color.a *= opacity;

    if (!corner_textures_loaded) {
        for (const auto& pos : tiles) {
            render_highlight(config, pos, color);
        }
        return;
    }

    for (const auto& pos : tiles) {
        for (int corner = 0; corner < 4; corner++) {
            CornerNeighbors neighbors = get_corner_neighbors(pos, corner, tiles);
            const GPUTextureHandle& tex = get_corner_texture(neighbors);
            render_corner_quad_rotated(config, pos, corner, tex, color);
        }
    }
}
```

### 3.6 Add get_path_texture()

```cpp
const GPUTextureHandle& GridRenderer::get_path_texture(PathSegment seg, bool from_start) {
    switch (seg) {
        case PathSegment::Start:
            return path_start;
        case PathSegment::Straight:
            return (from_start && path_straight_from_start) ? path_straight_from_start : path_straight;
        case PathSegment::Corner:
        case PathSegment::CornerFlipped:
            return (from_start && path_corner_from_start) ? path_corner_from_start : path_corner;
        case PathSegment::End:
            return (from_start && path_end_from_start) ? path_end_from_start : path_end;
        default:
            return path_straight;
    }
}
```

### 3.7 Replace render_path

```cpp
void GridRenderer::render_path(const RenderConfig& config,
                                const std::vector<BoardPos>& path) {
    if (path.size() < 2) return;

    if (!path_textures_loaded) {
        for (size_t i = 1; i < path.size(); i++) {
            render_highlight(config, path[i], TileColor::PATH);
        }
        return;
    }

    for (size_t i = 0; i < path.size(); i++) {
        PathSegment seg = select_path_segment(path, i);
        bool from_start = (i == 1);  // Second tile uses FromStart variant

        float rotation = 0.0f;
        if (i == 0) {
            rotation = path_segment_rotation(path[0], path[1]);
        } else if (i == path.size() - 1) {
            rotation = path_segment_rotation(path[i-1], path[i]);
            from_start = (path.size() == 2);  // End uses FromStart if path length = 2
        } else {
            rotation = path_segment_rotation(path[i-1], path[i]);
        }

        if (seg == PathSegment::CornerFlipped) {
            rotation += 90.0f;
        }

        const GPUTextureHandle& texture = get_path_texture(seg, from_start);
        if (texture) {
            render_path_segment(config, path[i], texture, rotation);
        }
    }
}
```

### 3.8 Replace render_hover

```cpp
void GridRenderer::render_hover(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;

    if (hover_tile) {
        int ts = config.tile_size();
        float tx = static_cast<float>(pos.x * ts);
        float ty = static_cast<float>(pos.y * ts);

        Vec2 tl = transform_board_point(config, tx, ty);
        Vec2 tr = transform_board_point(config, tx + ts, ty);
        Vec2 br = transform_board_point(config, tx + ts, ty + ts);
        Vec2 bl = transform_board_point(config, tx, ty + ts);

        SDL_FRect src = {0, 0, (float)hover_tile.width, (float)hover_tile.height};
        g_gpu.draw_sprite_transformed(hover_tile, src, tl, tr, br, bl, TileColor::HOVER.a);
    } else {
        render_highlight(config, pos, TileColor::HOVER);
    }
}
```

### 3.9 Add render_target

```cpp
void GridRenderer::render_target(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid() || !target_tile) return;

    int ts = config.tile_size();
    float tx = static_cast<float>(pos.x * ts);
    float ty = static_cast<float>(pos.y * ts);

    Vec2 tl = transform_board_point(config, tx, ty);
    Vec2 tr = transform_board_point(config, tx + ts, ty);
    Vec2 br = transform_board_point(config, tx + ts, ty + ts);
    Vec2 bl = transform_board_point(config, tx, ty + ts);

    SDL_FRect src = {0, 0, (float)target_tile.width, (float)target_tile.height};
    g_gpu.draw_sprite_transformed(target_tile, src, tl, tr, br, bl, 0.8f);  // 80% opacity
}
```

### 3.10 Delete select_variant()

Remove the `select_variant()` function (replaced by `get_corner_texture()`).

---

## Step 4: Main.cpp Updates

### 4.1 Update render_single_pass()

Add floor grid and target tile:

```cpp
void render_single_pass(GameState& state, const RenderConfig& config) {
    // 1. Floor grid (semi-transparent dark tiles with gaps)
    state.grid_renderer.render_floor_grid(config);

    // 2. Grid lines
    state.grid_renderer.render(config);

    // 3. Selection highlights
    if (state.selected_unit_idx >= 0 && state.game_phase == GamePhase::Playing) {
        state.grid_renderer.render_move_range_alpha(config,
            state.reachable_tiles, state.move_blob_opacity);
        state.grid_renderer.render_attack_range(config, state.attackable_tiles);

        // Movement path
        if (!state.movement_path.empty()) {
            state.grid_renderer.render_path(config, state.movement_path);

            // Target reticle at path destination
            state.grid_renderer.render_target(config, state.movement_path.back());
        }
    }

    // 4. Hover highlight
    if (state.hover_valid) {
        state.grid_renderer.render_hover(config, state.hover_pos);
    }

    // 5. Units (existing code)
    // ...
}
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `build_assets.py` | Add `resize_image()`, `generate_scaled_tiles()` |
| `include/grid_renderer.hpp` | Update texture members, add methods |
| `src/grid_renderer.cpp` | Replace init, add rotated corners, update path rendering |
| `src/main.cpp` | Add floor grid and target tile to render order |

---

## Validation Checklist

### Asset Generation
- [ ] `./build.fish build` creates `dist/resources/tiles/s1/` and `s2/`
- [ ] Corner sprites: 24×24 (s1) and 48×48 (s2)
- [ ] Path sprites: 48×48 (s1) and 96×96 (s2)
- [ ] Floor/hover/target tiles: correct sizes

### Corner Rendering (Triangle Artifacts Fix)
- [ ] Movement blob has smooth rounded exterior corners
- [ ] Interior corners connect seamlessly
- [ ] Gradient flows outward from blob interior (rotation correct)
- [ ] No visible triangular seams between corner quadrants

### Path Rendering (Thin Arrow Fix)
- [ ] Arrow is visually prominent (not thin)
- [ ] Arrow head is properly sized
- [ ] FromStart variants connect smoothly to unit position
- [ ] Corners rotate correctly for L-shaped paths

### Selection Box (Bracket-Corner Reticle)
- [x] Selection box appears at selected unit position
- [x] Selection box appears at path destination when hovering valid move
- [x] Uses `tile_box.png` (bracket corners), not `tile_glow.png`
- [ ] **TODO:** Align selection box size/padding with grid tile (currently uses source aspect ratio)

### Floor Grid
- [ ] Semi-transparent dark tiles visible
- [ ] Visible gaps between tiles (baked into sprite)

### Preservation
- [ ] Unit sprites unchanged (integer scaling)
- [ ] Perspective unchanged (16° board tilt)
- [ ] Shadows unchanged (19.5 offset)

---

## Post-Implementation Fixes (2024-12-24)

### Bug Fix: Movement Blob Visual Continuity
**Issue:** Tile under selected unit was excluded from the contiguous movement blob.

**Root Cause:** `reachable_tiles` only contains valid move destinations, not the unit's current position.

**Fix:** `main.cpp:1145-1148` now includes unit position in blob tiles:
```cpp
BoardPos unit_pos = state.units[state.selected_unit_idx].board_pos;
std::vector<BoardPos> blob_tiles = state.reachable_tiles;
blob_tiles.push_back(unit_pos);
```

### Bug Fix: Selection Box Sprite
**Issue:** Target reticle used `tile_glow.png` (a glow effect) instead of `tile_box.png` (bracket-corner selection box).

**Ground Truth:** From `tile_interaction_flow.md` §1.2, `TileBoxSprite` uses `tile_box.png` for the "Selected unit box outline" with bracket corners.

**Fix:**
1. `build_assets.py`: Changed `tile_glow.png` → `tile_box.png` for `select_box.png`
2. `grid_renderer.hpp/cpp`: Renamed `target_tile` → `select_box`, function `render_target` → `render_select_box`
3. `main.cpp:1154-1162`: Renders selection box at both unit position and hover destination

### Known Issue: Selection Box Alignment
**Status:** Not yet fixed.

**Problem:** Selection box uses source sprite aspect ratio (80×80) scaled to tile size, but may need padding/sizing adjustments to align properly with grid tiles.

**Expected:** Selection box corners should align with tile boundaries per Duelyst's `TileBoxSprite` behavior.
