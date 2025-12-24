# Phase 7 Transition: Grid Feature Parity

Implementation guide to bring Sylvanshine's grid rendering to Duelyst feature parity.

**Scope:** Attack system, enemy hover indicator, seam sprites, z-order constants.
**Deferred:** Bloom, card tiles, attack arc animation.

---

## API Constraints

**Sprite rendering:** `draw_sprite_transformed(texture, src, corners, opacity)` — opacity only, no color tint.
**Colored quads:** `draw_quad_transformed(corners, color)` — solid color, no texture.
**Tinted sprites:** NOT SUPPORTED. Use `render_highlight()` for colored overlays.

---

## Executive Summary

### What Needs to Change

| # | Task | Complexity | Dependencies |
|---|------|------------|--------------|
| 0 | Load `corner_0_seam.png` texture | Trivial | None |
| 1 | Add z-order constants | Trivial | None |
| 2 | Add enemy hover indicator | Low | Asset pipeline |
| 3 | Refactor attack to two-phase system | Medium | Asset pipeline |
| 4 | Implement seam detection | Medium | Task 0, Task 3 |

### Critical Discoveries

1. **Attack system conflation:** Sylvanshine's `attackable_tiles` = enemy positions only. Duelyst has TWO concepts:
   - Attack Pattern (yellow blob of all attack-reach tiles)
   - Attackable Targets (reticles on actual enemies)

2. **Attack reticles don't pulse:** Only selection boxes pulse. Reticles are static.

3. **Enemy indicator is hover-only:** Duelyst shows it during passive hover, not permanently.

4. **Seam texture exists but isn't loaded:** `corner_0_seam.png` is generated but never loaded.

---

## Reference: Duelyst Constants

### Tile Sprites

| Sprite | File | Size | Purpose | Pulses |
|--------|------|------|---------|--------|
| Selection Box | `tile_box.png` | 80×80 | On selected unit | Yes |
| Glow Tile | `tile_glow.png` | 100×100 | Under move destination | No |
| Target Tile | `tile_target.png` | 128×128 | Move destination | Yes |
| Attack Reticle | `tile_large.png` | 128×128 | On attackable enemies | **No** |
| Enemy Indicator | `tile_opponent.png` | 128×128 | Hover over enemy | No |

### Z-Order (`Player.js:48-56`)

```
1 = boardTileZOrder        (floor, enemy indicator)
2 = moveTileZOrder         (movement blob)
3 = assistTileZOrder       (future)
4 = aggroTileZOrder        (attack blob)
5 = attackableTargetTileZOrder  (attack reticles)
6 = cardTileZOrder         (future)
7 = selectTileZOrder       (selection box)
8 = pathTileZOrder         (movement path)
9 = mouseOverTileZOrder    (hover)
```

### Opacity Values (`config.js`)

| Constant | Value | Decimal |
|----------|-------|---------|
| `TILE_SELECT_OPACITY` | 200 | 78% |
| `TILE_FAINT_OPACITY` | 75 | 29% |
| `PATH_TILE_ACTIVE_OPACITY` | 150 | 59% |

### Sylvanshine Asset Status

| Duelyst Asset | Sylvanshine | Status |
|---------------|-------------|--------|
| `tile_box.png` | `select_box.png` | ✓ Loaded |
| `tile_glow.png` | `glow.png` | ✓ Loaded |
| `tile_target.png` | `target.png` | ✓ Loaded |
| `tile_large.png` | — | **Missing from pipeline** |
| `tile_opponent.png` | — | **Missing from pipeline** |
| `tile_merged_large_0_seam.png` | `corner_0_seam.png` | Generated, **not loaded** |

---

## Task 0: Load Seam Texture (Prerequisite)

**Problem:** `corner_0_seam.png` exists in `dist/` but isn't loaded.

### Step 0.1: Add texture handle

`grid_renderer.hpp` — find `GPUTextureHandle corner_0123;` and add ONE LINE after it:
```cpp
    GPUTextureHandle corner_0_seam;  // Seam texture for blob boundaries
```

### Step 0.2: Load texture

`grid_renderer.cpp` — find `corner_0123 = g_gpu.load_texture` and add ONE LINE after it:
```cpp
    corner_0_seam = g_gpu.load_texture((prefix + "corner_0_seam.png").c_str());
```

### Step 0.3: Update validation

`grid_renderer.cpp` — find `corner_textures_loaded = corner_0 && corner_01` and update:
```cpp
    corner_textures_loaded = corner_0 && corner_01 && corner_03 &&
                              corner_013 && corner_0123 && corner_0_seam;  // ADD corner_0_seam
```

---

## Task 1: Z-Order Constants

**Goal:** Document render order with explicit constants.

`grid_renderer.hpp` — add after the `TileOpacity` namespace (around line 49):
```cpp
namespace TileZOrder {
    constexpr int BOARD = 1;
    constexpr int MOVE_RANGE = 2;
    constexpr int ASSIST = 3;
    constexpr int ATTACK_RANGE = 4;
    constexpr int ATTACKABLE_TARGET = 5;
    constexpr int CARD = 6;
    constexpr int SELECT = 7;
    constexpr int PATH = 8;
    constexpr int HOVER = 9;
}
```

`main.cpp` — find `void render_single_pass(` and add comment inside at the start of the function body:
```cpp
void render_single_pass(GameState& state, const RenderConfig& config) {
    // Render order (back to front) — matches TileZOrder
    // 1. BOARD:             render_floor_grid()
    // 2. MOVE_RANGE:        render_move_range_alpha()
    // 4. ATTACK_RANGE:      render_attack_blob()
    // 5. ATTACKABLE_TARGET: render_attack_reticle()
    // 7. SELECT:            render_select_box()
    // 8. PATH:              render_path()
    // 9. HOVER:             render_hover()

    // 1. Floor grid...
```

---

## Task 2: Enemy Hover Indicator

**Duelyst behavior:** Show `tile_opponent.png` only during passive hover over enemies (no unit selected).

### Step 2.1: Asset Pipeline

`build_assets.py` — find `path_map.items():` loop (around line 685) and add BEFORE it:
```python
        # Enemy indicator tile
        src = src_dir / 'tile_opponent.png'
        dst = dst_dir / 'enemy_indicator.png'
        if src.exists():
            if output_files is not None:
                output_files.add(dst)
            if force or needs_regeneration(src, dst):
                resize_image(src, dst, tile_size, tile_size)
                generated += 1

        # Path tiles (existing code follows...)
        path_map = {
```

### Step 2.2: Renderer

**Note:** API only supports opacity on sprites, not color tint. Use colored quad overlay.

`grid_renderer.hpp` — find `GPUTextureHandle target_tile;` and add ONE LINE after it:
```cpp
    GPUTextureHandle enemy_indicator;  // Red overlay for enemy hover
```

`grid_renderer.hpp` — find `void render_target(` declaration and add after it:
```cpp
    void render_enemy_indicator(const RenderConfig& config, BoardPos pos);
```

`grid_renderer.cpp` — find `target_tile = g_gpu.load_texture` and add ONE LINE after it:
```cpp
    enemy_indicator = g_gpu.load_texture((prefix + "enemy_indicator.png").c_str());
```

`grid_renderer.cpp` — add after the closing brace of `render_target()` function:
```cpp
void GridRenderer::render_enemy_indicator(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;
    // Red overlay using existing highlight system (API doesn't support tinted sprites)
    constexpr SDL_FColor ENEMY_COLOR = {0.824f, 0.157f, 0.275f, 75.0f/255.0f};  // #D22846, 29%
    render_highlight(config, pos, ENEMY_COLOR);
}
```

### Step 2.3: Integration

`main.cpp` — find `// 4. Hover highlight` and REPLACE the entire hover block:

**BEFORE:**
```cpp
    // 4. Hover highlight
    if (state.hover_valid) {
        state.grid_renderer.render_hover(config, state.hover_pos);
    }
```

**AFTER:**
```cpp
    // 4. Hover highlight
    if (state.hover_valid) {
        // Enemy indicator (hover-only, Duelyst-faithful) — only when no unit selected
        if (state.selected_unit_idx < 0) {
            for (const auto& unit : state.units) {
                if (unit.type == UnitType::Enemy && unit.board_pos == state.hover_pos) {
                    state.grid_renderer.render_enemy_indicator(config, unit.board_pos);
                    break;
                }
            }
        }
        state.grid_renderer.render_hover(config, state.hover_pos);
    }
```

**Note:** Uses `unit.type == UnitType::Enemy` (defined in `entity.hpp`).

---

## Task 3: Attack System Refactor

**Problem:** Sylvanshine only renders attackable targets (enemies). Duelyst renders:
1. **Attack blob** (z=4): Yellow tiles showing full attack reach
2. **Attack reticles** (z=5): `tile_large.png` on actual enemies

### Step 3.1: Asset Pipeline

`build_assets.py` — find the enemy indicator code you added in Task 2, and add AFTER it:
```python
        # Attack reticle tile
        src = src_dir / 'tile_large.png'
        dst = dst_dir / 'attack_reticle.png'
        if src.exists():
            if output_files is not None:
                output_files.add(dst)
            if force or needs_regeneration(src, dst):
                resize_image(src, dst, tile_size, tile_size)
                generated += 1
```

### Step 3.2: Attack Pattern Generator

`grid_renderer.hpp` — find `std::vector<BoardPos> get_attackable_tiles(` and add after it:
```cpp
std::vector<BoardPos> get_attack_pattern(BoardPos from, int range);
```

`grid_renderer.cpp` — find `get_attackable_tiles` function and add after its closing brace:
```cpp
std::vector<BoardPos> get_attack_pattern(BoardPos from, int range) {
    std::vector<BoardPos> result;
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            if (x == from.x && y == from.y) continue;
            int dist = std::max(std::abs(x - from.x), std::abs(y - from.y));  // Chebyshev
            if (dist <= range) result.push_back({x, y});
        }
    }
    return result;
}
```

### Step 3.3: Attack Reticle Renderer

`grid_renderer.hpp` — find `GPUTextureHandle enemy_indicator;` (added in Task 2) and add after it:
```cpp
    GPUTextureHandle attack_reticle;  // tile_large.png for attackable enemies
```

`grid_renderer.hpp` — find `void render_enemy_indicator(` and add after it:
```cpp
    void render_attack_reticle(const RenderConfig& config, BoardPos pos,
                               float opacity = 200.0f/255.0f);  // Static, no pulsing!
```

`grid_renderer.cpp` — find `enemy_indicator = g_gpu.load_texture` (added in Task 2) and add after it:
```cpp
    attack_reticle = g_gpu.load_texture((prefix + "attack_reticle.png").c_str());
```

`grid_renderer.cpp` — add after `render_enemy_indicator()` function:
```cpp
void GridRenderer::render_attack_reticle(const RenderConfig& config, BoardPos pos, float opacity) {
    if (!pos.is_valid() || !attack_reticle) return;

    int ts = config.tile_size();
    float tx = static_cast<float>(pos.x * ts);
    float ty = static_cast<float>(pos.y * ts);

    Vec2 tl = transform_board_point(config, tx, ty);
    Vec2 tr = transform_board_point(config, tx + ts, ty);
    Vec2 br = transform_board_point(config, tx + ts, ty + ts);
    Vec2 bl = transform_board_point(config, tx, ty + ts);

    SDL_FRect src = {0, 0, (float)attack_reticle.width, (float)attack_reticle.height};
    g_gpu.draw_sprite_transformed(attack_reticle, src, tl, tr, br, bl, opacity);
}
```

### Step 3.4: Attack Blob Renderer

**Note:** This is an initial version. Task 4 will update the signature to add seam detection support.

`grid_renderer.hpp` — find `void render_attack_reticle(` (just added) and add after it:
```cpp
    void render_attack_blob(const RenderConfig& config,
                            const std::vector<BoardPos>& tiles,
                            float opacity = 200.0f/255.0f);
```

`grid_renderer.cpp` — add after `render_attack_reticle()` function:
```cpp
void GridRenderer::render_attack_blob(const RenderConfig& config,
                                       const std::vector<BoardPos>& tiles,
                                       float opacity) {
    if (tiles.empty()) return;

    SDL_FColor color = TileColor::ATTACK_CURRENT;  // Yellow #FFD900
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

### Step 3.5: Integration — Replace Attack Rendering

`main.cpp` — find `render_attack_range(config, state.attackable_tiles)` and REPLACE that single line:

**BEFORE:**
```cpp
        state.grid_renderer.render_attack_range(config, state.attackable_tiles);
```

**AFTER:**
```cpp
        // Two-phase attack system (Duelyst-faithful)
        const auto& unit = state.units[state.selected_unit_idx];
        auto attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

        // Remove enemy positions from blob (they get reticles instead)
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
                   != state.attackable_tiles.end();
        });

        // Remove tiles overlapping with move blob (avoid z-fighting)
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
        });

        // Render attack blob (z=4) then reticles (z=5)
        state.grid_renderer.render_attack_blob(config, attack_blob);
        for (const auto& target : state.attackable_tiles) {
            state.grid_renderer.render_attack_reticle(config, target);
        }
```

**Required include** at top of `main.cpp` (if not present):
```cpp
#include "grid_renderer.hpp"  // For get_attack_pattern()
```

**Note:** Uses `std::erase_if` which requires C++20 (already enabled in this project via CMakeLists.txt).

---

## Task 4: Seam Detection

**Prerequisite:** Task 0 (load seam texture), Task 3 (attack blob exists).

**Goal:** Show visible boundary line where move blob meets attack blob.

### Step 4.1: Add Seam Detection Helper

`grid_renderer.cpp` — add before `get_corner_neighbors()` function:
```cpp
// Check if a corner needs seam texture (boundary between two blobs)
static bool needs_seam_at_corner(BoardPos pos, int corner,
                                  const std::vector<BoardPos>& current_blob,
                                  const std::vector<BoardPos>& alt_blob) {
    if (alt_blob.empty()) return false;

    // Offset patterns for each corner (same as get_corner_neighbors)
    constexpr int offsets[4][3][2] = {
        {{-1, 0}, {-1,-1}, { 0,-1}},  // TL
        {{ 0,-1}, { 1,-1}, { 1, 0}},  // TR
        {{ 1, 0}, { 1, 1}, { 0, 1}},  // BR
        {{ 0, 1}, {-1, 1}, {-1, 0}},  // BL
    };

    auto in_blob = [](BoardPos p, const std::vector<BoardPos>& blob) {
        return std::find(blob.begin(), blob.end(), p) != blob.end();
    };

    // Check for same-blob neighbor (if found, no seam needed)
    for (int i = 0; i < 3; i++) {
        BoardPos neighbor = {pos.x + offsets[corner][i][0], pos.y + offsets[corner][i][1]};
        if (in_blob(neighbor, current_blob)) return false;
    }

    // Check for alt-blob neighbor (if found, seam needed)
    for (int i = 0; i < 3; i++) {
        BoardPos neighbor = {pos.x + offsets[corner][i][0], pos.y + offsets[corner][i][1]};
        if (in_blob(neighbor, alt_blob)) return true;
    }

    return false;
}
```

### Step 4.2: Update get_corner_texture

`grid_renderer.hpp` — find `const GPUTextureHandle& get_corner_texture(CornerNeighbors n);` and REPLACE:
```cpp
    const GPUTextureHandle& get_corner_texture(CornerNeighbors n, bool use_seam = false);
```

`grid_renderer.cpp` — find `GridRenderer::get_corner_texture` function and REPLACE:
```cpp
const GPUTextureHandle& GridRenderer::get_corner_texture(CornerNeighbors n, bool use_seam) {
    if (use_seam && corner_0_seam) return corner_0_seam;
    if (!n.edge1 && !n.edge2) return corner_0;
    if (n.edge1 && !n.edge2) return corner_01;
    if (!n.edge1 && n.edge2) return corner_03;
    if (n.edge1 && n.edge2 && !n.diagonal) return corner_013;
    return corner_0123;
}
```

### Step 4.3: Update render_move_range_alpha Signature

`grid_renderer.hpp` — find `void render_move_range_alpha(` and REPLACE:
```cpp
    void render_move_range_alpha(const RenderConfig& config,
                                  const std::vector<BoardPos>& tiles,
                                  float opacity,
                                  const std::vector<BoardPos>& alt_blob = {});  // Optional for seam detection
```

`grid_renderer.cpp` — find `GridRenderer::render_move_range_alpha` and REPLACE entire function:
```cpp
void GridRenderer::render_move_range_alpha(const RenderConfig& config,
                                            const std::vector<BoardPos>& tiles,
                                            float opacity,
                                            const std::vector<BoardPos>& alt_blob) {
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
            bool seam = needs_seam_at_corner(pos, corner, tiles, alt_blob);
            const GPUTextureHandle& tex = get_corner_texture(neighbors, seam);
            render_corner_quad_rotated(config, pos, corner, tex, color);
        }
    }
}
```

### Step 4.4: Update render_attack_blob Signature

`grid_renderer.hpp` — find `void render_attack_blob(` (added in Task 3) and REPLACE:
```cpp
    void render_attack_blob(const RenderConfig& config,
                            const std::vector<BoardPos>& tiles,
                            float opacity = 200.0f/255.0f,
                            const std::vector<BoardPos>& alt_blob = {});  // Optional for seam detection
```

`grid_renderer.cpp` — find `GridRenderer::render_attack_blob` (added in Task 3) and REPLACE:
```cpp
void GridRenderer::render_attack_blob(const RenderConfig& config,
                                       const std::vector<BoardPos>& tiles,
                                       float opacity,
                                       const std::vector<BoardPos>& alt_blob) {
    if (tiles.empty()) return;

    SDL_FColor color = TileColor::ATTACK_CURRENT;  // Yellow #FFD900
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
            bool seam = needs_seam_at_corner(pos, corner, tiles, alt_blob);
            const GPUTextureHandle& tex = get_corner_texture(neighbors, seam);
            render_corner_quad_rotated(config, pos, corner, tex, color);
        }
    }
}
```

### Step 4.5: Update Call Sites in main.cpp

`main.cpp` — find `render_move_range_alpha(config, blob_tiles, state.move_blob_opacity)` and update:

**BEFORE:**
```cpp
        state.grid_renderer.render_move_range_alpha(config,
            blob_tiles, state.move_blob_opacity);
```

**AFTER:**
```cpp
        state.grid_renderer.render_move_range_alpha(config,
            blob_tiles, state.move_blob_opacity, attack_blob);  // Pass attack_blob for seam detection
```

`main.cpp` — find `render_attack_blob(config, attack_blob)` (added in Task 3) and update:

**BEFORE:**
```cpp
        state.grid_renderer.render_attack_blob(config, attack_blob);
```

**AFTER:**
```cpp
        state.grid_renderer.render_attack_blob(config, attack_blob, 200.0f/255.0f, blob_tiles);  // Pass move_blob for seam detection
```

**IMPORTANT:** The `attack_blob` variable must be computed BEFORE calling `render_move_range_alpha`. Reorder Task 3's code so `attack_blob` is calculated first, then both render calls can use it.

### Step 4.6: Reorder Integration Code

`main.cpp` — the full integration block should now be:
```cpp
        // Calculate attack blob FIRST (needed for seam detection in both renderers)
        const auto& unit = state.units[state.selected_unit_idx];
        auto attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

        // Remove enemy positions (they get reticles)
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
                   != state.attackable_tiles.end();
        });

        // Remove tiles overlapping with move blob
        std::erase_if(attack_blob, [&](const BoardPos& p) {
            return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
        });

        // Render move blob (z=2) with seam detection
        state.grid_renderer.render_move_range_alpha(config,
            blob_tiles, state.move_blob_opacity, attack_blob);

        // Render attack blob (z=4) with seam detection
        state.grid_renderer.render_attack_blob(config, attack_blob, 200.0f/255.0f, blob_tiles);

        // Render attack reticles (z=5)
        for (const auto& target : state.attackable_tiles) {
            state.grid_renderer.render_attack_reticle(config, target);
        }
```

---

## Validation Checklist

### Task 0: Seam Texture
- [ ] `corner_0_seam` member added to GridRenderer
- [ ] Texture loads without error

### Task 1: Z-Order
- [ ] `TileZOrder` namespace in header
- [ ] Render order comment in main.cpp

### Task 2: Enemy Indicator
- [ ] Asset in pipeline
- [ ] Shows ONLY during passive hover (no selection)
- [ ] Red color (#D22846), 29% opacity
- [ ] Does NOT show under player units

### Task 3: Attack System
- [ ] Attack reticle asset in pipeline
- [ ] Yellow blob shows attack reach (excludes enemy positions)
- [ ] Blob excludes tiles overlapping move blob
- [ ] Reticles on actual enemies (static, no pulse)
- [ ] Reticles render above blob (z=5 > z=4)

### Task 4: Seams
- [ ] Visible line at move/attack boundary
- [ ] No false seams in isolated blobs

---

## Files Modified

| File | Tasks | Changes |
|------|-------|---------|
| `grid_renderer.hpp` | 0,1,2,3,4 | `corner_0_seam`, `TileZOrder`, `enemy_indicator`, `attack_reticle`, function signatures |
| `grid_renderer.cpp` | 0,2,3,4 | Texture loads, `render_enemy_indicator`, `render_attack_reticle`, `render_attack_blob`, `needs_seam_at_corner` |
| `build_assets.py` | 2,3 | Add `enemy_indicator.png`, `attack_reticle.png` |
| `main.cpp` | 1,2,3,4 | Render order comment, enemy hover logic, two-phase attack with seam detection |

---

## Existing Systems (Do Not Duplicate)

### Pulsing
- State: `GameState::target_pulse_phase` (`main.cpp:93`)
- Update: `target_pulse_phase += dt / 0.7f` (`main.cpp:1018`)
- Formula: `0.85 + 0.15 * (0.5 + 0.5 * cos(phase * 2π))`
- Used for: destination select box ONLY (not attack reticles)

### Corner Blob System
- `render_move_range_alpha()` already uses corner tiles
- `render_attack_blob()` should use identical approach with different color
