# Flow: Tile Interaction System

Complete documentation of Duelyst's tile rendering system including all sprite types, triggers, state machines, and interaction flows.

**Primary Sources:**
- `app/view/Player.js` — Main tile orchestration (2311 lines)
- `app/view/layers/game/TileLayer.js` — Tile rendering layer (577 lines)
- `app/view/nodes/map/*.js` — Individual tile sprite classes

---

## Audit Log

| Date | Scope | Status |
|------|-------|--------|
| 2025-12-23 | Click sequence verification | ✅ Verified (GameLayer.js:6274-6639) |
| 2025-12-23 | Right-click behavior | ✅ Verified (GameLayer.js:6308-6312) |
| 2025-12-23 | FromStart sprite variants | ✅ Verified distinct atlas positions |
| 2025-12-23 | Seam sprite usage | ✅ Verified (TileLayer.js:252-265) |
| 2025-12-23 | Flipped corner logic | ✅ Verified cross-product formula |
| 2025-12-24 | Enemy hover preview | ✅ Verified (Player.js:987-988, 1393, 1457) |

**Note:** This document focuses on **highlight tile rendering** (movement/attack blobs, paths).
For SDK.Tile game entities (Mana Springs), see `grid_rendering.md` §26.

---

## 1. Tile Sprite Class Hierarchy

### 1.1 Base Classes

| Class | File | Description |
|-------|------|-------------|
| `TileMapScaledSprite` | `TileMapScaledSprite.js` | Base for all tile-sized sprites, auto-scales to `CONFIG.TILESIZE` |
| `BaseSprite` | `../BaseSprite.js` | Cocos2d sprite base with pooling support |

### 1.2 Single-Tile Sprites

| Class | Texture Resource | Purpose | Trigger |
|-------|-----------------|---------|---------|
| `TileMapHoverSprite` | `RSX.tile_hover.frame` | Mouse hover indicator | Passive mouse movement |
| `TileMapLargeSprite` | `RSX.tile_large.frame` | Attackable target highlight | Unit selected, enemy in range |
| `TileMapGridSprite` | `RSX.tile_grid.frame` | Board grid cell (ownership) | Board initialization |
| `TileGlowSprite` | `RSX.tile_glow.frame` | Move target glow | Hover on valid move tile |
| `TileBoxSprite` | `RSX.tile_box.frame` | Selected unit box outline | Unit selection |
| `TileSpawnSprite` | `RSX.tile_spawn.frame` | Unit spawn location | Playing unit card |
| `TileSpellSprite` | `RSX.tile_spell.frame` | Spell target indicator | Playing spell card |
| `TileCardSprite` | `RSX.tile_card.frame` | Card play location | Card hover |
| `TileAttackSprite` | `RSX.tile_attack.frame` | Attack target reticle | Attack targeting |
| `TileOpponentSprite` | `RSX.tile_opponent.frame` | Opponent hover | Opponent's turn hover |

### 1.3 Merged Tile Corner Sprites (Movement/Attack Blob)

Each tile in a contiguous region is rendered as 4 corner pieces. Corner sprites selected based on neighbor presence.

**Large Variant (Movement/Attack Range):**

| Class | Pattern | Description |
|-------|---------|-------------|
| `TileMapMergedLarge0Sprite` | No neighbors | Isolated rounded corner |
| `TileMapMergedLarge01Sprite` | Edge neighbor (left/top) | One edge extends |
| `TileMapMergedLarge03Sprite` | Edge neighbor (right/bottom) | Other edge extends |
| `TileMapMergedLarge013Sprite` | Both edges, no diagonal | L-shaped connection |
| `TileMapMergedLarge0123Sprite` | All neighbors | Fully filled corner |
| `TileMapMergedLarge0SeamSprite` | Adjacent different area | Seam between regions |

**Hover Variant (Active Selection):**

| Class | Pattern |
|-------|---------|
| `TileMapMergedHover0Sprite` | No neighbors |
| `TileMapMergedHover01Sprite` | Edge neighbor (left/top) |
| `TileMapMergedHover03Sprite` | Edge neighbor (right/bottom) |
| `TileMapMergedHover013Sprite` | Both edges, no diagonal |
| `TileMapMergedHover0123Sprite` | All neighbors |
| `TileMapMergedHover0SeamSprite` | Seam |

### 1.4 Path Sprites (Movement Arrow)

Daisy-chained to form movement path from unit to target.

| Class | Texture | Usage |
|-------|---------|-------|
| `TileMapPathMoveStartSprite` | `tile_path_move_start` | First segment at unit position |
| `TileMapPathMoveStraightSprite` | `tile_path_move_straight` | Middle straight segment |
| `TileMapPathMoveStraightFromStartSprite` | `tile_path_move_straight_from_start` | Second segment (straight) |
| `TileMapPathMoveCornerSprite` | `tile_path_move_corner` | 90° turn |
| `TileMapPathMoveCornerFlippedSprite` | `tile_path_move_corner` (flipped) | 90° turn (alternate) |
| `TileMapPathMoveCornerFromStartSprite` | `tile_path_move_corner_from_start` | Corner from start |
| `TileMapPathMoveCornerFromStartFlippedSprite` | `tile_path_move_corner_from_start` (flipped) | Corner from start (alternate) |
| `TileMapPathMoveEndSprite` | `tile_path_move_end` | Arrow endpoint |
| `TileMapPathMoveEndFromStartSprite` | `tile_path_move_end_from_start` | Arrow when path length = 2 |

### 1.5 Attack Path Sprite

| Class | File | Description |
|-------|------|-------------|
| `AttackPathSprite` | `AttackPathSprite.js` | Animated arc projectile path for ranged attacks |

**AttackPathSprite Properties:**
```javascript
needsDepthDraw: true,
depthModifier: 0.0,
fadeDistance: CONFIG.PATH_FADE_DISTANCE,    // 40.0
arcDistance: CONFIG.PATH_ARC_DISTANCE,      // 47.5
speed: CONFIG.TILESIZE * (1.0 / CONFIG.PATH_MOVE_DURATION),  // 63.33 px/s
```

---

## 2. Tile Z-Order Hierarchy

**Source:** `app/view/Player.js:48-56`

```javascript
boardTileZOrder: 1,
moveTileZOrder: 2,
assistTileZOrder: 3,
aggroTileZOrder: 4,
attackableTargetTileZOrder: 5,
cardTileZOrder: 6,
selectTileZOrder: 7,
pathTileZOrder: 8,
mouseOverTileZOrder: 9,
```

**Rendering Order (back to front):**
1. Board grid tiles
2. Movement range blob
3. Assist tiles (unused?)
4. Attack range blob
5. Attackable target indicators
6. Card play locations
7. Selection box
8. Movement path arrow
9. Mouse hover tile

---

## 3. Color Constants

**Source:** `app/common/config.js:824-843`

| Constant | RGB | Hex | Usage |
|----------|-----|-----|-------|
| `CONFIG.PATH_COLOR` | 255,255,255 | #FFFFFF | Movement path |
| `CONFIG.MOVE_COLOR` | 240,240,240 | #F0F0F0 | Movement range blob |
| `CONFIG.MOVE_ALT_COLOR` | 255,255,255 | #FFFFFF | Movement hover |
| `CONFIG.AGGRO_COLOR` | 255,217,0 | #FFD900 | Attack range (friendly) |
| `CONFIG.AGGRO_ALT_COLOR` | 255,255,255 | #FFFFFF | Attack hover |
| `CONFIG.AGGRO_OPPONENT_COLOR` | 210,40,70 | #D22846 | Attack range (enemy) |
| `CONFIG.SELECT_COLOR` | 255,255,255 | #FFFFFF | Selection box |
| `CONFIG.SELECT_OPPONENT_COLOR` | 210,40,70 | #D22846 | Enemy selection |
| `CONFIG.MOUSE_OVER_COLOR` | 255,255,255 | #FFFFFF | Passive hover |
| `CONFIG.CARD_PLAYER_COLOR` | 255,217,0 | #FFD900 | Card play (friendly) |
| `CONFIG.CARD_OPPONENT_COLOR` | 210,40,70 | #D22846 | Card play (enemy) |

---

## 4. Opacity Constants

**Source:** `app/common/config.js:528-531`

| Constant | Value | Normalized | Usage |
|----------|-------|------------|-------|
| `CONFIG.TILE_SELECT_OPACITY` | 200 | 0.784 | Selected tile |
| `CONFIG.TILE_HOVER_OPACITY` | 200 | 0.784 | Hover tile |
| `CONFIG.TILE_DIM_OPACITY` | 127 | 0.498 | Dimmed blob (during hover) |
| `CONFIG.TILE_FAINT_OPACITY` | 75 | 0.294 | Passive hover |
| `CONFIG.PATH_TILE_ACTIVE_OPACITY` | 150 | 0.588 | Active movement path |
| `CONFIG.PATH_TILE_DIM_OPACITY` | 100 | 0.392 | Inactive path |
| `CONFIG.TARGET_ACTIVE_OPACITY` | (computed) | — | Active target reticle |
| `CONFIG.TARGET_DIM_OPACITY` | (computed) | — | Passive target |

---

## 5. Timing Constants

**Source:** `app/common/config.js:550-567`

| Constant | Value | Usage |
|----------|-------|-------|
| `CONFIG.FADE_FAST_DURATION` | 0.2s | Quick transitions (hover, path) |
| `CONFIG.FADE_MEDIUM_DURATION` | 0.35s | Standard transitions |
| `CONFIG.FADE_SLOW_DURATION` | 1.0s | Slow transitions |
| `CONFIG.PATH_MOVE_DURATION` | 1.5s | Attack path animation cycle |
| `CONFIG.PULSE_MEDIUM_DURATION` | (varies) | Pulsing scale animation |

---

## 6. State Machine: Player Tile State

**Source:** `app/view/Player.js:144-154`

```javascript
selecting: false,      // Unit is selected
showingCard: false,    // Card is being played
previewing: false,     // Preview mode
hovering: false,       // Something is being hovered
hoveringOnBoard: false, // Hover is on board (not UI)
hoverChanged: false,   // Hover position changed this frame

_previewDirty: false,
_selectedDirty: false,
_cardDirty: false,
_hoverDirty: false,
```

---

## 7. Interaction Flow: Unit Selection

### 7.1 Trigger
User clicks on own unit.

### 7.2 Call Chain
```
setSelectedEntityNode(entityNode)           [Player.js:588]
  ├─ removeAllSelectedTiles()               [Player.js:597] - Clear previous
  ├─ selectedEntityNode.setSelected(true)   [Player.js:614]
  ├─ play_effect(sfx_unit_select)           [Player.js:616] - Audio feedback
  └─ showEntityTiles()                      [Player.js:618] - Show range blobs
```

### 7.3 showEntityTiles() Flow
**Source:** `app/view/Player.js:1301-1463`

```
showEntityTiles(opacity, fadeDuration, preview)
  │
  ├─ Get movement range positions
  │    └─ sdkEntity.getMovementRange().getValidPositions()
  │
  ├─ Get attack pattern
  │    └─ sdkEntity.getAttackRange().getValidPositions()
  │
  ├─ showBoxTileForSelect()                  [Line 1369]
  │    └─ Creates TileBoxSprite at unit position
  │
  ├─ showMergedTilesForAction(moveLocations, ..., RSX.tile_merged_large.frame, ...)  [Line 1437]
  │    └─ Creates movement blob (4 corners per tile)
  │
  └─ showMergedTilesForAction(attackLocations, ..., RSX.tile_merged_large.frame, ...) [Line 1457]
       └─ Creates attack blob (different color)
```

### 7.4 Merged Tile Algorithm
**Source:** `app/view/layers/game/TileLayer.js:213-269`

For each tile position:
1. Check 8 neighbors (4 edges + 4 diagonals)
2. For each of 4 corners, encode neighbor pattern:
   - `'1'` = edge neighbor exists (left for TL corner)
   - `'2'` = diagonal neighbor exists
   - `'3'` = other edge neighbor exists (top for TL corner)
3. Select sprite variant from pattern:
   - `''` → `_0` (isolated)
   - `'1'` → `_01` (edge extends)
   - `'3'` → `_03` (other edge)
   - `'13'` → `_013` (both edges)
   - `'123'` → `_0123` (fully connected)
   - `'_seam'` → `_0_seam` (adjacent different region)

### 7.5 Corner Rotation
| Corner | Rotation |
|--------|----------|
| Top-Left (tl) | 0° |
| Top-Right (tr) | 90° |
| Bottom-Right (br) | 180° |
| Bottom-Left (bl) | 270° |

---

## 8. Interaction Flow: Hover on Valid Move Tile

### 8.1 Trigger
Mouse moves over tile within movement range while unit is selected.

### 8.2 Call Chain
```
showHover(opacity, fadeDuration)            [Player.js:1557]
  │
  ├─ Check: selectedSdkEntity exists
  ├─ Check: movementRange.getIsPositionValid(mouseBoardPosition)
  │
  ├─ wasHoveringOnBoard?
  │    ├─ YES: hoverFadeDuration = 0.0    [Line 1605] ← INSTANT transition
  │    └─ NO:  hoverFadeDuration = FADE_FAST_DURATION
  │
  ├─ showGlowTileForAction()                [Line 1724]
  │    └─ Creates TileGlowSprite at hover position
  │
  ├─ path = getMovementRange().getPathTo()  [Line 1736]
  │    └─ BFS pathfinding from unit to hover position
  │
  └─ showPath(path, active=true, direct=false)  [Line 1815]
       └─ showTilePath()                    [Line 1963]
            ├─ removePath() - Always clears previous
            └─ Create daisy-chain of path sprites
```

### 8.3 Key Behavior: Instant Path Update
**VERIFIED:** When hovering between tiles WITHIN the board:
- `hoverFadeDuration = 0.0` (line 1605)
- Previous path is removed and new path created instantly
- No visible fade transition when moving mouse within blob

### 8.4 Path Sprite Selection Logic
**Source:** `app/view/Player.js:2003-2069`

```javascript
// For each position in path:
if (!prevScreenPosition) {
  // First tile
  tileMapPathMoveClass = TileMapPathMoveStartSprite;
} else if (!nextScreenPosition) {
  // Last tile
  if (fromStart) {
    tileMapPathMoveClass = TileMapPathMoveEndFromStartSprite;
  } else {
    tileMapPathMoveClass = TileMapPathMoveEndSprite;
  }
} else if (corner) {
  // Turn in path
  if (fromStart) {
    tileMapPathMoveClass = isFlipped ? TileMapPathMoveCornerFromStartFlippedSprite
                                     : TileMapPathMoveCornerFromStartSprite;
  } else {
    tileMapPathMoveClass = isFlipped ? TileMapPathMoveCornerFlippedSprite
                                     : TileMapPathMoveCornerSprite;
  }
} else if (fromStart) {
  // Second tile, straight
  tileMapPathMoveClass = TileMapPathMoveStraightFromStartSprite;
} else {
  // Middle tile, straight
  tileMapPathMoveClass = TileMapPathMoveStraightSprite;
}
```

### 8.5 Path Rotation Calculation
**Source:** `app/view/Player.js:2010-2046`

```javascript
// Calculate angle from previous to current position
if (prevScreenPosition) {
  pdx = screenPosition.x - prevScreenPosition.x;
  pdy = screenPosition.y - prevScreenPosition.y;
  radians = pa = -Math.atan2(pdy, pdx);
}
// Calculate angle from current to next position
if (nextScreenPosition) {
  ndx = nextScreenPosition.x - screenPosition.x;
  ndy = nextScreenPosition.y - screenPosition.y;
  radians = na = -Math.atan2(ndy, ndx);
}
// Detect corner by angle change
if (na !== pa) {
  corner = true;
}
```

---

## 9. Interaction Flow: Hover Outside Movement Range

### 9.1 Trigger
Mouse moves outside valid movement range while unit is selected.

### 9.2 Call Chain
```
showHover(opacity, fadeDuration)
  │
  ├─ path = null (no valid move)            [Line 1817-1818]
  │
  └─ removePath(fadeDuration)               [Line 1819]
       └─ Fades out path over FADE_FAST_DURATION (0.2s)
```

### 9.3 removeHover() Side Effects
**Source:** `app/view/Player.js:1856-1899`

```javascript
removeHover(fadeDuration, keepPersistent, keepSpellTiles)
  ├─ removeTilesWithFade(mouseOverTiles)    - Hover tiles
  ├─ removeTilesWithFade(mouseOverSpellTiles) - Spell area tiles
  ├─ _fadeTiles(selectedTiles)              - Restore blob opacity
  ├─ removeBoxTileForAction()
  ├─ removeGlowTileForAction()
  ├─ removeSpawnTile()
  ├─ removeSpellTile()
  ├─ removeCardTile()
  ├─ removeOpponentTile()
  └─ if (!keepPersistent):
       ├─ removeTargetTile()
       └─ removePath()
```

---

## 10. Interaction Flow: Attack Targeting

### 10.1 Direct Path (Ranged Attack)
When hovering over attackable enemy, uses direct path with animated `AttackPathSprite`:

```
showHover()
  │
  ├─ Check: withinAttackRange && mouseOverSdkEntity
  │
  ├─ directPath = true                      [Line 1714]
  │
  └─ showPath(path, active=true, direct=true)
       └─ showDirectPath()                  [Line 1961]
            └─ Creates AttackPathSprite array
            └─ Animates arc motion with fade in/out
```

### 10.2 AttackPathSprite Animation
**Source:** `app/view/nodes/map/AttackPathSprite.js:78-115`

```javascript
update(dt) {
  // Speed increases for short distances
  const speedModifier = Math.min(1.5, Math.max(1.0, (CONFIG.TILESIZE * 2.0) / this._distance));
  this._currentDistance += this.speed * dt * speedModifier;

  // Arc motion (sine wave)
  const arcPct = Math.min(this._currentDistance / this._distance, 1.0);
  const arcModifier = Math.sin(arcPct * Math.PI);
  nextPosition.y += this.arcDistance * arcModifier;

  // Depth adjustment for arc
  this.setDepthOffset(-this.arcDistance * (arcModifier - 0.5) * 2.0);

  // Opacity fade in/out at endpoints
  const fadeIn = Math.min(this._currentDistance / this.fadeDistance, 1.0);
  const fadeOut = 1.0 - Math.min(...);
  this._pathOpacityModifier = fadeIn * fadeOut;

  // Loop when complete
  if (movePct >= 1.0) {
    this._restartPath();
  }
}
```

---

## 11. Blob Dimming During Hover

### 11.1 Behavior
When hovering a tile within the movement blob, the entire blob dims to 50% opacity.

### 11.2 Implementation
**Source:** `app/view/Player.js:1842-1848`

```javascript
_showActiveHoverTile(locs, tiles, framePrefix, opacity, fadeDuration, color, fadeOpacity, fadeTiles) {
  if (fadeOpacity == null) { fadeOpacity = CONFIG.TILE_DIM_OPACITY; }  // 127/255 = 50%
  if (fadeTiles == null) { fadeTiles = this.selectedTiles; }
  this._fadeTiles(fadeTiles, fadeDuration, fadeOpacity);  // Dim the blob
  this.showMergedTilesForAction(locs, ..., tiles);        // Show hover tiles at full
}
```

### 11.3 Restore on Hover Exit
**Source:** `app/view/Player.js:1871-1874`

```javascript
if (CONFIG.SHOW_MERGED_MOVE_ATTACK_TILES) {
  this.getTileLayer().updateMergedTileTextures(RSX.tile_merged_large.frame, ...);
}
this._fadeTiles(this.selectedTiles, fadeDuration);  // Restore full opacity
```

---

## 12. Special Tile Functions

### 12.1 Function Summary

| Function | Sprite Class | Behavior |
|----------|-------------|----------|
| `showBoxTileForSelect()` | `TileBoxSprite` | Pulsing scale at unit |
| `showBoxTileForAction()` | `TileBoxSprite` | Action indicator |
| `showTargetTile()` | `BaseSprite` or `TileAttackSprite` | Target reticle |
| `showSpawnTile()` | `TileSpawnSprite` | Unit placement (pulsing) |
| `showSpellTile()` | `TileSpellSprite` | Spell target (pulsing) |
| `showCardTile()` | `TileCardSprite` | Card location (pulsing) |
| `showOpponentTile()` | `TileOpponentSprite` | Opponent hover |
| `showGlowTileForAction()` | `TileGlowSprite` | Move target glow |

### 12.2 Pulsing Scale Animation
**Source:** `app/view/Player.js:2295-2301`

```javascript
_showPulsingScaleTile(tile, position, opacity, fadeDuration, color, scale) {
  if (scale == null) { scale = 0.85; }
  tile.setScale(1.0);
  tile.startPulsingScale(CONFIG.PULSE_MEDIUM_DURATION, scale);
  this._showSpecialTile(tile, position, this.selectTileZOrder, opacity, fadeDuration, color);
}
```

---

## 13. TileLayer Rendering Pipeline

### 13.1 Layer Hierarchy
```
TileLayer
├── _boardNonBatchNode (cc.Node)
│   ├── XYZRotation: CONFIG.XYZ_ROTATION (16°, 0°, 0°)
│   └── Contains: non-batched tile sprites
└── _boardBatchNode (cc.SpriteBatchNode)
    ├── Texture: RSX.tile_board.img
    └── Contains: batched merged tiles
```

### 13.2 High Quality Mode
When `CONFIG.boardQuality === CONFIG.BOARD_QUALITY_HIGH`:
- Board batch node renders to offscreen `RenderPass`
- Result is 3D-rotated and blitted to screen
- Enables anti-aliasing for rotated tiles

### 13.3 Low Quality Mode
- Board batch node added directly to layer
- Both batch and non-batch nodes have `XYZRotation` applied

---

## 14. Verified Behaviors

### 14.1 Click Sequence for Movement — VERIFIED
**Source:** `GameLayer.js:6274-6639`

**Confirmed 2-click sequence:**
1. **Click 1:** Select unit → Blob appears, path shows on hover
2. **Hover:** Path updates instantly as mouse moves within blob
3. **Click 2:** Execute move via `_actionSelectedCardOnBoard()` → `selectedSdkEntity.actionMove()`

**Right-click:** Calls `NavigationManager.getInstance().requestUserTriggeredCancel()` to deselect.

### 14.2 Path Lock System — DOCUMENTED
**Source:** `Player.js` path lock methods

- `_showPathsLocked`: Boolean flag
- `_showPathsLockId`: Current lock holder ID
- `_showPathsLockRequests[]`: Queue of lock requests

**Purpose:** Prevents path display during:
- Tutorial sequences
- Action animations in progress
- Card inspection overlays

### 14.3 Seam Sprite Usage — VERIFIED
**Source:** `TileLayer.js:252-265`, `tiles_board.plist`

**When seam is used:**
- Corner has NO same-region neighbor
- BUT HAS adjacent tile from different region (movement vs attack)

**Visual effect:** Boundary line between movement and attack blobs.

**Sprites (same 61×61 size, different visual content):**
- `tile_merged_large_0.png` — Normal isolated corner
- `tile_merged_large_0_seam.png` — Corner with visible edge line

### 14.4 "FromStart" Path Variants — VERIFIED
**Source:** `tiles_board.plist` frame definitions

| Sprite | Atlas Position | Size | Purpose |
|--------|---------------|------|---------|
| `tile_path_move_straight.png` | {{406,2}} | 128×12 | Middle segment |
| `tile_path_move_straight_from_start.png` | {{406,16}} | 128×12 | Second tile (connects to start) |
| `tile_path_move_end.png` | {{250,2}} | 82×60 | Arrow (middle/end of path) |
| `tile_path_move_end_from_start.png` | {{250,64}} | 82×60 | Arrow (path length = 2) |

**Usage:** "FromStart" variants connect visually with start position; regular variants connect with preceding segments.

### 14.5 Flipped Corner Logic — VERIFIED
**Source:** `Player.js:2039-2045`

```javascript
// Determine flip based on cross product of direction vectors
const cross = pdx * ndy - pdy * ndx;
isFlipped = cross < 0;
```

**Visual effect:** Corner sprites are mirrored horizontally when turning in the opposite rotational direction.

**Example:**
- Right→Up turn: Normal corner sprite
- Right→Down turn: Flipped corner sprite (mirrored)

### 14.6 Enemy Hover Preview (No Selection) — VERIFIED

**Source:** `Player.js:987-988, 1310-1313, 1373, 1385, 1393, 1457`

When hovering an enemy unit with no friendly unit selected, Duelyst shows the enemy's attack range as a red blob.

**Trigger:**
```javascript
// Line 987-988: Called for ANY entity hover during your turn
if (isForMyPlayer && !this.getIsTakingSelectionAction()) {
  this.previewEntityTiles();  // NOT gated by entity ownership
}
```

**Key Variables:**
| Variable | Meaning | Value for Enemy Hover |
|----------|---------|----------------------|
| `isForMyPlayer` | Local player's view | `true` (your turn) |
| `isMyEntity` | Entity owned by you | `false` (enemy) |

**Gate Logic:**
```javascript
// Line 1373: Passes for your turn
if (isForMyPlayer) {
  // Line 1385: Movement tiles — BLOCKED for enemy
  if (moveLocations && moveLocations.length > 1 && isMyEntity) {
    hasMoveTiles = true;  // FALSE: isMyEntity fails
  }

  // Line 1393: Attack pattern — ALLOWED for enemy
  if (!isMyEntity || CONFIG.SHOW_MERGED_MOVE_ATTACK_TILES) {
    attackPattern = sdkEntity.getAttackRange().getValidPositions(...);
    // TRUE: !isMyEntity passes
  }
}

// Line 1457: Rendered with opponent color
this.showMergedTilesForAction(...,
  sdkEntity.isOwnedByMyPlayer() ? CONFIG.AGGRO_COLOR : CONFIG.AGGRO_OPPONENT_COLOR,
  ...);
```

**Result:**
| Element | Shown | Color |
|---------|-------|-------|
| Movement blob | NO | — |
| Attack blob | YES | `AGGRO_OPPONENT_COLOR` (#D22846) |
| Ownership indicator | HIDDEN | (hover state hides it) |

**Visual Behavior:**
1. Red ownership indicator hides (entity enters hover state)
2. Red attack blob appears showing enemy's attack range
3. Passive hover tile shown at cursor position

---

## 15. File Reference

### 15.1 Primary Files

| File | Lines | Purpose |
|------|-------|---------|
| `app/view/Player.js` | 2311 | Main tile orchestration |
| `app/view/layers/game/TileLayer.js` | 577 | Tile rendering layer |
| `app/common/config.js` | ~900 | Constants |

### 15.2 Sprite Files

| File | Purpose |
|------|---------|
| `app/view/nodes/map/TileMapScaledSprite.js` | Base scaled sprite |
| `app/view/nodes/map/TileMapHoverSprite.js` | Hover indicator |
| `app/view/nodes/map/TileMapLargeSprite.js` | Attackable target |
| `app/view/nodes/map/TileMapMerged*.js` | Merged tile corners (12 files) |
| `app/view/nodes/map/TileMapPathMove*.js` | Path segments (9 files) |
| `app/view/nodes/map/AttackPathSprite.js` | Animated attack path |
| `app/view/nodes/map/Tile*Sprite.js` | Special tiles (8 files) |

### 15.3 Asset Files

| File | Contents |
|------|----------|
| `app/resources/tiles/tiles_board.plist` | Board tile atlas |
| `app/resources/tiles/tiles_action.plist` | Action tile atlas |
| `app/resources/tiles/tile_*.png` | Individual tile textures |

---

## 16. Cross-Reference to Grid Rendering Summary

This document expands on `duelyst_analysis/summaries/grid_rendering.md` with:
- Complete sprite class enumeration
- Interaction flow state machines
- Function-level call chains
- Verified behavior with source line references
