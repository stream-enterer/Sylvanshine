# Phase 7: Animation & Polish (Boilerplate)

Out-of-scope items from Phase 6 visual analysis. Implement after Phase 6 is complete and validated.

## Goals

1. **Selection box** — Pulsing outline at selected unit position
2. **Instant hover transitions** — No fade when moving within board
3. **Pulsing scale animations** — Target reticle, selection box
4. **Attack range polish** — Yellow color, attack reticle

---

## Prerequisites

- Phase 6 complete and validated
- Assets: `tile_box.png`, `tile_large.png` (attack target)

---

## 1. Selection Box

### Ground Truth

**Source:** `tile_interaction_flow.md` §12.1, §12.2

```javascript
// TileBoxSprite at selected unit position
showBoxTileForSelect() → TileBoxSprite
  - Position: selected unit's board position
  - Z-order: selectTileZOrder (7)
  - Animation: pulsing scale (100% ↔ 85%)

_showPulsingScaleTile(tile, position, opacity, fadeDuration, color, scale) {
  if (scale == null) { scale = 0.85; }
  tile.setScale(1.0);
  tile.startPulsingScale(CONFIG.PULSE_MEDIUM_DURATION, scale);
}
```

### Implementation

1. Load `tile_box.png` in `GridRenderer::init()`
2. Add `render_selection_box(config, pos)` method
3. Add pulsing scale animation system
4. Call from `render_single_pass()` when unit selected

---

## 2. Instant Hover Transitions

### Ground Truth

**Source:** `grid_rendering.md` §22.3, `tile_interaction_flow.md` §8.3

```javascript
// Player.js:1604-1610
if (wasHoveringOnBoard) {
  hoverFadeDuration = 0.0;    // INSTANT when already on board
} else {
  hoverFadeDuration = CONFIG.FADE_FAST_DURATION;  // 0.2s
}
```

**Behavior:**
- Mouse moves between tiles within board → instant transition (no fade)
- Mouse enters board from outside → fade in (0.2s)
- Mouse exits board → fade out (0.2s)

### Implementation

1. Add `was_hovering_on_board` state to GameState
2. Track board entry/exit transitions
3. Set fade duration based on transition type:
   ```cpp
   float fade_duration = was_hovering_on_board ? 0.0f : 0.2f;
   ```

---

## 3. Pulsing Scale Animation

### Ground Truth

**Source:** `tile_interaction_flow.md` §12.2

```javascript
// CONFIG.PULSE_MEDIUM_DURATION = varies (typically 0.5-1.0s)
tile.startPulsingScale(CONFIG.PULSE_MEDIUM_DURATION, 0.85);

// Scale oscillates: 1.0 → 0.85 → 1.0 → 0.85 ...
```

**Applied to:**
- Selection box (TileBoxSprite)
- Target reticle (optional)
- Spawn tiles (TileSpawnSprite)
- Spell tiles (TileSpellSprite)

### Implementation

1. Add `PulsingScale` animation struct:
   ```cpp
   struct PulsingScale {
       float min_scale = 0.85f;
       float max_scale = 1.0f;
       float period = 0.5f;  // seconds per cycle
       float elapsed = 0.0f;

       float current_scale() const {
           float t = elapsed / period;
           float phase = std::sin(t * 2.0f * M_PI);
           return min_scale + (max_scale - min_scale) * (0.5f + 0.5f * phase);
       }
   };
   ```

2. Apply scale in render functions

---

## 4. Attack Range Polish

### Ground Truth

**Source:** `grid_rendering.md` §8

| Constant | Value | Hex | Usage |
|----------|-------|-----|-------|
| `CONFIG.AGGRO_COLOR` | 255, 217, 0 | #FFD900 | Attack range (friendly) |
| `CONFIG.AGGRO_OPPONENT_COLOR` | 210, 40, 70 | #D22846 | Attack range (enemy) |

**Attack target reticle:**
- Sprite: `tile_large.png`
- Class: `TileMapLargeSprite`
- Z-order: attackableTargetTileZOrder (5)

### Implementation

1. Update `TileColor::ATTACK_CURRENT` to yellow:
   ```cpp
   constexpr SDL_FColor ATTACK_CURRENT = {1.0f, 0.851f, 0.0f, 200.0f/255.0f};  // #FFD900
   ```

2. Load `tile_large.png` for attack target reticle
3. Render at attackable enemy positions

---

## 5. Z-Order Constants

### Ground Truth

**Source:** `tile_interaction_flow.md` §2

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

### Implementation

Currently implicit in render order. Consider:
1. Add z-order constants to `grid_renderer.hpp`
2. Document render order explicitly
3. (Optional) Sort by z-order if dynamic rendering needed

---

## Files to Modify

| File | Changes |
|------|---------|
| `include/grid_renderer.hpp` | Add selection box, pulsing animation, z-order constants |
| `src/grid_renderer.cpp` | Add render_selection_box, pulsing logic |
| `src/main.cpp` | Add was_hovering_on_board tracking, selection box rendering |
| `include/types.hpp` | (Optional) Add TileZOrder namespace |

---

## Validation Checklist

### Selection Box
- [ ] White outline appears at selected unit
- [ ] Box pulses smoothly (100% → 85% → 100%)
- [ ] Box disappears on deselection

### Instant Hover
- [ ] Moving mouse within board = no visible fade
- [ ] Entering board from outside = smooth fade in
- [ ] Exiting board = smooth fade out

### Pulsing Animation
- [ ] Selection box pulses
- [ ] (Optional) Target reticle pulses
- [ ] Animation timing feels smooth

### Attack Range
- [ ] Attack range is yellow (#FFD900), not red
- [ ] Attack target reticle appears on enemies in range

---

## Reference

See `docs/grid_visual_issues.md` for:
- Issue 4: Missing Animation/Timing Behaviors
- Issue 5: Missing Z-Order Hierarchy
- Complete ground truth references
