# Tile Highlighting Bugs Analysis

**Date:** 2024-12-24
**Status:** FIXED

## Bug Reports

**Bug 1:** When player unit moves next to enemy unit:
- Enemy tile becomes highlighted white instead of yellow
- Other tiles adjacent to player become highlighted yellow instead of nohighlight
- Player tile is highlighted white (unknown if correct)

**Bug 2:** On new turn, enemy tiles in player move range are highlighted white instead of yellow.

---

## Duelyst Ground Truth

### Color Constants (app/common/config.js:824-843)

| Constant | RGB | Hex | Usage |
|----------|-----|-----|-------|
| `CONFIG.MOVE_COLOR` | 240,240,240 | #F0F0F0 | Movement range blob |
| `CONFIG.AGGRO_COLOR` | 255,217,0 | **#FFD900** | Attack range (friendly) |
| `CONFIG.AGGRO_OPPONENT_COLOR` | 210,40,70 | #D22846 | Enemy selection |

### Tile Rendering Hierarchy (Player.js:48-56)

```
z=2: Movement blob (white #F0F0F0)
z=4: Attack blob (yellow #FFD900)
z=5: Attack reticles on enemies (yellow-tinted TileMapLargeSprite)
z=7: Selection box (white)
```

### Attack Blob vs Attack Reticle

In Duelyst:
- **Attack Blob (yellow):** Shows tiles in attack range from CURRENT position that are NOT in movement range. Purpose: show "attack extension" beyond movement.
- **Attack Reticle:** Shows on actual enemies you can attack. Uses `TileMapLargeSprite` tinted with `CONFIG.AGGRO_COLOR` (yellow #FFD900).

### State After Movement

From tile_interaction_flow.md and grid_rendering.md:

```
Movement Complete
    │
    ├─ Hide: Movement blob (unit has moved)
    ├─ Hide: Attack blob (no "extension" when can't move)
    ├─ Show: Selection box at new position
    └─ Show: Attack reticles on enemies in range (yellow-tinted)
```

---

## Root Cause Analysis

### Issue 1: Attack Reticle is White Instead of Yellow

**Location:** `src/grid_renderer.cpp:619-632`

```cpp
void GridRenderer::render_attack_reticle(const RenderConfig& config, BoardPos pos, float opacity) {
    // ...
    g_gpu.draw_sprite_transformed(attack_reticle, src, tl, tr, br, bl, opacity);
    //                                                                 ^^^^^^^
    //                                                                 ONLY OPACITY, NO COLOR TINTING
}
```

**Problem:**
- `render_attack_reticle()` passes only opacity to `draw_sprite_transformed()`
- The attack_reticle texture (`tile_large.png`) is white
- No color tinting is applied, so it renders white

**Contrast with `render_attack_blob()`:**
```cpp
void GridRenderer::render_attack_blob(...) {
    SDL_FColor color = TileColor::ATTACK_CURRENT;  // Yellow #FFD900
    color.a *= opacity;
    // ...
    render_corner_quad_rotated(config, pos, corner, tex, color);  // PASSES COLOR
}
```

The attack blob correctly uses yellow tinting via `render_corner_quad_rotated()` → `draw_sprite_transformed_tinted()`.

**Fix Required:** `render_attack_reticle()` must tint the reticle with `TileColor::ATTACK_CURRENT` (yellow #FFD900).

---

### Issue 2: Yellow Blob Shows on All Adjacent Tiles After Movement

**Location:** `src/main.cpp:1218-1237`

**Current Logic:**
```cpp
// blob_tiles = reachable_tiles + unit_pos
// After moving: reachable_tiles is empty, so blob_tiles = {unit_pos} only

std::vector<BoardPos> attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

// Remove tiles in movement blob
std::erase_if(attack_blob, [&](const BoardPos& p) {
    return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
});
// Only removes unit_pos, leaves ALL adjacent empty tiles

// Remove enemy positions (they get reticles)
std::erase_if(attack_blob, [&](const BoardPos& p) {
    return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
           != state.attackable_tiles.end();
});

// Result: attack_blob contains all adjacent empty tiles → rendered yellow
```

**Problem:**
- After moving, `reachable_tiles` is cleared (correct - can't move again)
- `blob_tiles` only contains `unit_pos`
- `attack_blob` includes ALL tiles in attack range (minus unit_pos, minus enemies)
- This causes yellow highlighting on all empty adjacent tiles

**Duelyst Behavior:**
- The yellow attack blob shows tiles you could attack as an EXTENSION beyond movement
- After moving, there's no movement range, so there's no "extension" concept
- Only attack reticles should show on actual enemies

**Fix Required:** After movement (when `reachable_tiles` is empty), the yellow attack blob should NOT be rendered. Only attack reticles on enemies should appear.

---

### Issue 3: Player Tile Highlighted White (Unknown if Correct)

**Current Behavior:**
- `blob_tiles = reachable_tiles + unit_pos` (line 1213-1214)
- Movement blob renders at all blob_tiles including unit_pos
- So unit's tile gets white (movement color) highlight

**Duelyst Behavior:**
From tile_interaction_flow.md section 7.3:
- Unit position IS included in the movement blob
- Selection box (`TileBoxSprite`) renders on top at z=7

**Verdict:** This is correct. The unit's tile should be white (part of movement blob) with selection box on top.

---

## Summary of Required Fixes

| Bug | Root Cause | Fix Location |
|-----|------------|--------------|
| Enemy tile white instead of yellow | `render_attack_reticle()` doesn't apply color tint | `grid_renderer.cpp:619-632` |
| Adjacent tiles yellow after move | Yellow blob rendered when it shouldn't be (no extension) | `main.cpp:1218-1237` |

### Fix 1: Tint Attack Reticle Yellow

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

    // Apply yellow tint (Duelyst: CONFIG.AGGRO_COLOR = #FFD900)
    SDL_FColor tint = TileColor::ATTACK_CURRENT;
    tint.a *= opacity;

    SDL_FRect src = {0, 0, (float)attack_reticle.width, (float)attack_reticle.height};
    g_gpu.draw_sprite_transformed_tinted(attack_reticle, src, tl, tr, br, bl, tint);
}
```

### Fix 2: Don't Show Yellow Blob When Movement Unavailable

```cpp
// In render_single_pass(), after line 1217:

// Attack blob = attack range from CURRENT position only (Duelyst behavior)
// Only show if there's a movement range to extend from
const auto& unit = state.units[state.selected_unit_idx];
std::vector<BoardPos> attack_blob;

// Only show yellow attack blob if movement is available (extension concept)
// After moving: no extension, only show reticles on enemies
if (!state.reachable_tiles.empty()) {
    attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

    // Remove tiles that are in the movement blob (they're already highlighted)
    std::erase_if(attack_blob, [&](const BoardPos& p) {
        return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
    });

    // Remove enemy positions (they get reticles instead of yellow blob)
    std::erase_if(attack_blob, [&](const BoardPos& p) {
        return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
               != state.attackable_tiles.end();
    });
}

// Render move blob (z=2) with seam detection
state.grid_renderer.render_move_range_alpha(config,
    blob_tiles, state.move_blob_opacity, attack_blob);

// Render attack blob (z=4) only if non-empty
if (!attack_blob.empty()) {
    state.grid_renderer.render_attack_blob(config, attack_blob, 200.0f/255.0f, blob_tiles);
}

// Render attack reticles (z=5) - always show on attackable enemies
for (const auto& target : state.attackable_tiles) {
    state.grid_renderer.render_attack_reticle(config, target);
}
```

---

## Expected Behavior After Fixes

### Turn Start (Unit Selected, Hasn't Moved)
- White blob: Tiles unit can move to + unit position
- Yellow blob: Tiles in attack range that are NOT in white blob (extension)
- Yellow reticles: On enemies within attack range from current position

### After Movement
- White blob: Just unit position (can't move again)
- Yellow blob: NONE (no extension when can't move)
- Yellow reticles: On enemies within attack range from NEW position

---

## Code References

| File | Function | Line | Purpose |
|------|----------|------|---------|
| `grid_renderer.cpp` | `render_attack_reticle` | 619-632 | Renders reticle (needs color tint) |
| `grid_renderer.cpp` | `render_attack_blob` | 635-659 | Renders yellow blob (correct) |
| `grid_renderer.hpp` | `TileColor::ATTACK_CURRENT` | 37 | Yellow #FFD900 color constant |
| `main.cpp` | `render_single_pass` | 1218-1242 | Attack blob/reticle logic |
| `main.cpp` | `update_selected_ranges` | 262-298 | Updates reachable/attackable tiles |

## Duelyst References

| File | Section | Content |
|------|---------|---------|
| `tile_interaction_flow.md` | §3 | Color constants |
| `tile_interaction_flow.md` | §7.3 | showEntityTiles() flow |
| `grid_rendering.md` | §8 | Tile highlight color config |
| `grid_rendering.md` | §9 | Z-order hierarchy |

---

## Implementation Log

**2024-12-24: Both fixes implemented**

### Fix 1: Attack Reticle Yellow Tinting
- **File:** `src/grid_renderer.cpp:619-636`
- **Change:** `render_attack_reticle()` now uses `draw_sprite_transformed_tinted()` with `TileColor::ATTACK_CURRENT` (yellow #FFD900)
- **Result:** Enemy attack reticles now render yellow instead of white

### Fix 2: Yellow Blob Only When Movement Available
- **File:** `src/main.cpp:1216-1250`
- **Change:** Attack blob calculation now gated by `if (!state.reachable_tiles.empty())`
- **Result:** After moving, no yellow blob on empty adjacent tiles - only yellow reticles on attackable enemies

### Additional Verification: Player Tile White
- **Finding:** Confirmed correct behavior via Duelyst source analysis
- **Evidence:** `app/view/Player.js:1217-1231` shows `selectedTiles` includes unit position
- **Result:** No fix needed - player tile white with selection box on top is correct Duelyst behavior
