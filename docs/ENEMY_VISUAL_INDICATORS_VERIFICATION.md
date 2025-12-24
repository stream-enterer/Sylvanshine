# Implementation Plan Verification

Cross-verification of `ENEMY_VISUAL_INDICATORS_IMPL_PLAN.md` against Duelyst source code and Sylvanshine architecture.

---

## Verification Summary

| Step | Claim | Verified Against | Result | Action |
|------|-------|------------------|--------|--------|
| P1.1 | `is_enemy_idle()` conditions | EntityNode.js:489 | PARTIAL | Add note about `is_attacking()` |
| P1.2 | Render after floor grid | main.cpp structure | OK | None |
| P1.3 | Remove hover-only code | main.cpp:1278-1285 | OK | None |
| P1.3 | Keep #D22846 for indicator | config.js:813 | **WRONG** | Fix to #FF0000 |
| P2.1 | #D22846 for attack preview | config.js:831, Player.js:1457 | OK | None |
| P2.2 | Add color param to `render_attack_blob` | grid_renderer.hpp:160 | OK | Backward compatible |
| P2.3 | `TileOpacity::FULL` = 200/255 | config.js:528 | OK | Matches Duelyst |
| T2.5 | Range 1 = 8 tiles | Chebyshev math | OK | 3x3 - 1 = 8 |
| T2.6 | Range 2 = 24 tiles | Chebyshev math | OK | 5x5 - 1 = 24 |

---

## Critical Issue #1: Ownership Indicator Color

### Claim in Plan
> **Decision:** Keep current `#D22846` — it's more visually cohesive with the attack system colors.

### Duelyst Source (config.js:812-813)
```javascript
CONFIG.OPPONENT_OWNER_OPACITY = 80;
CONFIG.OPPONENT_OWNER_COLOR = { r: 255, g: 0, b: 0 };  // #FF0000
```

### Duelyst Source (config.js:831)
```javascript
CONFIG.AGGRO_OPPONENT_COLOR = { r: 210, g: 40, b: 70 };  // #D22846
```

### Analysis
The plan conflates two different colors:
- **Ownership indicator** (idle state): `#FF0000` @ 80/255 (31%) — pure red
- **Attack preview blob** (hover state): `#D22846` @ 200/255 (78%) — pinkish red

These serve different purposes:
- `#FF0000` is a danger/warning color (enemy presence)
- `#D22846` is an aggression/action color (attack range)

### Resolution
**Option A (Spec-faithful):** Use `#FF0000 @ 31%` for ownership indicator

**Option B (Sylvanshine-original):** Keep `#D22846 @ 29%` but document as intentional deviation

**Recommendation:** Option A. The colors are distinct for a reason. Using the same color for both creates visual ambiguity.

### Required Plan Change
```cpp
// grid_renderer.cpp render_enemy_indicator()
// BEFORE (wrong):
constexpr SDL_FColor ENEMY_COLOR = {0.824f, 0.157f, 0.275f, 75.0f/255.0f};  // #D22846, 29%

// AFTER (Duelyst-faithful):
constexpr SDL_FColor ENEMY_OWNER_COLOR = {1.0f, 0.0f, 0.0f, 80.0f/255.0f};  // #FF0000, 31%
```

---

## Issue #2: `is_attacking()` Check Not in Duelyst

### Claim in Plan
```cpp
if (enemy.is_dead() || enemy.is_moving() || enemy.is_attacking()) {
    return false;
}
```

### Duelyst Source (EntityNode.js:489)
```javascript
&& !this.hovered && !this.selected && !this.isValidTarget && !this.getIsMoving() && this.getIsSpawned()
```

### Analysis
Duelyst checks:
- `!this.hovered` ✓ (plan has hover check)
- `!this.selected` — N/A for enemies in Sylvanshine
- `!this.isValidTarget` ✓ (plan checks `attackable_tiles`)
- `!this.getIsMoving()` ✓ (plan has `is_moving()`)
- `this.getIsSpawned()` — Sylvanshine units are always spawned after creation

Duelyst does NOT check `isAttacking()`. However:
- In Duelyst, attacks are instant (no attack animation state)
- In Sylvanshine, attacks have duration and animation state
- Adding `is_attacking()` is logically consistent with "idle state" concept

### Resolution
Keep `is_attacking()` check but document as Sylvanshine extension.

### Required Plan Change
Add comment in code:
```cpp
// Must be alive and not animating
// NOTE: is_attacking() check is Sylvanshine-specific (Duelyst attacks are instant)
if (enemy.is_dead() || enemy.is_moving() || enemy.is_attacking()) {
```

---

## Issue #3: Missing `is_spawning()` Check

### Duelyst Source (EntityNode.js:489)
```javascript
&& this.getIsSpawned()
```

### Sylvanshine Context
- Sylvanshine has `EntityState::Spawning` state
- Entity has `is_spawning()` method
- Plan doesn't check this

### Analysis
During spawn animation, the entity shouldn't show ownership indicator (visual clutter during spawn FX).

### Resolution
Add check to `is_enemy_idle()`:

```cpp
// Must be alive, spawned, and not animating
if (enemy.is_dead() || enemy.is_spawning() || enemy.is_moving() || enemy.is_attacking()) {
    return false;
}
```

---

## Issue #4: Enemy Turn Behavior Undocumented in Duelyst

### Plan Claim
```cpp
// Hide during enemy turn (enemies are "acting", not idle)
if (state.turn_phase == TurnPhase::EnemyTurn) {
    return false;
}
```

### Duelyst Behavior
In Duelyst bot games (single-player), there is NO opponent visualization during bot turn (see spec section 12.5). The bot's actions "just happen" without preview.

However, the ownership indicator is tied to `gameLayer.getIsActive()` (line 492), not turn phase. The indicator would still show during opponent's turn in Duelyst if the entity is idle.

### Analysis
The plan adds enemy turn hiding as a Sylvanshine design choice, not Duelyst behavior. This is reasonable for visual clarity during AI turn.

### Resolution
Keep the check but clarify in documentation:
```cpp
// Hide during enemy turn (Sylvanshine design choice - reduces visual noise during AI actions)
// Duelyst: indicators remain visible but AI provides no visual feedback anyway
if (state.turn_phase == TurnPhase::EnemyTurn) {
```

---

## Verified Claims

### P2.1: AGGRO_OPPONENT_COLOR Value
**Claim:** `#D22846` (0.824, 0.157, 0.275)

**Source:** config.js:831
```javascript
CONFIG.AGGRO_OPPONENT_COLOR = { r: 210, g: 40, b: 70 };
```

**Verification:** 210/255 = 0.824, 40/255 = 0.157, 70/255 = 0.275 ✓

### P2.3: TileOpacity::FULL Matches Duelyst
**Claim:** `TileOpacity::FULL = 200/255 = 78.4%`

**Source:** config.js:528
```javascript
CONFIG.TILE_SELECT_OPACITY = 200;
```

**Verification:** Exact match ✓

### P2 Backward Compatibility
**Claim:** Adding color parameter won't break existing call

**Existing call (main.cpp:1247):**
```cpp
state.grid_renderer.render_attack_blob(config, attack_blob, 200.0f/255.0f, blob_tiles);
```

**Proposed signature:**
```cpp
void render_attack_blob(const RenderConfig& config,
                        const std::vector<BoardPos>& tiles,
                        float opacity = 200.0f/255.0f,
                        const std::vector<BoardPos>& alt_blob = {},
                        SDL_FColor color = TileColor::ATTACK_CURRENT);
```

**Verification:** 4 arguments passed, 5th parameter has default. Backward compatible ✓

### Test T2.5: Attack Range 1 = 8 Tiles
**Claim:** Chebyshev distance ≤ 1 produces 8 tiles

**Math:**
- Chebyshev distance = max(|dx|, |dy|)
- Distance ≤ 1 means -1 ≤ dx ≤ 1 AND -1 ≤ dy ≤ 1
- This is a 3x3 grid = 9 tiles, minus center = 8 tiles ✓

### Test T2.6: Attack Range 2 = 24 Tiles
**Claim:** Chebyshev distance ≤ 2 produces 24 tiles

**Math:**
- Distance ≤ 2 means -2 ≤ dx ≤ 2 AND -2 ≤ dy ≤ 2
- This is a 5x5 grid = 25 tiles, minus center = 24 tiles ✓

---

## Cascading Effect Analysis

### P1: Adding Idle Indicator Render Pass

**Insertion point:** After `render_floor_grid()`, before selection highlights block

**Potential issues:**
1. **Performance:** Loop over all units each frame — negligible (typically 2-5 units)
2. **Z-order:** Renders below selection blobs — correct
3. **State dependency:** Reads `attackable_tiles` — only populated when unit selected, otherwise empty — correct

**Verdict:** No cascading effects ✓

### P2: Adding Color Parameter to `render_attack_blob()`

**Affected call sites:**
1. `main.cpp:1247` — passes 4 args, new param has default — OK
2. New P2 call — passes 5 args including color — OK

**Potential issues:**
1. **Default parameter after non-default:** `alt_blob = {}` precedes `color = ...` — valid C++ ✓
2. **Header/source mismatch:** Both must be updated — plan addresses this ✓

**Verdict:** No cascading effects ✓

### P2: Computing Attack Preview On-the-fly

**Called per frame when:** `hover_valid && selected_unit_idx < 0 && hovered enemy`

**Performance:**
- `find_unit_at_pos()`: O(n) where n = unit count (~5)
- `get_attack_pattern()`: O(45) fixed (9x5 board)
- `render_attack_blob()`: O(tiles × 4 corners) = O(32) for range 1

**Total:** O(100) per frame when hovering enemy — negligible

**Verdict:** No performance impact ✓

---

## Required Plan Updates

### 1. Fix Ownership Indicator Color

**File:** `grid_renderer.cpp`

**Change:**
```cpp
// BEFORE:
constexpr SDL_FColor ENEMY_COLOR = {0.824f, 0.157f, 0.275f, 75.0f/255.0f};

// AFTER:
constexpr SDL_FColor ENEMY_OWNER_COLOR = {1.0f, 0.0f, 0.0f, 80.0f/255.0f};  // #FF0000, 31%
```

### 2. Add `is_spawning()` Check

**In `is_enemy_idle` lambda:**
```cpp
if (enemy.is_dead() || enemy.is_spawning() || enemy.is_moving() || enemy.is_attacking()) {
    return false;
}
```

### 3. Add Documentation Comments

```cpp
// Ownership indicators for idle enemies
// - Duelyst uses #FF0000 @ 31% (OPPONENT_OWNER_COLOR)
// - is_attacking() check is Sylvanshine-specific (Duelyst attacks are instant)
// - Enemy turn hiding is Sylvanshine design choice (Duelyst shows during opponent turn)
```

---

## Final Corrected Implementation

### P1: Complete `is_enemy_idle` Lambda

```cpp
// Ownership indicators for idle enemies (Duelyst: red tile under non-interacted enemies)
// Rendered below movement/attack blobs, above floor
auto is_enemy_idle = [&](size_t idx) -> bool {
    const Entity& enemy = state.units[idx];

    // Must be alive, spawned, and not animating
    // NOTE: is_attacking() is Sylvanshine-specific (Duelyst attacks are instant)
    if (enemy.is_dead() || enemy.is_spawning() || enemy.is_moving() || enemy.is_attacking()) {
        return false;
    }

    // Hide during enemy turn (Sylvanshine design: reduces visual noise during AI)
    if (state.turn_phase == TurnPhase::EnemyTurn) {
        return false;
    }

    // Hide if this enemy is being hovered
    if (state.hover_valid && enemy.board_pos == state.hover_pos) {
        return false;
    }

    // Hide if this enemy is a valid attack target (will show reticle instead)
    for (const auto& target : state.attackable_tiles) {
        if (target == enemy.board_pos) {
            return false;
        }
    }

    return true;
};

for (size_t i = 0; i < state.units.size(); i++) {
    if (state.units[i].type == UnitType::Enemy && is_enemy_idle(i)) {
        state.grid_renderer.render_enemy_indicator(config, state.units[i].board_pos);
    }
}
```

### P1: Corrected Color Constant

```cpp
// grid_renderer.cpp render_enemy_indicator()
void GridRenderer::render_enemy_indicator(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;
    // Duelyst: OPPONENT_OWNER_COLOR = #FF0000 @ 80/255 (31%)
    constexpr SDL_FColor ENEMY_OWNER_COLOR = {1.0f, 0.0f, 0.0f, 80.0f/255.0f};
    render_highlight(config, pos, ENEMY_OWNER_COLOR);
}
```
