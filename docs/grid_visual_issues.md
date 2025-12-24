# Grid Visual Issues: Duelyst vs Sylvanshine Analysis

Comparative analysis of visual discrepancies with root cause tracing.

---

## Issue 1: Movement Arrow is Thin

### Visual Comparison
| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Arrow body | Thick, semi-transparent with border | Thin line |
| Arrow head | Prominent, 60px tall | Small |
| Overall presence | High visual weight | Low visual weight |

### Ground Truth (from duelyst_analysis)

**Source:** `grid_rendering.md` §13 (Complete Tile Frame Table)

| Sprite | Source Size | Content Size | Offset |
|--------|-------------|--------------|--------|
| `tile_path_move_start.png` | 128×128 | 71×12 | {28.5, 0} |
| `tile_path_move_straight.png` | 128×128 | 128×12 | {0, 0} |
| `tile_path_move_end.png` | 128×128 | 82×60 | {-23, 0} |
| `tile_path_move_corner.png` | 128×128 | 70×70 | {-29, -29} |

**Scaling in Duelyst:**
- CONFIG.TILESIZE = 95px
- Sprites are 128×128, scaled to 95×95
- Ratio: 95/128 = 0.742 (74.2%)

**Scaling in Sylvanshine:**
- TILE_SIZE = 48px (scale=1) or 96px (scale=2)
- At scale=1: 48/128 = 0.375 (37.5% — nearly half of Duelyst)
- At scale=2: 96/128 = 0.75 (75% — close to Duelyst)

### Root Cause

1. **Scale=1 renders arrows at half the relative size of Duelyst**
   - Arrow content that's 12px tall in 128px sprite becomes ~4.5px at scale=1
   - Barely visible at lower resolution

2. **Missing "FromStart" sprite variants**
   - Ground truth (`tile_interaction_flow.md` §8.4) shows 9 different path sprite classes
   - We only use 4: start, straight, corner, end
   - Missing: `*FromStart` variants for visual continuity from unit

### Implementation Path That Led to Problem

```
Phase 3 Implementation:
  ├─ Read design spec (not ground truth)
  ├─ Implemented basic path sprites (4 types)
  ├─ Did NOT consult duelyst_analysis for sprite variants
  └─ Did NOT account for scaling ratio differences
```

### Solution

1. **Pre-scale path sprites at build time** (like corner tiles in Phase 6)
2. **Add FromStart sprite variants**
3. **Consider @2x sprites** for scale=2 mode

---

## Issue 2: Missing Target Tile Reticle

### Visual Comparison
| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Move target | TileGlowSprite (bright glow) | Simple highlight |
| Attack target | TileAttackSprite (reticle) | Not implemented |
| Hover feedback | Distinct tile types | Generic highlight |

### Ground Truth

**Source:** `tile_interaction_flow.md` §12.1

| Function | Sprite Class | Behavior |
|----------|-------------|----------|
| `showTargetTile()` | `TileAttackSprite` | Target reticle |
| `showGlowTileForAction()` | `TileGlowSprite` | Move target glow |
| `showBoxTileForAction()` | `TileBoxSprite` | Pulsing action indicator |

**Available Assets:**
- `tile_hover.png` (128×128) — used for passive hover
- `tile_large.png` (128×128) — attackable target
- `tile_glow.png` — move destination
- `tile_box.png` — selection box outline

### Root Cause

**Not implemented at all.** Phase 1-5 only implemented:
- Basic hover highlight (solid color quad)
- Movement path arrow

Did NOT implement:
- Target reticle at path destination
- Glow effect for move targets
- Pulsing scale animation

### Implementation Path That Led to Problem

```
Design spec (grid_system_design.md):
  ├─ Focused on movement blob and path
  ├─ Did NOT specify target reticle requirement
  └─ Hover was documented as simple highlight
```

### Solution

Add target tile rendering:
1. Load `tile_glow.png` or equivalent
2. Render at path destination when hovering valid move tile
3. (Optional) Add pulsing scale animation

---

## Issue 3: Triangle Artifacts in Movement Blob

### Visual Comparison
| Aspect | Duelyst | Sylvanshine |
|--------|---------|-------------|
| Corner blending | Smooth, seamless | Visible triangular seams |
| Edge gradient | Correct orientation | Misaligned (not rotated) |
| Interior fill | Consistent | Inconsistent patterns |

### Ground Truth

**Source:** `grid_rendering.md` §7 (Rotation Values)

| Corner | Rotation |
|--------|----------|
| Top-Left (tl) | 0° |
| Top-Right (tr) | 90° |
| Bottom-Right (br) | 180° |
| Bottom-Left (bl) | 270° |

**Critical detail:** Each corner sprite has a gradient baked in that flows from opaque exterior edge to semi-transparent interior. Rotation ensures gradients face outward from blob.

### Root Cause

**Phase 5 `render_corner_quad()` does NOT rotate corner sprites.**

Current implementation:
```cpp
void GridRenderer::render_corner_quad(...) {
    // Places sprite at correct position
    // Does NOT apply rotation
    Vec2 tl = transform_board_point(config, x, y);
    Vec2 tr = transform_board_point(config, x + half, y);
    // ... renders unrotated quad
}
```

Required:
```cpp
// Rotate each corner: TL=0°, TR=90°, BR=180°, BL=270°
float rotation_deg = corner * 90.0f;
// Apply rotation before perspective transform
```

### Implementation Path That Led to Problem

```
Phase 5 Implementation:
  ├─ Correctly identified corner sprite selection algorithm
  ├─ Correctly implemented neighbor detection
  ├─ MISSED rotation requirement from ground truth
  └─ Did not visually test against Duelyst reference
```

### Solution

**Phase 6 addresses this** with `render_corner_quad_rotated()`.

---

## Summary Table

| Issue | Root Cause | Phase 6 Fixes? | Additional Work |
|-------|-----------|----------------|-----------------|
| Thin arrow | Scaling ratio + missing FromStart | **Yes** | — |
| No target reticle | Not implemented | **Yes** | — |
| Triangle artifacts | Missing corner rotation | **Yes** | — |
| Missing animations | Not implemented | No | Phase 7 |
| Z-order hierarchy | Implicit only | Partial | Phase 7 |

---

## Issue 4: Missing Animation/Timing Behaviors

### Ground Truth (duelyst_analysis)

**Fade Duration Constants:**
| Constant | Value | Usage |
|----------|-------|-------|
| `CONFIG.FADE_FAST_DURATION` | 0.2s | Quick transitions (hover, path) |
| `CONFIG.FADE_MEDIUM_DURATION` | 0.35s | Standard transitions |
| `CONFIG.FADE_SLOW_DURATION` | 1.0s | Slow transitions |

**Opacity Constants:**
| Constant | Value | Normalized | Usage |
|----------|-------|------------|-------|
| `CONFIG.TILE_SELECT_OPACITY` | 200 | 0.784 | Selected tile |
| `CONFIG.TILE_HOVER_OPACITY` | 200 | 0.784 | Hover tile |
| `CONFIG.TILE_DIM_OPACITY` | 127 | 0.498 | Dimmed blob during hover |
| `CONFIG.TILE_FAINT_OPACITY` | 75 | 0.294 | Passive hover |

**Hover Transition Timing:**
- **Within board** (wasHoveringOnBoard = true): `fadeDuration = 0.0` (INSTANT)
- **Entering board**: `fadeDuration = 0.2s`
- **Exiting board**: `fadeDuration = 0.2s`

**Pulsing Scale Animation:**
- Selection box pulses at 85% scale (`CONFIG.PULSE_MEDIUM_DURATION`)
- Target reticle pulses similarly

### Current Sylvanshine Status

| Feature | Implemented | Match Duelyst |
|---------|-------------|---------------|
| Fade durations | Partial | TileFadeAnim exists |
| Instant hover within board | No | Always fades |
| Blob dim on hover | Yes | Matches |
| Blob restore on hover exit | Yes | Matches |
| Pulsing scale | No | Not implemented |
| Selection box | No | Not implemented |

### Root Cause

Phase 1 implemented TileFadeAnim but:
- Does not distinguish "within board" vs "entering board"
- Missing instant transition logic
- Missing pulsing scale animations

---

## Issue 5: Missing Z-Order Hierarchy

### Ground Truth (tile_interaction_flow.md §2)

```
boardTileZOrder: 1      ← Floor grid
moveTileZOrder: 2       ← Movement range blob
assistTileZOrder: 3     ← Assist tiles
aggroTileZOrder: 4      ← Attack range blob
attackableTargetTileZOrder: 5
cardTileZOrder: 6
selectTileZOrder: 7     ← Selection box
pathTileZOrder: 8       ← Movement path arrow
mouseOverTileZOrder: 9  ← Hover highlight
```

### Current Sylvanshine Status

Render order is implicit in `render_single_pass()`:
1. Grid lines
2. Move range
3. Attack range
4. Path
5. Hover
6. Units

**Missing:**
- Explicit z-order values
- Floor grid (now in Phase 6)
- Selection box
- Proper layering verification

---

## Scope Classification

| Feature | In Phase 6 | Notes |
|---------|------------|-------|
| Corner rotation | ✅ Yes | Fixes triangle artifacts |
| Floor grid | ✅ Yes | Adds Duelyst-style gaps |
| Path arrow scaling | ✅ Yes | Pre-scaled sprites |
| FromStart variants | ✅ Yes | Added to path system |
| Target glow tile | ✅ Yes | Renders at path end |
| Instant hover transition | ❌ No | Future enhancement |
| Pulsing scale animation | ❌ No | Future enhancement |
| Selection box | ❌ No | Future enhancement |
| Z-order constants | ❌ No | Currently implicit |

---

## Future Enhancements (Post Phase 6)

### Phase 7 Candidates

1. **Selection Box**
   - TileBoxSprite at selected unit
   - Pulsing scale animation (85%)
   - White outline appearance

2. **Instant Hover Transitions**
   - Track `wasHoveringOnBoard` state
   - Set `fadeDuration = 0.0` for within-board moves
   - Smoother visual experience

3. **Pulsing Animations**
   - Scale oscillation: 100% ↔ 85%
   - Applied to: selection box, target reticle, spawn tiles

4. **Attack Range Enhancements**
   - Yellow color (CONFIG.AGGRO_COLOR = #FFD900)
   - Attack target reticle (tile_large.png)
   - Attack path arc animation (future)
