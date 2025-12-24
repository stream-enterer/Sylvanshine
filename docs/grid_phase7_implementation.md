# Phase 7: Grid Visual Polish — Implementation Guide

Verified and cross-referenced against:
- Sylvanshine codebase (`grid_renderer.cpp`, `main.cpp`, `build_assets.py`)
- Duelyst source (`app/view/Player.js`, `app/common/config.js`)
- Duelyst analysis knowledge base

---

## Verified Constants (Duelyst Source)

### Opacity Values (`app/common/config.js`)

| Constant | Value | Decimal | Line |
|----------|-------|---------|------|
| `TILE_SELECT_OPACITY` | 200 | 0.784 (78%) | 528 |
| `TILE_HOVER_OPACITY` | 200 | 0.784 (78%) | 529 |
| `TILE_DIM_OPACITY` | 127 | 0.498 (50%) | 530 |
| `TILE_FAINT_OPACITY` | 75 | 0.294 (29%) | 531 |
| `PATH_TILE_ACTIVE_OPACITY` | 150 | 0.588 (59%) | 538 |
| `TARGET_ACTIVE_OPACITY` | 200 | 0.784 (78%) | 547 |

### Timing Values (`app/common/config.js`)

| Constant | Value | Line |
|----------|-------|------|
| `FADE_FAST_DURATION` | 0.2s | 553 |
| `FADE_MEDIUM_DURATION` | 0.35s | 554 |
| `PULSE_MEDIUM_DURATION` | 0.7s | 557 |
| `TILESIZE` | 95px | - |

### Z-Order (`app/view/Player.js:48-56`)

| Name | Value | Use |
|------|-------|-----|
| `boardTileZOrder` | 1 | Floor grid |
| `moveTileZOrder` | 2 | Movement blob |
| `assistTileZOrder` | 3 | Assist tiles |
| `aggroTileZOrder` | 4 | Attack blob |
| `attackableTargetTileZOrder` | 5 | Attack reticle |
| `cardTileZOrder` | 6 | Card play tiles |
| `selectTileZOrder` | 7 | Selection box |
| `pathTileZOrder` | 8 | Movement path |
| `mouseOverTileZOrder` | 9 | Hover highlight |

### Colors (`app/common/config.js`)

| Constant | Value | Hex |
|----------|-------|-----|
| `AGGRO_COLOR` | `{r:255, g:217, b:0}` | #FFD900 |
| `MOUSE_OVER_COLOR` | `{r:255, g:255, b:255}` | #FFFFFF |
| `MOUSE_OVER_OPPONENT_COLOR` | `{r:210, g:40, b:70}` | #D22846 |

---

## Verified Asset Dimensions

### Source Assets (`app/resources/tiles/`)

| Asset | Dimensions | Duelyst Use |
|-------|------------|-------------|
| `tile_box.png` | 80×80 | Selection box (84.2% of 95px tile) |
| `tile_board.png` | 128×128 | Floor grid |
| `tile_hover.png` | 128×128 | Passive hover |
| `tile_glow.png` | 100×100 | Movement glow |
| `tile_target.png` | 128×128 | Target reticle |
| `tile_large.png` | 128×128 | Attack reticle |
| `tile_grid.png` | 128×128 | Enemy ownership indicator |
| `tile_merged_large_*.png` | 64×64 | Corner tiles (quarter-tile) |
| `tile_path_move_*.png` | 128×128 | Path segments |

### Generated Assets (`dist/resources/tiles/s1/`)

| Asset | Dimensions | Notes |
|-------|------------|-------|
| `select_box.png` | 48×48 | Rendered at 84.2% = 40px |
| `floor.png` | 48×48 | 1px margin (46px content) |
| `hover.png` | 48×48 | 1px margin |
| `glow.png` | 48×48 | Full tile |
| `target.png` | 48×48 | Full tile |
| `corner_*.png` | 24×24 | Quarter-tile |
| `path_*.png` | 48×48 | NEAREST interpolation |

---

## Completed Features

| Feature | Implementation | Verified Value | Reference |
|---------|----------------|----------------|-----------|
| Selection box rendering | `tile_box.png` at 84.2% tile size | `80/95 = 0.842` | `grid_renderer.cpp:331-333` |
| Selection box pulsing | 1.0→0.85 scale, 0.7s period | matches Duelyst | `main.cpp:1017-1020, 1177` |
| Path arrow opacity | 59% (150/255) | `PATH_TILE_ACTIVE_OPACITY` | `grid_renderer.cpp:549` |
| Path sprite hard edges | NEAREST interpolation | preserves Duelyst crisp look | `build_assets.py:692` |
| Glow tile | 20% opacity (50/255) | hardcoded in Duelyst | `grid_renderer.cpp:366` |
| Passive hover opacity | 29% (75/255) | `TILE_FAINT_OPACITY` | `grid_renderer.hpp:42` |
| Duelyst color scheme | Move=#F0F0F0, Attack=#FFD900 | verified | `grid_renderer.hpp:33-37` |
| Instant hover transitions | 0s fade when already on board | `Player.js:1604-1608` | `main.cpp:949` |
| Movement path rendering | All 7 segment types | includes FromStart variants | `grid_renderer.cpp:269-303` |
| Corner-based move blob | 5 corner variants | proper neighbor detection | `grid_renderer.cpp:435-507` |

---

## Implementation Status Check

### Feature 1: Attack Target Reticle — NOT IMPLEMENTED

**Duelyst has two separate concepts:**
1. **Attack Pattern** (`Player.js:1440-1460`): Yellow blob showing all tiles where attacks COULD land
   - Uses merged corner tiles like movement blob
   - Rendered at `aggroTileZOrder = 4`
2. **Attackable Targets** (`Player.js:1400-1431`): Specific enemy positions within range
   - Uses `TileMapLargeSprite` (`tile_large.png`)
   - Rendered at `attackableTargetTileZOrder = 5`
   - Sets enemy nodes as valid targets for highlighting

**Sylvanshine current implementation:**
```cpp
// grid_renderer.cpp:128-132
void GridRenderer::render_attack_range(const RenderConfig& config,
                                       const std::vector<BoardPos>& attackable_tiles) {
    for (const auto& tile : attackable_tiles) {
        render_highlight(config, tile, TileColor::ATTACK_CURRENT);  // Solid yellow quad
    }
}
```
- Only renders enemy positions (attackable targets)
- Uses solid quads instead of `tile_large.png` reticle
- **Missing**: Attack pattern blob (tiles around unit showing attack reach)

---

### Feature 2: Enemy Ownership Indicator — NOT IMPLEMENTED

**Duelyst:**
- `TileOpponentSprite` uses `tile_grid.png`
- Rendered under enemy units with opponent color (#D22846)
- Creates visual distinction between player and enemy units

**Sylvanshine:**
- No equivalent implementation
- Asset exists but not in pipeline or renderer

---

### Feature 3: Move/Attack Seam Sprites — NOT IMPLEMENTED

**Duelyst:**
- Uses `tile_merged_large_0_seam.png` at boundaries between move and attack blobs
- Detects when a corner borders both move and attack regions
- Creates visible dividing line

**Sylvanshine:**
- Asset in pipeline: `corner_0_seam.png` (24×24) ✓
- Seam detection logic: NOT implemented
- Currently move and attack blobs render independently without seam handling

---

### Feature 4: Z-Order Constants — IMPLICIT ONLY

**Duelyst:** Explicit constants in `Player.js:48-56`

**Sylvanshine:**
- Has `z_order_offset` in `sprite_properties.hpp` for sprites
- Tile z-order is implicit via render call sequence in `main.cpp:1144-1185`:
  ```
  1. render_floor_grid()      → equivalent to boardTileZOrder
  2. render_move_range_alpha() → equivalent to moveTileZOrder
  3. render_attack_range()     → equivalent to aggroTileZOrder
  4. render_select_box()       → equivalent to selectTileZOrder
  5. render_path()             → equivalent to pathTileZOrder
  6. render_hover()            → equivalent to mouseOverTileZOrder
  ```
- Current order matches Duelyst conceptually
- **Missing**: Explicit constants for documentation/sorting

---

### Feature 5: Bloom Activation — INFRASTRUCTURE ONLY

**Duelyst:** `CONFIG.BLOOM_DEFAULT = 0.7`

**Sylvanshine:**
- Pass infrastructure exists (`render_pass.cpp:111-112`):
  ```cpp
  "depth", "highpass", "blur", "bloom", "bloomCompositeA", "bloomCompositeB"
  ```
- Passes created at 0.5× resolution
- **NOT routed** — `gpu_renderer.cpp` renders directly to swapchain
- Would require pipeline restructuring to enable

---

### Feature 6: Card Play Tiles — NOT IMPLEMENTED (prerequisite missing)

Assets exist: `tile_card.png`, `tile_spawn.png`
Prerequisite: Card/summoning system not implemented

---

### Feature 7: Attack Path Arc — NOT IMPLEMENTED (prerequisite missing)

Prerequisite: Ranged unit system not implemented

---

## Remaining Tasks

### Tier 1: Core Polish (High Value, Low Complexity)

#### 1. Attack Target Reticle

**Goal:** Show `tile_large.png` on attackable enemy positions.

**Duelyst Implementation:**
- Sprite: `tile_large.png` (128×128)
- Z-order: `attackableTargetTileZOrder = 5`
- Color: `CONFIG.AGGRO_COLOR` (#FFD900)
- Opacity: `TILE_SELECT_OPACITY` (200/255 = 78%)
- Used via `TileMapLargeSprite` class

**Current State:**
- Asset exists: `app/resources/tiles/tile_large.png`
- NOT in build pipeline
- NOT rendered in Sylvanshine

**Implementation:**
1. Add to `build_assets.py` scaling:
   ```python
   # In generate_scaled_tiles()
   src = src_dir / 'tile_large.png'
   dst = dst_dir / 'attack_reticle.png'
   resize_image(src, dst, tile_size, tile_size)
   ```
2. Add texture handle to `GridRenderer`:
   ```cpp
   GPUTextureHandle attack_reticle;
   ```
3. Load in `GridRenderer::init()`:
   ```cpp
   attack_reticle = g_gpu.load_texture((prefix + "attack_reticle.png").c_str());
   ```
4. Add render function:
   ```cpp
   void render_attack_reticle(const RenderConfig& config, BoardPos pos);
   ```
5. Call for each attackable enemy in `render_single_pass()`

**Files:** `build_assets.py`, `grid_renderer.hpp`, `grid_renderer.cpp`, `main.cpp`

---

#### 2. Enemy Ownership Indicator

**Goal:** Show `tile_grid.png` under enemy units.

**Duelyst Implementation:**
- Sprite: `tile_grid.png` (128×128, 122×122 visible content)
- Color: opponent color (#D22846 red)
- Z-order: below unit, above floor (between board and move tiles)
- Used via `TileOpponentSprite` class

**Current State:**
- Asset exists: `app/resources/tiles/tile_grid.png`
- NOT in build pipeline
- NOT rendered in Sylvanshine

**Implementation:**
1. Add to `build_assets.py`:
   ```python
   src = src_dir / 'tile_grid.png'
   dst = dst_dir / 'enemy_indicator.png'
   resize_image(src, dst, tile_size, tile_size)
   ```
2. Add to `GridRenderer`:
   ```cpp
   GPUTextureHandle enemy_indicator;
   void render_enemy_indicator(const RenderConfig& config, BoardPos pos);
   ```
3. Render under each enemy unit before unit sprites

**Files:** `build_assets.py`, `grid_renderer.hpp`, `grid_renderer.cpp`, `main.cpp`

---

### Tier 2: Visual Refinement (Medium Value, Medium Complexity)

#### 3. Move/Attack Seam Sprites

**Goal:** Visible boundary line between movement blob (white) and attack blob (yellow).

**Duelyst Implementation:**
- Uses `tile_merged_large_0_seam.png` variant
- Seam detection: corner has no same-region neighbor but has different-region neighbor
- Creates visible dividing line at blob interface

**Current State:**
- Asset exists: `app/resources/tiles/tile_merged_large_0_seam.png`
- Asset in pipeline: `corner_0_seam.png` (24×24)
- Seam detection logic NOT implemented

**Implementation:**
1. Extend `get_corner_neighbors()` to accept two blob sets:
   ```cpp
   CornerNeighbors get_corner_neighbors(BoardPos pos, int corner,
                                        const std::vector<BoardPos>& move_blob,
                                        const std::vector<BoardPos>& attack_blob);
   ```
2. Add seam detection:
   ```cpp
   bool is_seam_corner(BoardPos pos, int corner,
                       const std::vector<BoardPos>& move_blob,
                       const std::vector<BoardPos>& attack_blob);
   ```
3. Use `corner_0_seam` texture when seam detected

**Complexity:** Medium — requires dual-blob adjacency analysis

**Files:** `grid_renderer.hpp`, `grid_renderer.cpp`

---

#### 4. Z-Order Constants

**Goal:** Explicit z-order values for documentation and potential dynamic sorting.

**Current State:** Render order is implicit via function call sequence

**Implementation:**
```cpp
// Add to grid_renderer.hpp
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

**Status:** Documentation only — current render order already matches

**Files:** `grid_renderer.hpp`

---

#### 5. Bloom Activation

**Goal:** Route rendering through bloom post-processing.

**Duelyst Implementation:**
- `CONFIG.BLOOM_DEFAULT = 0.7`
- Bright pixels bleed outward
- Makes tile edges softer/thicker

**Current State:**
- PassManager has bloom passes (`highpass`, `blur`, `bloom`, `bloomCompositeA/B`)
- Passes created at 0.5× resolution
- NOT routed — renders direct to swapchain

**Implementation:**
1. Render scene to offscreen FBO
2. Apply highpass filter (extract bright pixels)
3. Apply blur passes
4. Composite bloom with original
5. Output to swapchain

**Complexity:** High — requires render pipeline restructuring

**Files:** `gpu_renderer.cpp`, `render_pass.cpp`

**See:** `docs/duelyst_implementation_differences.md` for design rationale

---

### Tier 3: Future Features (Prerequisites Required)

#### 6. Card Play Tiles

**Prerequisites:** Card/summoning system

**Assets Available:**
- `tile_card.png` — Card play highlight
- `tile_spawn.png` — Spawn location

**Duelyst Implementation:**
- `TileCardSprite`, `TileSpawnSprite`
- Z-order: `cardTileZOrder = 6`
- Color: player=#FFD900, opponent=#D22846

---

#### 7. Attack Path Arc Animation

**Prerequisites:** Ranged unit implementation

**Duelyst Constants:**
```javascript
PATH_MOVE_DURATION = 1.5s       // Arc cycle time
PATH_FADE_DISTANCE = 40px       // Fade in/out distance
PATH_ARC_DISTANCE = 47.5px      // Arc height
```

**Speed modifier:** `clamp((TILESIZE * 2) / distance, 1.0, 1.5)`

---

## Implementation Priority

```
Immediate (this session):
├── 1. Attack target reticle (asset + render)
└── 2. Enemy ownership indicator (asset + render)

Next Session:
├── 3. Move/attack seam logic
└── 4. Z-order constants (documentation)

Deferred:
├── 5. Bloom activation
├── 6. Card play tiles (needs card system)
└── 7. Attack path arc (needs ranged units)
```

---

## Validation Checklist

### Attack Reticle
- [ ] Asset added to build pipeline
- [ ] Texture loads without error
- [ ] Yellow reticle on attackable enemies
- [ ] Correct z-order (above attack blob, below selection)
- [ ] Disappears on deselect

### Enemy Indicator
- [ ] Asset added to build pipeline
- [ ] Red grid under enemy units
- [ ] Correct z-order (below unit sprite)
- [ ] Not shown for player units

### Seam Sprites
- [ ] Visible line at move/attack boundary
- [ ] Only at actual interfaces
- [ ] Correct corner orientation

---

## File Reference

| File | Lines | Purpose |
|------|-------|---------|
| `grid_renderer.cpp` | 325-391 | Selection/glow/target rendering |
| `grid_renderer.cpp` | 435-507 | Corner tile system |
| `grid_renderer.cpp` | 509-550 | Path segment rendering |
| `grid_renderer.hpp` | 32-48 | Color/opacity constants |
| `main.cpp` | 912-999 | Tile animation system |
| `main.cpp` | 1144-1185 | Single-pass grid render |
| `build_assets.py` | 591-698 | Tile scaling pipeline |
| `app/view/Player.js` | 48-56 | Duelyst z-order constants |
| `app/common/config.js` | 528-561 | Duelyst tile config |

---

## Related Documents

- `docs/duelyst_implementation_differences.md` — Intentional deviations
- `docs/grid_system_design.md` — Original design specification
- `duelyst_analysis/summaries/grid_rendering.md` — Duelyst forensics
