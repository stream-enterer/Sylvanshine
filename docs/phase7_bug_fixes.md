# Phase 7 Bug Fixes (Revised)

Comprehensive bug analysis traced to Duelyst ground truth. Execute in order.

**Sources:** `duelyst_analysis/flows/tile_interaction_flow.md`, `duelyst_analysis/summaries/grid_rendering.md`

---

## Bug Summary

| ID | Bug | Root Cause | Duelyst Ground Truth |
|----|-----|------------|---------------------|
| A | Infinite sequential moves | `update_selected_ranges()` repopulates `reachable_tiles` every frame, ignoring `has_moved` | After move, unit can only attack, not move again |
| B | Selection box at old position during movement | Renders at `board_pos` which updates only on completion | Selection UI hidden during movement animation |
| C | Path arrow from old position during movement | Path calculated from `board_pos` (old) not `move_target` | Path hidden during movement animation |
| D | Yellow attack blob not visible | **GPU renderer discards RGB** — `render_corner_quad_rotated` passes only `tint.a` to shader | Yellow (#FFD900) shows attack range from current position |
| E | No attack reticles after movement | `update_selected_ranges()` uses wrong position during transition | Reticles show on attackable enemies from current position |
| F | Yellow ring around entire movement blob | Attack blob calculated from ALL move positions instead of current position | Yellow shows attack range from CURRENT position only |

---

## Fix 1: Gate Movement Range by has_moved

**File:** `src/main.cpp`
**Function:** `update_selected_ranges` (~line 262)

**Problem:** Unconditionally repopulates `reachable_tiles` every frame, allowing infinite moves.

**Find:**
```cpp
void update_selected_ranges(GameState& state) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) {
        state.selected_unit_idx = -1;
        return;
    }

    if (state.units[state.selected_unit_idx].is_dead()) {
        clear_selection(state);
        return;
    }

    BoardPos selected_pos = state.units[state.selected_unit_idx].board_pos;

    auto occupied = get_occupied_positions(state, state.selected_unit_idx);
    state.reachable_tiles = get_reachable_tiles(selected_pos, MOVE_RANGE, occupied);

    auto enemy_positions = get_enemy_positions(state, state.selected_unit_idx);
    state.attackable_tiles = get_attackable_tiles(
        selected_pos,
        state.units[state.selected_unit_idx].attack_range,
        enemy_positions
    );
}
```

**Replace:**
```cpp
void update_selected_ranges(GameState& state) {
    if (state.selected_unit_idx < 0) return;
    if (state.selected_unit_idx >= static_cast<int>(state.units.size())) {
        state.selected_unit_idx = -1;
        return;
    }

    int idx = state.selected_unit_idx;

    if (state.units[idx].is_dead()) {
        clear_selection(state);
        return;
    }

    // Don't update ranges while unit is busy (moving/attacking)
    if (state.units[idx].is_moving() || state.units[idx].is_attacking()) {
        return;
    }

    BoardPos selected_pos = state.units[idx].board_pos;

    // Only show movement range if unit hasn't moved this turn
    bool can_move = idx >= static_cast<int>(state.has_moved.size()) || !state.has_moved[idx];
    if (can_move) {
        auto occupied = get_occupied_positions(state, idx);
        state.reachable_tiles = get_reachable_tiles(selected_pos, MOVE_RANGE, occupied);
    }
    // If unit has moved, reachable_tiles stays empty (cleared in handle_move_click)

    // Always update attackable tiles from current position
    auto enemy_positions = get_enemy_positions(state, idx);
    state.attackable_tiles = get_attackable_tiles(
        selected_pos,
        state.units[idx].attack_range,
        enemy_positions
    );
}
```

---

## Fix 2: Hide Selection UI During Movement

**File:** `src/main.cpp`
**Function:** `render_single_pass` (~line 1195)

**Problem:** Selection box, blobs, and paths render during movement using stale `board_pos`.

**Find:**
```cpp
    // 3. Selection highlights
    if (state.selected_unit_idx >= 0 && state.game_phase == GamePhase::Playing) {
```

**Replace:**
```cpp
    // 3. Selection highlights (hide during movement/attack - Duelyst behavior)
    bool unit_is_busy = state.selected_unit_idx >= 0 &&
        (state.units[state.selected_unit_idx].is_moving() ||
         state.units[state.selected_unit_idx].is_attacking());

    if (state.selected_unit_idx >= 0 && state.game_phase == GamePhase::Playing && !unit_is_busy) {
```

---

## Fix 3: Prevent Path Calculation During Movement

**File:** `src/main.cpp`
**Function:** `update_hover_path` (~line 958)

**Problem:** Path calculated from old `board_pos` during movement.

**Find (at start of function):**
```cpp
void update_hover_path(GameState& state, const RenderConfig& config) {
    // Check if hover is in movement range
    bool in_move_range = false;
```

**Replace:**
```cpp
void update_hover_path(GameState& state, const RenderConfig& config) {
    // Don't calculate path while unit is moving
    if (state.selected_unit_idx >= 0 && state.units[state.selected_unit_idx].is_moving()) {
        return;
    }

    // Check if hover is in movement range
    bool in_move_range = false;
```

---

## Fix 4: Remove Redundant attackable_tiles Update

**File:** `src/main.cpp`
**Function:** `update` (~line 1006)

**Problem:** The Issue 5.11 fix recalculates `attackable_tiles` on move completion, but `update_selected_ranges` now handles this correctly. Remove the redundant code.

**Find:**
```cpp
    for (size_t i = 0; i < state.units.size(); i++) {
        bool was_moving = state.units[i].is_moving();
        state.units[i].update(dt, config);

        // Update attackable tiles when selected unit finishes moving
        if (was_moving && !state.units[i].is_moving() &&
            state.selected_unit_idx == static_cast<int>(i)) {
            auto enemy_positions = get_enemy_positions(state, state.selected_unit_idx);
            state.attackable_tiles = get_attackable_tiles(
                state.units[i].board_pos,
                state.units[i].attack_range,
                enemy_positions);
        }
    }
```

**Replace:**
```cpp
    for (size_t i = 0; i < state.units.size(); i++) {
        state.units[i].update(dt, config);
    }
```

**Note:** `update_selected_ranges()` now handles this case correctly because it checks `is_moving()` and only updates when the unit is idle.

---

## Fix 5: Clear UI State When Movement Starts

**File:** `src/main.cpp`
**Function:** `handle_move_click` (~line 725)

**Problem:** Need to ensure all hover/path state is cleared when movement begins.

**Find:**
```cpp
    if (unit_idx < static_cast<int>(state.has_moved.size())) {
        state.has_moved[unit_idx] = true;
    }

    // Keep selection — player can attack after move
    state.reachable_tiles.clear();
    state.movement_path.clear();

    state.units[unit_idx].start_move(config, clicked);
}
```

**Replace:**
```cpp
    if (unit_idx < static_cast<int>(state.has_moved.size())) {
        state.has_moved[unit_idx] = true;
    }

    // Keep selection — player can attack after move
    // Clear all movement-related UI state
    state.reachable_tiles.clear();
    state.movement_path.clear();
    state.attackable_tiles.clear();  // Will be recalculated after move completes
    state.move_blob_opacity = 1.0f;
    state.tile_anims.clear();

    state.units[unit_idx].start_move(config, clicked);
}
```

---

## Fix 6: Yellow Blob Calculation (Verify Logic)

**File:** `src/main.cpp`
**Function:** `render_single_pass` (~line 1201)

The yellow attack blob calculation in Issue 6 fix is conceptually correct but needs verification:

```cpp
// Calculate attack blob from ALL move positions
const auto& unit = state.units[state.selected_unit_idx];
std::vector<BoardPos> attack_blob;

for (const auto& from_pos : blob_tiles) {
    auto local_attack = get_attack_pattern(from_pos, unit.attack_range);
    for (const auto& p : local_attack) {
        if (std::find(attack_blob.begin(), attack_blob.end(), p) == attack_blob.end()) {
            attack_blob.push_back(p);
        }
    }
}
```

**Expected result for melee unit (attack_range=1) at (2,2) with move_range=3:**
- `blob_tiles` = 25 tiles (movement blob + unit position)
- `get_attack_pattern` returns 8 adjacent tiles (Chebyshev distance 1)
- Union = ~80 unique tiles
- After removing blob_tiles: ~55 tiles (attack fringe)
- After removing attackable enemies: slightly fewer

**If yellow still doesn't show:** Check `render_attack_blob` implementation in `grid_renderer.cpp` for:
1. Color being applied (should be yellow #FFD900 equivalent)
2. Opacity being non-zero (200/255 = 0.78)
3. Valid tile positions being rendered

---

## Fix 7: GPU Renderer Color Tinting (ROOT CAUSE)

**Files:**
- `include/gpu_renderer.hpp`
- `src/gpu_renderer.cpp`
- `shaders/sprite.frag`
- `src/grid_renderer.cpp`

**Problem:** `render_corner_quad_rotated` passed only `tint.a` (alpha) to `draw_sprite_transformed`, discarding the RGB color values entirely. This caused all blob tiles to render as white instead of their designated colors.

```cpp
// OLD CODE (grid_renderer.cpp:550)
g_gpu.draw_sprite_transformed(texture, src, tl, tr, br, bl, tint.a);
//                                                        ^^^^^^ Only alpha!
```

**Solution:**

1. **Extend `SpriteUniforms`** to include tint color:
```cpp
struct SpriteUniforms {
    float opacity;
    float dissolve_time;
    float seed;
    float padding;
    // Tint color (RGBA), multiplied with texture color
    float tint_r;
    float tint_g;
    float tint_b;
    float tint_a;
};
```

2. **Update `sprite.frag`** to apply tint:
```glsl
void main() {
    vec4 color = texture(u_texture, v_texCoord);
    vec4 tint = vec4(u_tint_r, u_tint_g, u_tint_b, u_tint_a);
    color.rgb *= tint.rgb;
    color.a *= u_opacity * tint.a;
    fragColor = color;
}
```

3. **Add `draw_sprite_transformed_tinted`** function that accepts full `SDL_FColor`.

4. **Update `render_corner_quad_rotated`** to use the new tinted function:
```cpp
// NEW CODE (grid_renderer.cpp:550)
g_gpu.draw_sprite_transformed_tinted(texture, src, tl, tr, br, bl, tint);
```

**Result:** Attack blob now renders with correct yellow color (#FFD900).

---

## Fix 8: Attack Blob Calculation (Duelyst Ground Truth)

**File:** `src/main.cpp`
**Function:** `render_single_pass` (~line 1216)

**Problem:** Attack blob was calculated from ALL positions in the movement blob, creating a huge yellow ring around the entire movement range. This was incorrect.

**Duelyst Ground Truth:** From `app/view/Player.js:1301-1463` (`showEntityTiles`):
- `moveLocations` = `sdkEntity.getMovementRange().getValidPositions()` — gray blob
- `attackLocations` = `sdkEntity.getAttackRange().getValidPositions()` — yellow blob

The yellow blob is simply the attack range from the **CURRENT position only**, not aggregated from all move positions.

**Old Code:**
```cpp
// Calculate attack blob from ALL move positions  ← WRONG
for (const auto& from_pos : blob_tiles) {
    auto local_attack = get_attack_pattern(from_pos, unit.attack_range);
    for (const auto& p : local_attack) {
        if (std::find(attack_blob.begin(), attack_blob.end(), p) == attack_blob.end()) {
            attack_blob.push_back(p);
        }
    }
}
```

**New Code:**
```cpp
// Attack blob = attack range from CURRENT position only (Duelyst behavior)
std::vector<BoardPos> attack_blob = get_attack_pattern(unit_pos, unit.attack_range);

// Remove tiles that are in the movement blob (they're already highlighted)
std::erase_if(attack_blob, [&](const BoardPos& p) {
    return std::find(blob_tiles.begin(), blob_tiles.end(), p) != blob_tiles.end();
});

// Remove enemy positions (they get reticles instead of yellow blob)
std::erase_if(attack_blob, [&](const BoardPos& p) {
    return std::find(state.attackable_tiles.begin(), state.attackable_tiles.end(), p)
           != state.attackable_tiles.end();
});
```

**Result:** Yellow blob now correctly shows only tiles attackable from current position. Enemy positions get reticles (rendered on top at z-order 5), not yellow tiles.

---

## Duelyst Reference: Selection UI State Machine

From `tile_interaction_flow.md`:

```
Unit Selected (Idle)
    │
    ├─ Show: Selection box, movement blob, attack blob, reticles
    │
    ▼
User Clicks Move Target
    │
    ├─ HIDE ALL: Selection box, blobs, path
    │
    ▼
Movement Animation Playing
    │
    ├─ Show: Nothing (unit animating)
    │
    ▼
Movement Complete
    │
    ├─ Show: Selection box (at new pos), attack blob, reticles
    ├─ Hide: Movement blob (unit has moved)
    │
    ▼
User Attacks OR Deselects
    │
    └─ Clear selection, end unit's turn
```

---

## Build & Test

```bash
./build.fish quick && ./build/tactics
```

### Checklist
- [x] Select unit → Move blob (gray/white) + attack fringe (yellow #FFD900) visible
- [x] Click to move → All selection UI disappears during animation
- [x] Movement completes → Unit still selected, NO move blob, attack range (yellow) around unit
- [x] Cannot move again (click empty tile in old move range = nothing happens)
- [ ] Can attack adjacent enemy → Turn ends for that unit
- [ ] Click non-reachable tile after move → Deselects, ends turn
- [ ] Space bar ends turn early
- [ ] AI still works

**Fixed in this session:**
- Fix 7 (GPU Color Tinting) — Root cause of yellow not showing
- Fix 8 (Attack Blob Calculation) — Yellow blob was aggregating from all move positions instead of current position only

---

## Trace Summary

| Screenshot | Expected State | Observed Bug |
|------------|----------------|--------------|
| #1 (Before) | Move blob + yellow fringe + reticles on in-range enemies | No yellow, no reticles (yellow may be correctly empty if no attack fringe exists at current setup) |
| #2 (During) | No selection UI visible | Selection box at OLD position, path can still be drawn |
| #3 (After) | Selection box at NEW position, attack reticles on adjacent enemies, NO move blob | Move blob still shows (infinite moves), selection box present but unclear if reticles show |
