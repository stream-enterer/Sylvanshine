# Phase 7: Grid Visual Polish — Implementation Guide

Consolidated from: `grid_phase7_plan.md`, `grid_visual_issues.md`, `out_of_scope/*.md`, `KANBAN.md`

---

## Completed (Prior Sessions)

| Feature | Implementation | Reference |
|---------|----------------|-----------|
| Selection box rendering | `tile_box.png` with 7.9% inset | `grid_renderer.cpp:325` |
| Selection box pulsing | 0.85–1.0 scale @ 0.7s period | `main.cpp:1017` |
| Path arrow opacity | 59% (150/255) | `grid_renderer.cpp:497` |
| Path sprite hard edges | NEAREST interpolation | `build_assets.py:529` |
| Glow tile | 20% opacity under movement target | `grid_renderer.cpp:349` |
| Passive hover opacity | 29% (75/255) | `grid_renderer.hpp:42` |
| Duelyst color scheme | Movement=#F0F0F0, Attack=#FFD900 | Already applied |

---

## Remaining Tasks

### Tier 1: Core Polish (High Value, Low-Medium Complexity)

#### 1. Instant Hover Transitions
**Goal:** No fade when mouse moves between tiles within board; fade only on enter/exit.

**Duelyst Behavior:**
```javascript
// Player.js:1604-1610
if (wasHoveringOnBoard) {
  hoverFadeDuration = 0.0;    // INSTANT when already on board
} else {
  hoverFadeDuration = CONFIG.FADE_FAST_DURATION;  // 0.2s
}
```

**Implementation:**
1. GameState already has `was_hovering_on_board` — verify it's being used
2. Check `update_hover_state()` sets fade duration based on transition type
3. Test: move mouse across board = no visible fade; enter/exit board = smooth fade

**Files:** `src/main.cpp` (update_hover_state)

---

#### 2. Attack Target Reticle
**Goal:** Show `tile_large.png` on attackable enemy positions.

**Duelyst Behavior:**
- Sprite: `tile_large.png` (128×128)
- Z-order: `attackableTargetTileZOrder = 5`
- Color: `CONFIG.AGGRO_COLOR` (#FFD900 yellow)
- Shown on: enemy units within attack range

**Assets Available:**
- `app/resources/tiles/tile_large.png` (needs adding to pipeline)

**Implementation:**
1. Add `tile_large.png` to `build_assets.py` scaling pipeline
2. Add `attack_reticle` texture handle to GridRenderer
3. Add `render_attack_reticle(config, pos)` function
4. Call for each attackable enemy position in render loop

**Files:** `build_assets.py`, `grid_renderer.hpp`, `grid_renderer.cpp`, `main.cpp`

---

#### 3. Enemy Ownership Indicator
**Goal:** Show `tile_grid.png` under enemy units to indicate ownership.

**Duelyst Behavior:**
- Sprite: `tile_grid.png` (122×122 content in 128×128)
- Color: opponent color (`#D22846` red)
- Z-order: below unit, above floor

**Assets Available:**
- `app/original_resources/tiles/tiles_board.plist` → `tile_grid.png` at {{2,126},{122,122}}

**Implementation:**
1. Extract/add `tile_grid.png` to asset pipeline
2. Render under each enemy unit before unit sprite
3. Tint with opponent color

**Files:** `build_assets.py`, `grid_renderer.cpp`, `main.cpp`

---

### Tier 2: Visual Refinement (Medium Value, Medium Complexity)

#### 4. Move/Attack Seam Sprites
**Goal:** Visible boundary between movement blob (white) and attack blob (yellow).

**Duelyst Behavior:**
- Uses `tile_merged_large_0_seam.png` variant
- Seam detection: corner has no same-region neighbor but has different-region neighbor
- Creates visible dividing line between blob types

**Assets Available:**
```
tile_merged_large_0_seam.png
tile_merged_hover_0_seam.png
```

**Implementation:**
1. Add seam detection logic to corner rendering
2. For each corner tile, check if it borders both move and attack regions
3. If seam detected, use `_seam` variant sprite

**Complexity:** Medium — requires adjacency analysis between two blob types

**Files:** `grid_renderer.cpp` (render_move_range, corner logic)

---

#### 5. Z-Order Constants
**Goal:** Explicit z-order values instead of implicit render order.

**Duelyst Z-Order:**
```cpp
namespace TileZOrder {
    constexpr int BOARD = 1;              // Floor grid
    constexpr int MOVE_RANGE = 2;         // Movement blob
    constexpr int ASSIST = 3;             // Assist tiles (future)
    constexpr int ATTACK_RANGE = 4;       // Attack blob
    constexpr int ATTACKABLE_TARGET = 5;  // Attack reticle
    constexpr int CARD = 6;               // Card play tiles (future)
    constexpr int SELECT = 7;             // Selection box
    constexpr int PATH = 8;               // Movement path
    constexpr int HOVER = 9;              // Hover highlight
}
```

**Implementation:**
1. Add `TileZOrder` namespace to `grid_renderer.hpp`
2. Document current render order matches this hierarchy
3. (Optional) Add z-order sorting if dynamic layering needed

**Files:** `grid_renderer.hpp`, `main.cpp` (documentation)

---

#### 6. Bloom Activation
**Goal:** Route rendering through bloom post-processing passes.

**Current Status:**
- PassManager has bloom passes: `highpass`, `blur`, `bloom`, `bloomCompositeA`, `bloomCompositeB`
- Passes created at 0.5× resolution
- NOT currently routed — renders direct to swapchain

**Duelyst Behavior:**
- `CONFIG.BLOOM_DEFAULT = 0.7`
- Bright pixels "bleed" outward
- Makes tile edges appear softer/thicker

**Implementation:**
1. Render scene to offscreen FBO instead of swapchain
2. Apply highpass filter (extract bright pixels)
3. Apply blur passes
4. Composite bloom with original scene
5. Output to swapchain

**Complexity:** High — requires render pipeline restructuring

**Files:** `gpu_renderer.cpp`, `render_pass.cpp`, possibly new bloom shader

**Reference:** `docs/duelyst_implementation_differences.md`

---

### Tier 3: Future Features (Low Priority, High Complexity)

#### 7. Card Play Tiles
**Goal:** Tile highlights for summoning units from cards.

**Duelyst Behavior:**
- `TileCardSprite`, `TileSpawnSprite`
- Color: player=#FFD900, opponent=#D22846
- Z-order: `cardTileZOrder = 6`

**Assets:**
- `tile_card.png` — card play highlight
- `tile_spawn.png` — spawn location

**Prerequisite:** Card/summoning system exists

---

#### 8. Attack Path Arc Animation
**Goal:** Animated sine-wave projectile trajectory for ranged attacks.

**Duelyst Behavior:**
```
PATH_MOVE_DURATION = 1.5s       // Arc cycle time
PATH_FADE_DISTANCE = 40px       // Fade in/out distance
PATH_ARC_DISTANCE = 47.5px      // Arc height
```

- Speed modifier: `clamp((TILE_SIZE * 2) / distance, 1.0, 1.5)`
- Arc motion: `sin(arc_pct * PI)`

**Prerequisite:** Ranged unit implementation

---

## Implementation Order

```
Phase 7a (Immediate):
├── 1. Instant hover transitions (verify/fix)
├── 2. Attack target reticle
└── 3. Enemy ownership indicator

Phase 7b (Polish):
├── 4. Move/attack seam sprites
├── 5. Z-order constants (documentation)
└── 6. Bloom activation (if desired)

Phase 7c (Future):
├── 7. Card play tiles (when card system exists)
└── 8. Attack path arc (when ranged units exist)
```

---

## Validation Checklist

### Instant Hover
- [ ] Moving mouse within board = no visible fade
- [ ] Entering board from outside = smooth fade in
- [ ] Exiting board = smooth fade out

### Attack Reticle
- [ ] Yellow reticle appears on attackable enemies
- [ ] Reticle matches tile size
- [ ] Disappears when unit deselected

### Enemy Indicator
- [ ] Red grid tile under enemy units
- [ ] Visible but not overwhelming
- [ ] Correct z-order (below unit sprite)

### Seam Sprites
- [ ] Visible line between move and attack blobs
- [ ] Only appears at actual boundaries
- [ ] Correct orientation

### Bloom
- [ ] Bright tiles have soft glow
- [ ] Performance acceptable
- [ ] Toggle option available

---

## File Reference

| File | Purpose |
|------|---------|
| `src/grid_renderer.cpp` | Tile rendering functions |
| `include/grid_renderer.hpp` | Texture handles, constants |
| `src/main.cpp` | Game state, render loop |
| `build_assets.py` | Asset pipeline |
| `src/gpu_renderer.cpp` | Render passes (for bloom) |
| `src/render_pass.cpp` | Pass infrastructure |

---

## Related Documents

- `docs/duelyst_implementation_differences.md` — architectural decisions and deviations from Duelyst
