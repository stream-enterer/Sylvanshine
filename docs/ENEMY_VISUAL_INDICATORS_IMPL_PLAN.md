# Enemy Visual Indicators Implementation Plan (LLM-Optimized)

Implementation plan for features identified in `ENEMY_VISUAL_INDICATORS_AUDIT.md`.

**Revision:** 3.0 — Optimized for LLM execution

---

## Prerequisites (Verify Before Starting)

Run these grep commands to confirm required functions/types exist:

| Check | Command | Expected |
|-------|---------|----------|
| `find_unit_at_pos` exists | `grep -n "int find_unit_at_pos" src/main.cpp` | Line ~207 |
| `get_attack_pattern` exists | `grep -n "get_attack_pattern" include/grid_renderer.hpp` | Line ~187 |
| `render_enemy_indicator` exists | `grep -n "render_enemy_indicator" include/grid_renderer.hpp` | Line ~155 |
| `TileColor` namespace | `grep -n "namespace TileColor" include/grid_renderer.hpp` | Line ~32 |
| Entity state methods | `grep "is_spawning\|is_moving\|is_attacking" include/entity.hpp` | 3 matches |
| `TurnPhase::EnemyTurn` | `grep "TurnPhase::EnemyTurn" src/main.cpp` | Multiple matches |

**If any check fails:** Stop and report the missing dependency.

---

## Overview

| Priority | Feature | Files Modified |
|----------|---------|----------------|
| P1 | Idle-state ownership indicator | `src/main.cpp`, `src/grid_renderer.cpp` |
| P2 | Enemy attack preview blob | `src/main.cpp`, `include/grid_renderer.hpp`, `src/grid_renderer.cpp` |

---

## P1: Fix Ownership Indicator Logic

### Step 1.1: Insert idle-enemy indicator loop after floor grid render

**File:** `src/main.cpp`

**Find this exact pattern:**
```cpp
    // 1. Floor grid (semi-transparent dark tiles with gaps)
    state.grid_renderer.render_floor_grid(config);

    // 2. Grid lines (disabled — using tile gaps instead)
```

**Replace with:**
```cpp
    // 1. Floor grid (semi-transparent dark tiles with gaps)
    state.grid_renderer.render_floor_grid(config);

    // 1b. Ownership indicators for idle enemies (Duelyst: red tile under non-interacted enemies)
    auto is_enemy_idle = [&](size_t idx) -> bool {
        const Entity& enemy = state.units[idx];
        if (enemy.is_dead() || enemy.is_spawning() || enemy.is_moving() || enemy.is_attacking()) {
            return false;
        }
        if (state.turn_phase == TurnPhase::EnemyTurn) {
            return false;
        }
        if (state.hover_valid && enemy.board_pos == state.hover_pos) {
            return false;
        }
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

    // 2. Grid lines (disabled — using tile gaps instead)
```

### Step 1.2: Remove old hover-only indicator code

**File:** `src/main.cpp`

**Find this exact pattern:**
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
```

**Replace with:**
```cpp
    // 4. Hover highlight
    if (state.hover_valid) {
        state.grid_renderer.render_hover(config, state.hover_pos);
```

### Step 1.3: Update indicator color to Duelyst spec

**File:** `src/grid_renderer.cpp`

**Find this exact pattern:**
```cpp
void GridRenderer::render_enemy_indicator(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;
    // Red overlay using existing highlight system (API doesn't support tinted sprites)
    constexpr SDL_FColor ENEMY_COLOR = {0.824f, 0.157f, 0.275f, 75.0f/255.0f};  // #D22846, 29%
```

**Replace with:**
```cpp
void GridRenderer::render_enemy_indicator(const RenderConfig& config, BoardPos pos) {
    if (!pos.is_valid()) return;
    // Duelyst OPPONENT_OWNER_COLOR: #FF0000 @ 31% opacity (config.js:812-813)
    constexpr SDL_FColor ENEMY_COLOR = {1.0f, 0.0f, 0.0f, 80.0f/255.0f};  // #FF0000, 31%
```

### Step 1.4: Verify build

```bash
./build.fish build
```

**Expected:** Build succeeds with 0 errors.

---

## P2: Enemy Attack Preview Blob

### Step 2.1: Add ENEMY_ATTACK color constant

**File:** `include/grid_renderer.hpp`

**Find this exact pattern:**
```cpp
namespace TileColor {
    // Movement: slightly off-white (Duelyst's #F0F0F0)
    constexpr SDL_FColor MOVE_CURRENT   = {0.941f, 0.941f, 0.941f, 200.0f/255.0f};

    // Attack: Duelyst's signature yellow #FFD900
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 0.851f, 0.0f, 200.0f/255.0f};

    // Path and hover
    constexpr SDL_FColor PATH  = {1.0f, 1.0f, 1.0f, 150.0f/255.0f};
    // Duelyst: CONFIG.TILE_FAINT_OPACITY = 75/255 for passive hover
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 75.0f/255.0f};
}
```

**Replace with:**
```cpp
namespace TileColor {
    // Movement: slightly off-white (Duelyst's #F0F0F0)
    constexpr SDL_FColor MOVE_CURRENT   = {0.941f, 0.941f, 0.941f, 200.0f/255.0f};

    // Attack: Duelyst's signature yellow #FFD900
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 0.851f, 0.0f, 200.0f/255.0f};

    // Enemy attack preview: Duelyst AGGRO_OPPONENT_COLOR #D22846
    constexpr SDL_FColor ENEMY_ATTACK   = {0.824f, 0.157f, 0.275f, 200.0f/255.0f};

    // Path and hover
    constexpr SDL_FColor PATH  = {1.0f, 1.0f, 1.0f, 150.0f/255.0f};
    // Duelyst: CONFIG.TILE_FAINT_OPACITY = 75/255 for passive hover
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 75.0f/255.0f};
}
```

### Step 2.2: Add color parameter to render_attack_blob declaration

**File:** `include/grid_renderer.hpp`

**Find this exact pattern:**
```cpp
    void render_attack_blob(const RenderConfig& config,
                            const std::vector<BoardPos>& tiles,
                            float opacity = 200.0f/255.0f,
                            const std::vector<BoardPos>& alt_blob = {});  // Optional for seam detection
```

**Replace with:**
```cpp
    void render_attack_blob(const RenderConfig& config,
                            const std::vector<BoardPos>& tiles,
                            float opacity = 200.0f/255.0f,
                            const std::vector<BoardPos>& alt_blob = {},
                            SDL_FColor color = TileColor::ATTACK_CURRENT);
```

### Step 2.3: Add color parameter to render_attack_blob definition

**File:** `src/grid_renderer.cpp`

**Find this exact pattern (function signature):**
```cpp
void GridRenderer::render_attack_blob(const RenderConfig& config,
                                       const std::vector<BoardPos>& tiles,
                                       float opacity,
                                       const std::vector<BoardPos>& alt_blob) {
    if (tiles.empty()) return;

    SDL_FColor color = TileColor::ATTACK_CURRENT;  // Yellow #FFD900
    color.a *= opacity;
```

**Replace with:**
```cpp
void GridRenderer::render_attack_blob(const RenderConfig& config,
                                       const std::vector<BoardPos>& tiles,
                                       float opacity,
                                       const std::vector<BoardPos>& alt_blob,
                                       SDL_FColor color) {
    if (tiles.empty()) return;

    color.a *= opacity;
```

### Step 2.4: Add enemy attack preview in hover block

**File:** `src/main.cpp`

**Find this exact pattern:**
```cpp
    // 4. Hover highlight
    if (state.hover_valid) {
        state.grid_renderer.render_hover(config, state.hover_pos);
```

**Replace with:**
```cpp
    // 4. Hover highlight
    if (state.hover_valid) {
        // Enemy attack preview: show attack range when hovering enemy (no unit selected)
        if (state.selected_unit_idx < 0 && state.turn_phase == TurnPhase::PlayerTurn) {
            int hovered_idx = find_unit_at_pos(state, state.hover_pos);
            if (hovered_idx >= 0 && state.units[hovered_idx].type == UnitType::Enemy) {
                const Entity& enemy = state.units[hovered_idx];
                auto attack_preview = get_attack_pattern(enemy.board_pos, enemy.attack_range);
                state.grid_renderer.render_attack_blob(config, attack_preview,
                                                        TileOpacity::FULL, {},
                                                        TileColor::ENEMY_ATTACK);
            }
        }
        state.grid_renderer.render_hover(config, state.hover_pos);
```

### Step 2.5: Verify build

```bash
./build.fish build
```

**Expected:** Build succeeds with 0 errors.

---

## Execution Order

```
P1: Ownership Indicator
├── 1.1 Insert is_enemy_idle lambda + loop after render_floor_grid
├── 1.2 Remove old hover-only indicator code
├── 1.3 Update ENEMY_COLOR to #FF0000 @ 31%
└── 1.4 Run ./build.fish build (must pass)

P2: Enemy Attack Preview
├── 2.1 Add TileColor::ENEMY_ATTACK constant
├── 2.2 Add color parameter to hpp declaration
├── 2.3 Add color parameter to cpp definition
├── 2.4 Add enemy attack preview in hover block
└── 2.5 Run ./build.fish build (must pass)
```

**Critical:** Steps within each phase depend on each other. Complete P1 before starting P2.

---

## Verification Checklist

After implementation, verify:

| Test | Action | Expected |
|------|--------|----------|
| T1 | Run game, start level | Red tiles under ALL enemies |
| T2 | Hover over enemy | Red tile disappears on hovered enemy, red attack blob appears |
| T3 | Move hover away | Red tile reappears on enemy |
| T4 | Select friendly unit | Red tiles hidden on attackable enemies (reticles show instead) |

---

## File Change Summary

| File | Changes |
|------|---------|
| `src/main.cpp` | +27 lines (lambda + loop + preview), -8 lines (old hover code) |
| `include/grid_renderer.hpp` | +2 lines (ENEMY_ATTACK constant), +1 line (color param) |
| `src/grid_renderer.cpp` | +1 line (color param), -1 line (hardcoded color), color change in render_enemy_indicator |

**Net:** ~22 lines added

---

## Rollback

If build fails after any step, revert with:
```bash
git checkout src/main.cpp include/grid_renderer.hpp src/grid_renderer.cpp
```
