# Duelyst Grid System Specification

Technical specification of Duelyst's grid rendering implementation based on forensic analysis.

**Forensic Sources:**
- `duelyst_analysis/summaries/grid_rendering.md`
- `duelyst_analysis/flows/tile_interaction_flow.md`
- `app/view/layers/game/TileLayer.js`
- `app/common/config.js`

---

## 1. Grid Visualization Method

### 1.1 Primary Approach: Negative Grid (Tile Gaps)

Duelyst renders the grid using **semi-opaque floor tiles with gaps between them**. This is a "negative grid" approach where the grid structure emerges from the spaces between tiles.

**NO explicit grid lines are drawn.** The visual grid pattern comes entirely from tile spacing.

### 1.2 Floor Tile Configuration

**Source:** `app/common/config.js:293-294`

```javascript
CONFIG.FLOOR_TILE_COLOR = { r: 0, g: 0, b: 0 };  // Black
CONFIG.FLOOR_TILE_OPACITY = 20;                   // 20/255 = 7.84%
```

### 1.3 Floor Tile Sprite (Ground Truth)

**Source:** `tile_board.png` analyzed via ImageMagick

| Property | Value |
|----------|-------|
| Sprite file size | 128×128 px |
| Visible content | 122×122 px |
| Bounding box | `122x122+3+3` |
| Transparent margin | **3px per side** |

### 1.4 Floor Gap Ratio (Scale-Independent)

```
Gap ratio = margin / sprite_size = 3 / 128 = 0.0234375 (2.34%)
Visible ratio = content / sprite_size = 122 / 128 = 0.953125 (95.31%)
```

This ratio is preserved regardless of tile size due to uniform scaling.

### 1.5 Scaling System

**Source:** `TileMapScaledSprite.js`

```javascript
this.setScale(CONFIG.TILESIZE / this.getContentSize().width);
// 95 / 128 = 0.7421875
```

**After scaling to 95px tile:**
- Sprite fills 95px cell
- Visible content: 122 × 0.7421875 = **90.55px**
- Gap per side: 3 × 0.7421875 = **2.23px**

---

## 2. Movement/Attack Blobs

### 2.1 Merged Corner System

Each tile in a contiguous region is rendered as 4 quarter-tile corner pieces.

**Source:** `app/view/layers/game/TileLayer.js:213-269`

**Corner Pattern Encoding:**
```javascript
// '1' = edge neighbor on first side
// '2' = diagonal neighbor
// '3' = edge neighbor on second side
values.tl = (nl ? '1' : '') + (ntl ? '2' : '') + (nt ? '3' : '');
```

### 2.2 Corner Sprite Dimensions (Ground Truth)

**Source:** `tile_merged_large_*.png` analyzed via ImageMagick

| Sprite | File Size | Visible Content | Bounding Box | Margin |
|--------|-----------|-----------------|--------------|--------|
| tile_merged_large_0.png | 64×64 | 61×61 | `61x61+3+3` | **3px** (outer edges) |
| tile_merged_large_0123.png | 64×64 | 62×62 | `62x62+1+1` | **1px** (inner connection) |

### 2.3 Corner Margin Ratios

**Solo corner (outer edges):**
```
Margin ratio = 3 / 64 = 0.046875 (4.69%)
```

**Filled corner (inner edges connect):**
```
Margin ratio = 1 / 64 = 0.015625 (1.56%)
```

### 2.4 Critical Alignment Property

The floor tile and corner sprites share the same margin ratio on their outer edges:

| Component | Margin | Ratio | Result |
|-----------|--------|-------|--------|
| Floor tile | 3px in 128px | 2.34% of full tile | Gap between cells |
| Corner sprite (outer) | 3px in 64px | 4.69% of half-tile = **2.34% of full tile** | Aligns with floor edge |

**This ensures highlight sprites align exactly with floor tile edges, never extending into the gap.**

### 2.5 Colors

**Source:** `app/common/config.js:824-843`

| Highlight Type | Color | Hex |
|----------------|-------|-----|
| Movement (MOVE_COLOR) | 240, 240, 240 | #F0F0F0 |
| Movement Alt (hover) | 255, 255, 255 | #FFFFFF |
| Attack (AGGRO_COLOR) | **255, 217, 0** | **#FFD900** (Yellow) |
| Attack Alt (hover) | 255, 255, 255 | #FFFFFF |
| Attack Opponent | 210, 40, 70 | #D22846 |
| Path | 255, 255, 255 | #FFFFFF |
| Selection | 255, 255, 255 | #FFFFFF |
| Hover | 255, 255, 255 | #FFFFFF |

### 2.6 Opacity Values

**Source:** `app/common/config.js:528-531`

| State | Value | Normalized |
|-------|-------|------------|
| Select/Hover (TILE_SELECT_OPACITY) | 200 | 78% |
| Dim (during hover, TILE_DIM_OPACITY) | 127 | 50% |
| Faint (passive hover, TILE_FAINT_OPACITY) | 75 | 29% |

### 2.7 Seam Tiles

When movement and attack blobs are adjacent, a special "seam" sprite creates a visible boundary.

**Source:** `TileLayer.js:252-265`

| Sprite | File Size | Visible | Usage |
|--------|-----------|---------|-------|
| tile_merged_large_0_seam.png | 64×64 | 61×61 at +3+3 | Boundary between regions |

---

## 3. Render Order (Z-Order)

**Source:** `app/view/Player.js:48-56`

```
boardTileZOrder: 1       Floor/ownership tiles
moveTileZOrder: 2        Movement range blob
assistTileZOrder: 3      Assist tiles (unused)
aggroTileZOrder: 4       Attack range blob
attackableTargetTileZOrder: 5   Target indicators
cardTileZOrder: 6        Card play locations
selectTileZOrder: 7      Selection box
pathTileZOrder: 8        Movement path
mouseOverTileZOrder: 9   Hover highlight
```

---

## 4. Movement Path Rendering

### 4.1 Path Sprites

Daisy-chained tile sprites forming an arrow from unit to destination.

| Class | Sprite | Usage |
|-------|--------|-------|
| TileMapPathMoveStartSprite | `tile_path_move_start` | First segment |
| TileMapPathMoveStraightSprite | `tile_path_move_straight` | Middle straight |
| TileMapPathMoveStraightFromStartSprite | `tile_path_move_straight_from_start` | 2nd tile (straight) |
| TileMapPathMoveCornerSprite | `tile_path_move_corner` | 90° turn |
| TileMapPathMoveCornerFlippedSprite | (same, flipped) | Opposite turn |
| TileMapPathMoveCornerFromStartSprite | `tile_path_move_corner_from_start` | Corner from start |
| TileMapPathMoveEndSprite | `tile_path_move_end` | Arrow endpoint |
| TileMapPathMoveEndFromStartSprite | `tile_path_move_end_from_start` | Arrow (path len=2) |

### 4.2 "FromStart" Variants

Distinct sprites in the atlas with visual styling that connects better to the start position.

### 4.3 Rotation Calculation

**Source:** `app/view/Player.js:2010-2046`

```javascript
pdx = screenPosition.x - prevScreenPosition.x;
pdy = screenPosition.y - prevScreenPosition.y;
pa = -Math.atan2(pdy, pdx);

ndx = nextScreenPosition.x - screenPosition.x;
ndy = nextScreenPosition.y - screenPosition.y;
na = -Math.atan2(ndy, ndx);

if (na !== pa) corner = true;
```

### 4.4 Corner Flip Detection

```javascript
const cross = pdx * ndy - pdy * ndx;
isFlipped = cross < 0;
```

---

## 5. Hover Behavior

### 5.1 Instant Transitions Within Board

**Source:** `Player.js:1604-1610`

```javascript
if (wasHoveringOnBoard) {
  hoverFadeDuration = 0.0;    // INSTANT within board
} else {
  hoverFadeDuration = CONFIG.FADE_FAST_DURATION;  // 0.2s
}
```

When moving mouse between tiles WITHIN the board, transitions are **instant** (no fade).

### 5.2 Hover Opacity

| Context | Opacity |
|---------|---------|
| Passive hover (no selection) | 29% (TILE_FAINT_OPACITY) |
| Active hover (unit selected) | 78% (TILE_HOVER_OPACITY) |

---

## 6. Coordinate System

### 6.1 Constants

| Constant | Value | Source |
|----------|-------|--------|
| BOARDCOL | 9 | config.js:73 |
| BOARDROW | 5 | config.js:72 |
| TILESIZE | 95px | config.js:287 |
| XYZ_ROTATION.x | 16° | config.js:575 |
| ENTITY_XYZ_ROTATION.x | 26° | config.js:577 |
| DEPTH_OFFSET | 19.5 | config.js:573 |

### 6.2 Transform System

```javascript
UtilsEngine.transformBoardToScreen = function (boardPoint) {
  return cc.p(
    boardPoint.x * CONFIG.TILESIZE + UtilsEngine._screenBoardOrigin.x,
    boardPoint.y * CONFIG.TILESIZE + UtilsEngine._screenBoardOrigin.y
  );
};
```

---

## 7. TileLayer Architecture

### 7.1 Node Hierarchy

```
TileLayer
├── _boardNonBatchNode (cc.Node)
│   ├── XYZRotation: {x: 16°, y: 0°, z: 0°}
│   └── Contains: non-batched tile sprites
└── _boardBatchNode (cc.SpriteBatchNode)
    ├── Texture: RSX.tile_board.img
    └── Contains: batched merged tiles
```

### 7.2 High Quality Mode

When `CONFIG.boardQuality === BOARD_QUALITY_HIGH`:
- Tiles render to offscreen RenderPass
- Result is 3D-rotated and blitted to screen
- Enables anti-aliased edges

---

## 8. Timing Constants

**Source:** `app/common/config.js:550-567`

| Constant | Value | Usage |
|----------|-------|-------|
| FADE_FAST_DURATION | 0.2s | Quick transitions |
| FADE_MEDIUM_DURATION | 0.35s | Standard transitions |
| FADE_SLOW_DURATION | 1.0s | Slow transitions |
| PATH_MOVE_DURATION | 1.5s | Attack path animation |
| PATH_ARC_DISTANCE | 47.5px | Arc height for attacks |
| PATH_FADE_DISTANCE | 40.0px | Fade in/out distance |

---

## 9. Sprite Atlas

**Source:** `app/original_resources/tiles/tiles_board.plist`

| Property | Value |
|----------|-------|
| Atlas Size | 607 × 254 pixels |
| Format | TexturePacker v2 |

**Key Frames:**

| Frame | Position | Size | Source Size |
|-------|----------|------|-------------|
| tile_board.png | 2,2 | 122×122 | 128×128 |
| tile_hover.png | 126,2 | 122×122 | 128×128 |
| tile_merged_large_0.png | 532,128 | 61×61 | 64×64 |
| tile_merged_large_0123.png | 400,93 | 64×64 | 64×64 |

---

## 10. Key Design Principles

### 10.1 Negative Grid

Grid pattern emerges from tile gaps, not drawn lines.

### 10.2 Aligned Margins

Floor tiles and corner sprites have **matching margin ratios** (2.34% of full tile on outer edges), ensuring highlights align exactly with floor edges.

### 10.3 Yellow Attack Range

Bright yellow (#FFD900) for friendly attacks, red for opponent — distinctive visual identity.

### 10.4 Instant In-Board Transitions

No animation when moving between tiles on board — responsive feel.

### 10.5 Blob Dimming

50% opacity during active hover to emphasize target tile.

---

## 11. Ground Truth Summary

### 11.1 Floor Tile

```
File:           tile_board.png (128×128)
Visible:        122×122 at +3+3
Margin:         3px per side
Gap ratio:      3/128 = 0.0234375 (2.34%)
Color:          Black tint
Opacity:        20/255 = 7.84%
```

### 11.2 Corner Sprites

```
File:           tile_merged_large_*.png (64×64)

Solo corner (_0):
  Visible:      61×61 at +3+3
  Outer margin: 3px (4.69% of quarter = 2.34% of full tile)

Filled corner (_0123):
  Visible:      62×62 at +1+1
  Inner margin: 1px (connects to neighbors)
```

### 11.3 Alignment Verification

```
Floor gap (per side):     3/128 of tile = 2.34%
Corner outer margin:      3/64 of half  = 4.69% of half = 2.34% of tile
                          ✓ ALIGNED
```

---

## 12. Audio Feedback

| Event | Sound | Priority |
|-------|-------|----------|
| Unit Selection | sfx_unit_select | CONFIG.SELECT_SFX_PRIORITY |
| Hover | sfx_ui_in_game_hover | Normal |
