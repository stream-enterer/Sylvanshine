# Enemy Visual Indicators Audit

Cross-reference of `enemy_visual_indicators_spec.md` against Sylvanshine implementation.

**Audit Date:** 2025-12-24
**Spec Version:** 1.0 (12 sections)
**Code Files Reviewed:** `src/main.cpp`, `src/grid_renderer.cpp`, `include/grid_renderer.hpp`

---

## Executive Summary

| System | Spec Status | Sylvanshine Status | Gap |
|--------|-------------|-------------------|-----|
| Ownership Indicator | Idle-state persistent | **Hover-only** | Inverted logic |
| Enemy Attack Preview | Red blob on hover | **Not implemented** | Missing |
| Opponent Turn Display | Network-synced | N/A (single player) | Not applicable |

---

## Section-by-Section Analysis

### Section 1: System Overview

| Feature | In Spec | In Sylvanshine | Notes |
|---------|---------|----------------|-------|
| Three distinct visual indicator systems | YES | PARTIAL | Only ownership indicator exists (partially) |

### Section 2: Ownership Indicator (`tile_grid.png`)

**2.1 Visual Properties**

| Property | Spec Value | Sylvanshine Value | Match |
|----------|------------|-------------------|-------|
| Texture | `tile_grid.png` (122x122) | `enemy_indicator.png` | Different asset name |
| Opacity (enemy) | 80/255 (31%) | 75/255 (29%) | Close |
| Color (enemy) | `#FF0000` pure red | `#D22846` (0.824, 0.157, 0.275) | Different (Sylvanshine uses aggro color) |
| Player opacity | 0 (hidden) | N/A | Correct (not shown for player) |

**Code reference:** `grid_renderer.cpp:615-616`
```cpp
constexpr SDL_FColor ENEMY_COLOR = {0.824f, 0.157f, 0.275f, 75.0f/255.0f};  // #D22846, 29%
```

**2.2 Visibility State Machine**

| Condition | Spec (Duelyst) | Sylvanshine | Status |
|-----------|---------------|-------------|--------|
| Shown when idle | YES | NO | **MISSING** |
| Hidden on hover | YES (via `setHovered`) | SHOWN on hover | **INVERTED** |
| Hidden on selection | YES | N/A (only on hover) | N/A |
| Hidden on valid target | YES | N/A | N/A |
| Hidden during movement | YES | N/A | N/A |
| Only when spawned | YES | N/A | N/A |

**Code reference:** `main.cpp:1276-1286`
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

**Gap:** Sylvanshine shows enemy indicator **only when hovering** the enemy. Duelyst shows it **always when idle**, hiding it on hover.

---

### Section 3: Enemy Preview Blob (Your Turn Hover)

| Feature | Spec | Sylvanshine | Status |
|---------|------|-------------|--------|
| Trigger: hover enemy, no unit selected | YES | NO | **MISSING** |
| Movement blob shown | NO (enemy) | N/A | N/A |
| Attack blob shown | YES (red #D22846) | NO | **MISSING** |
| Uses `AGGRO_OPPONENT_COLOR` | YES (#D22846) | N/A | N/A |

**Gap:** Sylvanshine has no enemy attack range preview on hover. When you hover an enemy with no unit selected, Duelyst shows the enemy's attack range as a red blob. Sylvanshine only shows the red indicator tile.

---

### Section 4: Opponent Action Display (Their Turn)

| Feature | Spec | Sylvanshine | Status |
|---------|------|-------------|--------|
| Network hover events | YES | NO | Not applicable |
| Network select events | YES | NO | Not applicable |
| Opponent selection box | YES | NO | Not applicable |
| Opponent path preview | YES | NO | Not applicable |

**Note:** Sylvanshine is single-player. Network visualization is not required.

---

### Section 5: TileOpponentSprite

| Feature | Spec | Sylvanshine | Status |
|---------|------|-------------|--------|
| `tile_opponent.png` asset | YES | NO | Not implemented |
| Passive enemy hover | YES | Partial | Uses `enemy_indicator.png` instead |

---

### Section 6: Selection Box Behavior

| Feature | Spec | Sylvanshine | Status |
|---------|------|-------------|--------|
| Player color (#FFFFFF) | YES | YES | Implemented |
| Opponent color (#D22846) | N/A | N/A | Single player |
| Pulsing animation (0.85-1.0) | YES | YES | Implemented |
| Pulse period (0.7s) | YES | YES | `PULSE_PERIOD = 0.7f` at line 1066 |

**Code reference:** `main.cpp:1269-1271`
```cpp
float pulse_scale = 0.85f + 0.15f * (0.5f + 0.5f * std::cos(state.target_pulse_phase * 2.0f * M_PI));
state.grid_renderer.render_select_box(config, dest, pulse_scale);
```

---

### Section 7: Color Reference Table

| Constant | Spec Value | Sylvanshine Value | Status |
|----------|------------|-------------------|--------|
| `SELECT_COLOR` | #FFFFFF | White (implicit) | Match |
| `MOVE_COLOR` | #F0F0F0 | `{0.941, 0.941, 0.941}` | Match |
| `AGGRO_COLOR` | #FFD900 | `{1.0, 0.851, 0.0}` | Match |
| `AGGRO_OPPONENT_COLOR` | #D22846 | Used for enemy indicator | Partial |
| `OPPONENT_OWNER_COLOR` | #FF0000 @ 31% | #D22846 @ 29% | Different |

**Code reference:** `grid_renderer.hpp:33-42`
```cpp
namespace TileColor {
    constexpr SDL_FColor MOVE_CURRENT   = {0.941f, 0.941f, 0.941f, 200.0f/255.0f};
    constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 0.851f, 0.0f, 200.0f/255.0f};
    constexpr SDL_FColor PATH  = {1.0f, 1.0f, 1.0f, 150.0f/255.0f};
    constexpr SDL_FColor HOVER = {1.0f, 1.0f, 1.0f, 75.0f/255.0f};
}
```

---

### Section 10: Complete Tile Sprite Catalogue

| Sprite | Spec Asset | Sylvanshine Asset | Status |
|--------|------------|-------------------|--------|
| `TileMapHoverSprite` | `tile_hover.png` | `hover.png` | Implemented |
| `TileBoxSprite` | `tile_box.png` | `select_box.png` | Implemented |
| `TileMapLargeSprite` | `tile_large.png` | `attack_reticle.png` | Implemented |
| `TileMapGridSprite` | `tile_grid.png` | `enemy_indicator.png` | Implemented (different behavior) |
| `TileGlowSprite` | `tile_glow.png` | `glow.png` | Implemented |
| `TileAttackSprite` | `tile_attack.png` | N/A | Not implemented |
| `TileSpawnSprite` | `tile_spawn.png` | N/A | Not implemented |
| `TileSpellSprite` | `tile_spell.png` | N/A | Not implemented |
| `TileCardSprite` | `tile_card.png` | N/A | Not implemented |
| `TileOpponentSprite` | `tile_opponent.png` | N/A | Not implemented |

**Merged tile corners:**

| Pattern | Spec | Sylvanshine | Status |
|---------|------|-------------|--------|
| `corner_0` | YES | YES | `corner_0.png` |
| `corner_01` | YES | YES | `corner_01.png` |
| `corner_03` | YES | YES | `corner_03.png` |
| `corner_013` | YES | YES | `corner_013.png` |
| `corner_0123` | YES | YES | `corner_0123.png` |
| `corner_0_seam` | YES | YES | `corner_0_seam.png` |

**Path tiles:**

| Sprite | Spec | Sylvanshine | Status |
|--------|------|-------------|--------|
| `path_start` | YES | YES | Implemented |
| `path_straight` | YES | YES | Implemented |
| `path_corner` | YES | YES | Implemented |
| `path_end` | YES | YES | Implemented |
| `*_from_start` variants | YES | YES | Implemented |

---

### Section 11: Entity Visual State Tags

| Tag | Spec | Sylvanshine | Status |
|-----|------|-------------|--------|
| Targetable highlight | YES | Partial | Attack reticle only |
| Deemphasis (dim non-active) | YES | NO | Not implemented |
| Dissolve (death) | YES | NO | Uses FX instead |
| Readiness indicator | YES | NO | Not implemented |
| Hover for player/opponent | YES | NO | Not implemented as separate system |
| Selection glow | YES | Partial | Selection box only |
| Instructional glow | YES | NO | Not implemented |
| Pulsing highlight | YES | Partial | Target pulse only |

---

### Section 12: Bot vs Network Game Differences

Sylvanshine is single-player, so all network-related features are not applicable.

**Relevant for Sylvanshine:**
- Ownership indicator: HIGH relevance (shows enemy units at a glance)
- Enemy attack preview on hover: HIGH relevance (helps player plan attacks)
- Opponent turn visualization: DESIGN CHOICE (could add "enemy thinking" polish)

---

## Feature Implementation Status

### Fully Implemented

1. **Movement blob** with merged tile corners and seam detection
2. **Attack blob** (yellow #FFD900) with corner merging
3. **Path rendering** with directional segments
4. **Selection box** with 0.85-1.0 pulsing animation
5. **Hover tile** at cursor position
6. **Glow tile** at movement destination
7. **Attack reticle** on attackable enemies

### Partially Implemented

1. **Enemy indicator** - Exists but logic is inverted (hover-only vs idle-state)

### Not Implemented

1. **Idle-state ownership indicator** - Enemy red tile should be persistent, hidden on interaction
2. **Enemy attack preview blob** - Red attack range on enemy hover
3. **Entity visual state tags** - Deemphasis, readiness, instructional glow
4. **Tile spawn/spell/card sprites** - Not needed yet

---

## Recommended Changes

### Priority 1: Fix Ownership Indicator Logic

**Current:** Shows red tile only when hovering enemy
**Spec:** Shows red tile always when enemy is idle, hides on hover

```cpp
// CURRENT (main.cpp:1276-1286) - WRONG
if (state.hover_valid && state.selected_unit_idx < 0) {
    if (unit.type == UnitType::Enemy && unit.board_pos == state.hover_pos) {
        render_enemy_indicator(...);  // Only on hover
    }
}

// CORRECT implementation:
for (const auto& unit : state.units) {
    if (unit.type == UnitType::Enemy && !unit.is_dead()) {
        bool is_idle = (unit.board_pos != state.hover_pos) &&
                       (state.selected_unit_idx < 0 ||
                        state.units[state.selected_unit_idx].board_pos != unit.board_pos) &&
                       !unit.is_moving();
        if (is_idle) {
            render_enemy_indicator(config, unit.board_pos);
        }
    }
}
```

### Priority 2: Add Enemy Attack Preview on Hover

When hovering an enemy unit with no friendly unit selected, show the enemy's attack range as a red blob.

```cpp
if (state.hover_valid && state.selected_unit_idx < 0) {
    int hovered_idx = find_unit_at_pos(state, state.hover_pos);
    if (hovered_idx >= 0 && state.units[hovered_idx].type == UnitType::Enemy) {
        auto attack_range = get_attack_pattern(
            state.hover_pos,
            state.units[hovered_idx].attack_range
        );
        // Render with AGGRO_OPPONENT_COLOR (#D22846)
        grid_renderer.render_attack_blob_enemy(config, attack_range);
    }
}
```

---

## Asset Gaps

| Asset | Needed | Exists in dist/ | Notes |
|-------|--------|----------------|-------|
| `enemy_indicator.png` | YES | YES | Already implemented |
| Enemy attack blob corners | YES | NO | Need red-tinted corner variants |

---

## Audit Complete

**Total spec sections:** 12
**Fully aligned:** 4 (Sections 6, 7 partial, 10 partial, 12)
**Partially aligned:** 5 (Sections 1, 2, 3, 5, 11)
**Not applicable:** 3 (Sections 4, 8.3, 12 network features)
