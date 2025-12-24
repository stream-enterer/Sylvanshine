# Duelyst Selection System: Complete Behavior Reference

**Sources:** `tile_interaction_flow.md`, `grid_rendering.md`, `Player.js`

---

## 1. Color Palette (Ground Truth)

| Element | Duelyst Color | Hex | RGB |
|---------|---------------|-----|-----|
| Movement blob | Light gray | #F0F0F0 | 240, 240, 240 |
| Attack blob (friendly) | **YELLOW** | #FFD900 | 255, 217, 0 |
| Attack blob (enemy) | Red | #D22846 | 210, 40, 70 |
| Selection box | White | #FFFFFF | 255, 255, 255 |
| Path | White | #FFFFFF | 255, 255, 255 |
| Hover | White | #FFFFFF | 255, 255, 255 |

---

## 2. Selection State Machine

### State A: No Unit Selected
- **Visible:** Nothing (or hover indicator if mouse on board)
- **Clickable:** Own units to select them

### State B: Unit Selected, Can Move + Attack
- **Visible:**
  - Selection box at unit position (pulsing)
  - Movement blob (gray #F0F0F0) - tiles unit can move to
  - Attack fringe (yellow #FFD900) - tiles attackable from ANY move position but NOT in move blob
  - Attack reticles (red target) - enemies within attack range from CURRENT position
- **Behavior on hover:**
  - Hover on move tile: Show path arrow, dim blob to 50%
  - Hover on enemy in range: Show attack indicator
- **Clickable:**
  - Move tile → Execute move, transition to State C
  - Attackable enemy → Execute attack, transition to State D
  - Empty tile outside range → Deselect

### State C: Unit Selected, Already Moved (Can Only Attack)
- **Visible:**
  - Selection box at NEW position (after move complete)
  - Attack range (yellow #FFD900) - tiles attackable from CURRENT position only
  - Attack reticles on enemies in range
  - **NO movement blob**
- **NOT visible:**
  - Movement blob (unit already moved)
  - Attack fringe (no movement positions to calculate from)
- **Clickable:**
  - Attackable enemy → Execute attack, transition to State D
  - Any other tile → Deselect, end unit's turn

### State D: Unit Selected, Already Attacked (Turn Complete)
- **Visible:** Nothing - unit is deselected
- **Unit state:** has_attacked = true, cannot be reselected this turn

---

## 3. Attack Range (Yellow Blob)

**Source:** `app/view/Player.js:1301-1463` — `showEntityTiles()`

The yellow attack blob shows tiles attackable from the **CURRENT position only**.

| Concept | Definition | Color |
|---------|------------|-------|
| **Attack Range** | Tiles within attack distance from current position | Yellow #FFD900 |
| **Attackable Tiles** | Enemy positions within attack range (get reticles) | Reticle overlay |

### Attack Pattern (Chebyshev Distance)

Duelyst uses **Chebyshev distance** for attack range:
- Range 1 = 8 surrounding tiles (cardinal + diagonal)
- Range 2 = 24 tiles (5x5 minus center)

**NOT Manhattan distance** (which would be 4 tiles for range 1).

---

## 4. Blob Rendering Details

### Movement Blob
- Color: Gray #F0F0F0
- Includes: Unit's current position + all reachable tiles
- Shape: Merged corner tiles with seam detection

### Attack Blob (Both Before and After Move)
- Color: **Yellow #FFD900**
- Calculation:
  ```
  attack_blob = get_attack_pattern(unit_pos, attack_range)  // Chebyshev
  remove tiles in movement blob (they're already gray)
  remove enemy positions (they get reticles instead)
  ```
- Purpose: Shows where you can attack RIGHT NOW from current position

### Attack Reticles
- Rendered ON TOP of blobs (z-order 5)
- Show on enemy positions within attack range
- Static (no pulsing)

---

## 5. Z-Order (Back to Front)

```
1. Board grid
2. Movement blob
3. (unused)
4. Attack blob
5. Attack reticles
6. Card tiles
7. Selection box
8. Path arrow
9. Hover indicator
```

---

## 6. Animation During Movement

**While unit is moving:**
- ALL selection UI is hidden (blobs, box, path, reticles)
- Unit animates from old position to new position

**When movement completes:**
- Selection box appears at NEW position
- Attack range appears (yellow)
- Attack reticles appear on enemies in range
- Movement blob does NOT appear (unit has moved)

---

## 7. Expected Visual for Screenshots

### Screenshot 1 Analysis (Post-Move State)
If unit has moved:
- Should see: Selection box + yellow attack tiles around unit + reticles on adjacent enemies
- Should NOT see: Large movement blob

If showing large blob → Bug: movement blob appearing when it shouldn't

### Screenshot 2 Analysis (Post-Move State)
- Shows a 2x4 tile blob including unit tile
- This could be attack range (if attack_range > 1)
- For melee (range 1), should see 3x3 area (8 tiles + center)

---

## 8. Bug Identification

### Bug 1: No Yellow Color
**Root cause:** `render_corner_quad_rotated` passes only `tint.a` to `draw_sprite_transformed`
```cpp
g_gpu.draw_sprite_transformed(texture, src, tl, tr, br, bl, tint.a);
//                                                        ^^^^^^ Only alpha!
```
**Fix:** Need to pass full color and modify GPU renderer to support tinting

### Bug 2: Movement Blob After Move
**Symptom:** Large blob visible after unit has moved
**Possible causes:**
1. `has_moved` not being checked correctly in render
2. `reachable_tiles` not being cleared
3. Attack fringe calculation including movement positions

### Bug 3: 8 Tiles vs 4 Tiles
**Expected:** For melee (range 1) with Chebyshev distance = 8 surrounding tiles
**If seeing 4 tiles:** Bug in attack pattern calculation (using Manhattan instead of Chebyshev)
**If seeing movement-sized blob:** Bug #2 (movement blob leaking)

---

## 9. Code Locations

| System | File | Function |
|--------|------|----------|
| State management | `main.cpp` | `update_selected_ranges()` |
| Render decision | `main.cpp` | `render_single_pass()` |
| Movement blob | `grid_renderer.cpp` | `render_move_range_alpha()` |
| Attack blob | `grid_renderer.cpp` | `render_attack_blob()` |
| Corner rendering | `grid_renderer.cpp` | `render_corner_quad_rotated()` |
| Attack calculation | `grid_renderer.cpp` | `get_attack_pattern()` |
