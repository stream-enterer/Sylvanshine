# Duelyst Grid Rendering System: Forensic Analysis

Complete technical documentation of Duelyst's grid implementation including rendering, highlighting, animation, and depth systems.

---

## 1. Configuration Constants

### Source: `app/common/config.js`

| Constant | Value | Source Line |
|----------|-------|-------------|
| `CONFIG.BOARDROW` | `5` | Line 72 |
| `CONFIG.BOARDCOL` | `9` | Line 73 |
| `CONFIG.BOARDCENTER` | `{ x: 4, y: 2 }` | Line 74 |
| `CONFIG.TILESIZE` | `95` | Line 287 |
| `CONFIG.TILEOFFSET_X` | `0.0` | Line 291 |
| `CONFIG.TILEOFFSET_Y` | `10.0` | Line 292 |
| `CONFIG.TILE_TARGET_PCT` | `0.45` | Line 289 |
| `CONFIG.FLOOR_TILE_COLOR` | `{ r: 0, g: 0, b: 0 }` | Line 293 |
| `CONFIG.FLOOR_TILE_OPACITY` | `20` | Line 294 |
| `CONFIG.REF_WINDOW_SIZE` | `{ width: 1280.0, height: 720.0 }` | Line 70 |
| `CONFIG.SCALE` | `2.0` | Line 571 |
| `CONFIG.DEPTH_OFFSET` | `19.5` | Line 573 |
| `CONFIG.XYZ_ROTATION` | `{ x: 16.0, y: 0.0, z: 0.0 }` | Line 575 |
| `CONFIG.ENTITY_XYZ_ROTATION` | `{ x: 26.0, y: 0.0, z: 0.0 }` | Line 577 |

### Derived Values

| Constant | Formula | Effective Value |
|----------|---------|-----------------|
| `CONFIG.INSTRUCTION_NODE_OFFSET` | `CONFIG.TILESIZE * 0.5` | `47.5` |
| `CONFIG.PATH_ARC_DISTANCE` | `CONFIG.TILESIZE * 0.5` | `47.5` |
| `CONFIG.OVERLAY_STATS_OFFSET` | `{ x: 0.0, y: -CONFIG.TILESIZE * 0.6 }` | `{ x: 0.0, y: -57.0 }` |
| `CONFIG.REACH_RANGED` | `CONFIG.BOARDCOL + CONFIG.BOARDROW` | `14` |
| `CONFIG.WHOLE_BOARD_RADIUS` | `max(BOARDCOL, BOARDROW)` | `9` |

---

## 2. Board SDK Class

### Source: `app/sdk/board.coffee`

```coffeescript
class Board extends SDKObject
  cardIndices: null
  columnCount: CONFIG.BOARDCOL  # 9
  rowCount: CONFIG.BOARDROW     # 5
```

### Position Validation

```coffeescript
isOnBoard: (position) ->
  return position? and position.x >= 0 and position.y >= 0 and position.x < @columnCount and position.y < @rowCount
```

### Spatial Query Methods

| Method | Description |
|--------|-------------|
| `getCardsWithinRadiusOfPosition(position, type, radius)` | Manhattan distance query |
| `getEntitiesInColumn(col)` | Filter by x position |
| `getEntitiesInRow(row)` | Filter by y position |
| `getEntitiesOnEntityStartingSide()` | Player 1: cols 0-3; Player 2: cols 5-8 |

---

## 3. Coordinate Transform System

### Source: `app/common/utils/utils_engine.js`

### Board Origin Calculation (lines 806-815)

```javascript
UtilsEngine._screenBoardSize = cc.size(
  CONFIG.BOARDCOL * CONFIG.TILESIZE,  // 9 * 95 = 855
  CONFIG.BOARDROW * CONFIG.TILESIZE   // 5 * 95 = 475
);
UtilsEngine._screenBoardOrigin = cc.p(
  Math.round((cc.winSize.width - UtilsEngine._screenBoardSize.width) * 0.5 + CONFIG.TILESIZE * 0.5 + CONFIG.TILEOFFSET_X),
  Math.round((cc.winSize.height - UtilsEngine._screenBoardSize.height) * 0.5 + CONFIG.TILESIZE * 0.5 + CONFIG.TILEOFFSET_Y)
);
```

### Transform Functions

**Board to Screen** (line 454-456):
```javascript
UtilsEngine.transformBoardToScreen = function (boardPoint) {
  return cc.p(
    boardPoint.x * CONFIG.TILESIZE + UtilsEngine._screenBoardOrigin.x,
    boardPoint.y * CONFIG.TILESIZE + UtilsEngine._screenBoardOrigin.y
  );
};
```

**Screen to Board** (line 462-464):
```javascript
UtilsEngine.transformScreenToBoard = function (screenPoint) {
  return cc.p(
    (screenPoint.x - UtilsEngine._screenBoardOrigin.x) / CONFIG.TILESIZE,
    (screenPoint.y - UtilsEngine._screenBoardOrigin.y) / CONFIG.TILESIZE
  );
};
```

**Screen to Board Index** (line 496-498):
```javascript
UtilsEngine.transformScreenToBoardIndex = function (screenPoint) {
  return cc.p(
    Math.floor((screenPoint.x - UtilsEngine._screenBoardOrigin.x + CONFIG.TILESIZE * 0.5) / CONFIG.TILESIZE),
    Math.floor((screenPoint.y - UtilsEngine._screenBoardOrigin.y + CONFIG.TILESIZE * 0.5) / CONFIG.TILESIZE)
  );
};
```

---

## 4. Perspective Projection System

### Matrix Generation (lines 787-803)

```javascript
// 60° FOV perspective projection
UtilsEngine.MAT4_PERSPECTIVE_PROJECTION = cc.kmMat4Identity(new cc.kmMat4());
cc.kmMat4PerspectiveProjection(UtilsEngine.MAT4_PERSPECTIVE_PROJECTION, 60, cc.winSize.width / cc.winSize.height, 0.1, zeye * 2);

// View matrix (LookAt)
const eye = cc.kmVec3Fill(null, cc.winSize.width / 2, cc.winSize.height / 2, zeye);
const center = cc.kmVec3Fill(null, cc.winSize.width / 2, cc.winSize.height / 2, 0.0);
const up = cc.kmVec3Fill(null, 0.0, 1.0, 0.0);
cc.kmMat4LookAt(UtilsEngine.MAT4_PERSPECTIVE_STACK, eye, center, up);
```

### Tile Map Rotation Matrix (lines 932-950)

```javascript
const xyzRotationMatrix = cc.kmMat4RotationPitchYawRoll(
  new cc.kmMat4(),
  cc.degreesToRadians(CONFIG.XYZ_ROTATION.x),  // 16.0°
  cc.degreesToRadians(CONFIG.XYZ_ROTATION.y),  // 0.0°
  cc.degreesToRadians(CONFIG.XYZ_ROTATION.z)   // 0.0°
);
```

### Point Projection (lines 547-570)

```javascript
UtilsEngine.projectPoint = function (point, projectionMatrix, zMatrix) {
  const { mat } = projectionMatrix;
  const winCenterPosition = UtilsEngine.getGSIWinCenterPosition();

  const x = point.x - winCenterPosition.x;
  const y = point.y - winCenterPosition.y;
  const d = 1.0 / (mat[3] * x + mat[7] * y + mat[15]);

  const xp = (mat[0] * x + mat[4] * y) * d;
  const yp = (mat[1] * x + mat[5] * y) * d;

  return cc.p(xp * winCenterPosition.x + winCenterPosition.x,
              yp * winCenterPosition.y + winCenterPosition.y);
};
```

---

## 5. Z-Order / Depth Sorting System

### Source: `app/view/extensions/NodeInjections.js`

### Auto Z-Order Properties (lines 182-186)

```javascript
cc.Node.prototype.autoZOrder = false;
cc.Node.prototype.autoZOrderPosition = null;
cc.Node.prototype.autoZOrderOffset = 0.0;
```

### Z-Order Index Calculation (line 203-204)

```javascript
cc.Node.prototype.getAutoZOrderIndex = function () {
  return Math.floor((this._position.y - UtilsEngine._boardCenterY + CONFIG.TILESIZE * 0.5) / CONFIG.TILESIZE);
};
```

### Z-Order Value Calculation (lines 216-218)

```javascript
cc.Node.prototype.getAutoZOrderValue = function () {
  return 100.0 - this.getAutoZOrderIndex() + this.autoZOrderOffset;
};
```

---

## 6. XYZ Rotation System

### Source: `app/view/extensions/NodeInjections.js:459-475`

```javascript
cc.Node.prototype.setXYZRotation = function (rotation) {
  if (!this.hasOwnProperty('_xyzRotation')) {
    this._xyzRotation = new cc.kmVec3();
  }
  const rotX = rotation.x != null ? rotation.x : this._xyzRotation.x;
  const rotY = rotation.y != null ? rotation.y : this._xyzRotation.y;
  const rotZ = rotation.z != null ? rotation.z : this._xyzRotation.z;

  if (this._xyzRotation.x !== rotX || this._xyzRotation.y !== rotY || this._xyzRotation.z !== rotZ) {
    this._xyzRotation.x = rotX;
    this._xyzRotation.y = rotY;
    this._xyzRotation.z = rotZ;
    this.setXYZRotationDirty();
  }
};
```

### Rotation Applications

| Node Type | Rotation | Source |
|-----------|----------|--------|
| `TileLayer._boardNonBatchNode` | 16°, 0°, 0° | `TileLayer.js:60` |
| `TileLayer._boardBatchNode` | 16°, 0°, 0° | `TileLayer.js:149` |
| Entity readiness particles | 26°, 0°, 0° | `EntityNode.js:172` |
| Shockwave FX | 26°, 0°, 0° | `FXShockwaveSprite.js:16` |

---

## 7. Tile Rendering Pipeline

### Source: `app/view/layers/game/TileLayer.js`

### Layer Hierarchy

```
TileLayer
├── _boardNonBatchNode (cc.Node)
│   ├── XYZRotation: CONFIG.XYZ_ROTATION
│   └── contains non-batched tile sprites
├── _boardBatchNode (cc.SpriteBatchNode)
│   ├── texture: RSX.tile_board.img
│   └── contains batched merged tiles
└── _renderPass (RenderPass, for high quality mode)
```

### Merged Tile Corner Values (lines 213-269)

```javascript
// Corner values encode neighbor presence
// '1' = edge neighbor exists
// '2' = corner neighbor exists
// '3' = edge neighbor on other side exists

values.tl = (nl ? '1' : '') + (ntl ? '2' : '') + (nt ? '3' : '');
values.tr = (nt ? '1' : '') + (ntr ? '2' : '') + (nr ? '3' : '');
values.br = (nr ? '1' : '') + (nbr ? '2' : '') + (nb ? '3' : '');
values.bl = (nb ? '1' : '') + (nbl ? '2' : '') + (nl ? '3' : '');
```

### Corner Piece Selection

| Pattern | Sprite Suffix | Description |
|---------|---------------|-------------|
| `''` (empty) | `0` | Solo corner |
| `'1'` | `01` | Edge neighbor only |
| `'3'` | `03` | Opposite edge only |
| `'13'` or `'123'` | `013` | Three-way junction |
| `'12'` or `'23'` | `0123` | Full connection |
| `'_seam'` | `0_seam` | Adjacent different area |

### Rotation Values

| Corner Position | Rotation |
|-----------------|----------|
| Top-Left (tl) | 0° |
| Top-Right (tr) | 90° |
| Bottom-Right (br) | 180° |
| Bottom-Left (bl) | 270° |

---

## 8. Tile Highlight Color Configuration

### Source: `app/common/config.js:824-843`

| Color Constant | RGB Value | Hex |
|----------------|-----------|-----|
| `CONFIG.PATH_COLOR` | `{ r: 255, g: 255, b: 255 }` | `#FFFFFF` |
| `CONFIG.MOVE_COLOR` | `{ r: 240, g: 240, b: 240 }` | `#F0F0F0` |
| `CONFIG.MOVE_ALT_COLOR` | `{ r: 255, g: 255, b: 255 }` | `#FFFFFF` |
| `CONFIG.AGGRO_COLOR` | `{ r: 255, g: 217, b: 0 }` | `#FFD900` |
| `CONFIG.AGGRO_ALT_COLOR` | `{ r: 255, g: 255, b: 255 }` | `#FFFFFF` |
| `CONFIG.AGGRO_OPPONENT_COLOR` | `{ r: 210, g: 40, b: 70 }` | `#D22846` |
| `CONFIG.SELECT_COLOR` | `{ r: 255, g: 255, b: 255 }` | `#FFFFFF` |
| `CONFIG.SELECT_OPPONENT_COLOR` | `{ r: 210, g: 40, b: 70 }` | `#D22846` |
| `CONFIG.MOUSE_OVER_COLOR` | `{ r: 255, g: 255, b: 255 }` | `#FFFFFF` |
| `CONFIG.CARD_PLAYER_COLOR` | `{ r: 255, g: 217, b: 0 }` | `#FFD900` |
| `CONFIG.CARD_OPPONENT_COLOR` | `{ r: 210, g: 40, b: 70 }` | `#D22846` |

### Opacity Constants (config.js:528-531)

| Constant | Value |
|----------|-------|
| `CONFIG.TILE_SELECT_OPACITY` | `200` |
| `CONFIG.TILE_HOVER_OPACITY` | `200` |
| `CONFIG.TILE_DIM_OPACITY` | `127` |
| `CONFIG.TILE_FAINT_OPACITY` | `75` |

---

## 9. Player Tile Z-Order Hierarchy

### Source: `app/view/Player.js:48-56`

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

---

## 10. Tile Sprite Classes

### Base Class: `app/view/nodes/map/TileMapScaledSprite.js`

```javascript
const TileMapScaledSprite = BaseSprite.extend({
  antiAlias: false,
  ctor(options) {
    this._super(options);
    this.setScale(CONFIG.TILESIZE / this.getContentSize().width);  // Scale to 95px
  },
  onExit() {
    this._super();
    cc.pool.putInPool(this);  // Object pooling
  },
});
```

### Tile Sprite Types

| Class | Texture Resource | Purpose |
|-------|------------------|---------|
| `TileMapGridSprite` | `RSX.tile_grid.frame` | Ownership indicator |
| `TileMapHoverSprite` | `RSX.tile_hover.frame` | Mouse hover |
| `TileMapLargeSprite` | `RSX.tile_large.frame` | Attackable target |
| `TileBoxSprite` | Box outline | Selected entity |
| `TileSpawnSprite` | Spawn effect | Unit spawn location |
| `TileSpellSprite` | Spell effect | Spell target |
| `TileGlowSprite` | Glow effect | Movement destination |

### Merged Tile Variants

| Class | Description |
|-------|-------------|
| `TileMapMergedLarge0Sprite` | Single corner |
| `TileMapMergedLarge01Sprite` | Two adjacent corners |
| `TileMapMergedLarge03Sprite` | Two opposite corners |
| `TileMapMergedLarge013Sprite` | Three corners |
| `TileMapMergedLarge0123Sprite` | All four corners |
| `TileMapMergedLarge0SeamSprite` | Edge seam |

---

## 11. Path Tile Sprites

### Source: `app/view/Player.js:2051-2069`

| Class | Usage |
|-------|-------|
| `TileMapPathMoveStartSprite` | First tile in path |
| `TileMapPathMoveEndSprite` | Last tile (not from start) |
| `TileMapPathMoveEndFromStartSprite` | Last tile when path length = 2 |
| `TileMapPathMoveStraightSprite` | Middle straight segment |
| `TileMapPathMoveStraightFromStartSprite` | Second tile (straight) |
| `TileMapPathMoveCornerSprite` | Corner piece (standard) |
| `TileMapPathMoveCornerFlippedSprite` | Corner piece (flipped) |
| `TileMapPathMoveCornerFromStartSprite` | Corner from start |
| `TileMapPathMoveCornerFromStartFlippedSprite` | Corner from start (flipped) |

### Path Rotation Logic (Player.js:2010-2046)

```javascript
// Calculate rotation from deltas
if (prevScreenPosition) {
  pdx = screenPosition.x - prevScreenPosition.x;
  pdy = screenPosition.y - prevScreenPosition.y;
  radians = pa = -Math.atan2(pdy, pdx);
}
if (nextScreenPosition) {
  ndx = nextScreenPosition.x - screenPosition.x;
  ndy = nextScreenPosition.y - screenPosition.y;
  radians = na = -Math.atan2(ndy, ndx);
}

// Detect corner based on angle change
if (na !== pa) {
  corner = true;
}
```

---

## 12. Hover State Machine

### Source: `app/view/Player.js:144-154`

```javascript
selecting: false,
showingCard: false,
previewing: false,
hovering: false,
hoveringOnBoard: false,
hoverChanged: false,

_previewDirty: false,
_selectedDirty: false,
_cardDirty: false,
_hoverDirty: false,
```

### Hover Flow (`showHover`, Player.js:1557-1841)

```
showHover(opacity, fadeDuration)
    │
    ├─ Check mouseIsOnBoard, isForMyPlayer, isAltPlayer
    │
    ├─ Determine hover type:
    │   ├─ forOpponentDeckAction → showCardTile + TileMapHoverSprite
    │   ├─ forMyCardAction (spell) → showSpellTile + _showActiveHoverTile
    │   ├─ forMyCardAction (unit) → showSpawnTile + TileMapHoverSprite
    │   ├─ selectedSdkEntity (attack) → TileMapHoverSprite (AGGRO_COLOR)
    │   ├─ selectedSdkEntity (move) → showGlowTileForAction
    │   └─ passive → TileMapHoverSprite (MOUSE_OVER_COLOR)
    │
    └─ Show path/target tile
```

---

## 13. Sprite Atlas Format

### Source: `app/original_resources/tiles/tiles_board.plist`

### Atlas Metadata

| Property | Value |
|----------|-------|
| Atlas Size | 607 × 254 pixels |
| Format | TexturePacker v2 |
| Texture File | `tiles_board.png` |

### Frame Definition Format

```xml
<key>tile_hover.png</key>
<dict>
    <key>frame</key>
    <string>{{126,2},{122,122}}</string>      <!-- {{x,y},{width,height}} -->
    <key>offset</key>
    <string>{0,0}</string>                     <!-- center offset -->
    <key>rotated</key>
    <false/>                                   <!-- 90° CW rotation -->
    <key>sourceColorRect</key>
    <string>{{3,3},{122,122}}</string>        <!-- trimmed region -->
    <key>sourceSize</key>
    <string>{128,128}</string>                 <!-- original size -->
</dict>
```

### UV Coordinate Calculation

```
u0 = frame.x / atlas.width
v0 = frame.y / atlas.height
u1 = (frame.x + frame.width) / atlas.width
v1 = (frame.y + frame.height) / atlas.height

If rotated == true:
    swap(width, height)
    UV coords rotated 90° CCW
```

### Complete Tile Frame Table

| Frame Name | Atlas Rect (x,y,w,h) | Source Size | Offset | Rotated |
|------------|---------------------|-------------|--------|---------|
| `tile_board.png` | 2,2,122,122 | 128×128 | 0,0 | No |
| `tile_grid.png` | 2,126,122,122 | 128×128 | 0,0 | No |
| `tile_hover.png` | 126,2,122,122 | 128×128 | 0,0 | No |
| `tile_large.png` | 126,126,122,122 | 128×128 | 0,0 | No |
| `tile_merged_large_0.png` | 532,128,61,61 | 64×64 | 1.5,-1.5 | No |
| `tile_merged_large_01.png` | 472,30,64,61 | 64×64 | 0,-1.5 | No |
| `tile_merged_large_03.png` | 388,159,61,64 | 64×64 | 1.5,0 | Yes |
| `tile_merged_large_013.png` | 466,93,64,64 | 64×64 | 0,0 | No |
| `tile_merged_large_0123.png` | 400,93,64,64 | 64×64 | 0,0 | No |
| `tile_merged_large_0_seam.png` | 517,191,61,61 | 64×64 | 1.5,-1.5 | No |
| `tile_path_move_start.png` | 250,206,71,12 | 128×128 | 28.5,0 | No |
| `tile_path_move_end.png` | 250,2,82,60 | 128×128 | -23,0 | No |
| `tile_path_move_straight.png` | 406,2,128,12 | 128×128 | 0,0 | No |
| `tile_path_move_corner.png` | 334,2,70,70 | 128×128 | -29,-29 | No |

---

## 14. Animation Timing Constants

### Source: `app/common/config.js:550-567`

| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.ANIMATE_FAST_DURATION` | `0.2` | Fast animations |
| `CONFIG.ANIMATE_MEDIUM_DURATION` | `0.35` | Medium animations |
| `CONFIG.ANIMATE_SLOW_DURATION` | `1.0` | Slow animations |
| `CONFIG.FADE_FAST_DURATION` | `0.2` | Fast fades |
| `CONFIG.FADE_MEDIUM_DURATION` | `0.35` | Medium fades |
| `CONFIG.FADE_SLOW_DURATION` | `1.0` | Slow fades |
| `CONFIG.PATH_FADE_DISTANCE` | `40.0` | Fade in/out distance |
| `CONFIG.PATH_ARC_DISTANCE` | `47.5` | Arc height |
| `CONFIG.PATH_ARC_ROTATION_MODIFIER` | `2.0` | Arc rotation factor |
| `CONFIG.PATH_MOVE_DURATION` | `1.5` | Path segment travel time |

---

## 15. Easing Functions

### TweenTypes Constants

### Source: `app/view/actions/TweenTypes.js`

```javascript
const TweenTypes = {
  TINT_FADE: 'tintFade',
  DISSOLVE: 'dissolve',
  GLOW_FADE: 'glowFade',
  GLOW_THICKNESS: 'glowThichness',
  HIGHLIGHT_FADE: 'highlightFade',
  BLOOM_INTENSITY: 'bloomIntensity',
};
```

### Standard Easing Types

| Easing Function | Curve Type |
|-----------------|------------|
| `cc.easeIn(rate)` | Accelerate |
| `cc.easeOut(rate)` | Decelerate |
| `cc.easeBackIn()` | Overshoot in |
| `cc.easeBackOut()` | Overshoot out |
| `cc.easeExponentialOut()` | Exp decelerate |
| `cc.easeCubicActionInOut()` | Cubic both |

---

## 16. Audio Feedback

### Source: `app/view/Player.js` + `app/data/resources.js`

### Audio Resources

| Resource | File | Purpose |
|----------|------|---------|
| `RSX.sfx_unit_select` | `sfx_ui_select.m4a` | Unit selection |
| `RSX.sfx_ui_in_game_hover` | `sfx_ui_in_game_hover.m4a` | Hover feedback |

### Trigger Points

```javascript
// Line 616: Unit selection
audio_engine.current().play_effect_for_interaction(RSX.sfx_unit_select.audio, CONFIG.SELECT_SFX_PRIORITY);

// Line 973, 1079, 1166: Hover events
audio_engine.current().play_effect(RSX.sfx_ui_in_game_hover.audio);
```

---

## 17. Network Hover Synchronization

### Source: `app/view/layers/game/GameLayer.js:6662-6725`

### Event Type

```javascript
network_game_hover: 'network_game_hover'
```

### Event Data Structure

```javascript
eventData = {
  playerId: number,
  handIndex: number|null,
  player1SignatureCard: boolean,
  player2SignatureCard: boolean,
  cardIndex: number|null,
  boardPosition: {x, y}|null,
  intentType: SDK.IntentType
};
```

---

## 18. AttackPathSprite Implementation

### Source: `app/view/nodes/map/AttackPathSprite.js`

### Properties

```javascript
AttackPathSprite = BaseSprite.extend({
  needsDepthDraw: true,
  depthModifier: 0.0,
  fadeDistance: CONFIG.PATH_FADE_DISTANCE,     // 40.0
  arcDistance: CONFIG.PATH_ARC_DISTANCE,       // 47.5
  speed: CONFIG.TILESIZE * (1.0 / CONFIG.PATH_MOVE_DURATION),  // 63.33 px/s
});
```

### Arc Animation (lines 78-115)

```javascript
update(dt) {
  const speedModifier = Math.min(1.5, Math.max(1.0, (CONFIG.TILESIZE * 2.0) / this._distance));
  this._currentDistance += this.speed * dt * speedModifier;

  const movePct = this._currentDistance / this._maxDistance;
  const arcPct = Math.min(this._currentDistance / this._distance, 1.0);
  const arcModifier = Math.sin(arcPct * Math.PI);

  const nextPosition = cc.p(
    this._sourceScreenPosition.x + dx * movePct,
    this._sourceScreenPosition.y + dy * movePct + this.arcDistance * arcModifier
  );

  this.setDepthOffset(-this.arcDistance * (arcModifier - 0.5) * 2.0);
  this.setAutoZOrderOffset((this.arcDistance / CONFIG.TILESIZE) * arcModifier * 1.5);

  const fadeIn = Math.min(this._currentDistance / this.fadeDistance, 1.0);
  const fadeOut = 1.0 - Math.min(Math.max(this._currentDistance - (this._distance - this.fadeDistance), 0.0) / this.fadeDistance, 1.0);
  this._pathOpacityModifier = fadeIn * fadeOut;
}
```

---

## 19. Depth Shader System

### Depth Write Shader

#### DepthVertex.glsl

```glsl
attribute vec4 a_position;
attribute vec2 a_texCoord;
uniform float u_depthOffset;
uniform float u_depthModifier;
varying vec2 v_texCoord;
varying float v_depthRange;

void main() {
    v_texCoord = a_texCoord;
    v_depthRange = ((a_position.y * (1.0 - u_depthModifier)) - u_depthOffset) * CC_MVMatrix[1][1];
    gl_Position = CC_PMatrix * CC_MVMatrix * a_position;
}
```

#### DepthFragment.glsl

```glsl
uniform vec2 u_resolution;
varying vec2 v_texCoord;
varying float v_depthRange;

void main() {
    float alpha = texture2D(CC_Texture0, v_texCoord).a;
    if (alpha < 0.1) discard;
    float depth = (gl_FragCoord.y - v_depthRange) / u_resolution.y;
    gl_FragColor = getPackedDepth(depth);
}
```

### Depth Test Shader

#### DepthTestFragment.glsl

```glsl
uniform sampler2D u_depthMap;
uniform vec2 u_resolution;
varying float v_depthRange;

void main() {
    vec2 screenCoord = gl_FragCoord.xy / u_resolution;
    if (getDepthTestFailed(u_depthMap, screenCoord, v_depthRange)) discard;
    gl_FragColor = v_fragmentColor * texture2D(CC_Texture0, v_texCoord);
}
```

### Depth Pack/Unpack

```glsl
// Pack depth to RGBA (24-bit precision)
vec4 getPackedDepth(in float depth) {
    const vec4 BIT_SHIFT = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
    const vec4 BIT_MASK = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
    vec4 packedDepth = mod(depth * BIT_SHIFT * vec4(255), vec4(256)) / vec4(255);
    packedDepth -= packedDepth.xxyz * BIT_MASK;
    return packedDepth;
}

// Unpack RGBA to depth
float getUnpackedDepth(in vec4 packedDepth) {
    const vec4 BIT_SHIFT = vec4(1.0/(256.0*256.0*256.0), 1.0/(256.0*256.0), 1.0/256.0, 1.0);
    return dot(packedDepth, BIT_SHIFT);
}

// Depth test
bool getDepthTestFailed(in sampler2D depthMap, in vec2 screenTexCoord, in float depth) {
    float screenDepth = getUnpackedDepth(texture2D(depthMap, screenTexCoord));
    return screenDepth < depth;
}
```

---

## 20. Game Layer Hierarchy

### Source: `app/view/layers/game/GameLayer.js:217-250`

```javascript
// FX Layer children (depth-tested)
this.getFXLayer().addChild(this.backgroundLayer, 0);
this.getFXLayer().addChild(this.tileLayer, 1);
this.getFXLayer().addChild(this.middlegroundLayer, 2);  // ShadowCastingLayer
this.getFXLayer().addChild(this.foregroundLayer, 3);

// NoFX Layer children (UI)
this.getNoFXLayer().addChild(this.bottomDeckLayer, 0);
this.getNoFXLayer().addChild(this.player1Layer, 0);
this.getNoFXLayer().addChild(this.player2Layer, 0);
this.getNoFXLayer().addChild(this.uiLayer, 1);
```

### UI Z-Order Constants

```javascript
_ui_z_order_low_priority_support_nodes: 1,
_ui_z_order_medium_priority_support_nodes: 2,
_ui_z_order_high_priority_support_nodes: 3,
_ui_z_order_indicators: 4,
_ui_z_order_battle_log: 5,
_ui_z_order_speech_nodes: 6,
_ui_z_order_notification_nodes: 7,
_ui_z_order_card_nodes: 8,
_ui_z_order_instructional_nodes: 9,
_ui_z_order_tooltip_nodes: 10,
```

---

## 21. Data Flow Summary

```
CONFIG (constants)
    │
    ├── BOARDCOL=9, BOARDROW=5, TILESIZE=95
    ├── XYZ_ROTATION={x:16,y:0,z:0}
    └── DEPTH_OFFSET=19.5
           │
           ▼
UtilsEngine.rebuild()
    │
    ├── _screenBoardSize = 855×475
    ├── _screenBoardOrigin = (centered + TILESIZE*0.5)
    └── _tileMapProjectionMatrix = PERSPECTIVE × ROTATION
           │
           ▼
TileLayer
    │
    ├── _boardNonBatchNode.setXYZRotation(XYZ_ROTATION)
    ├── _boardBatchNode → sprite batching
    └── _renderPass → offscreen + blit
           │
           ▼
EntityNode.setPosition(screenPos)
    │
    ├── contentSize = 95×95
    ├── autoZOrder = true
    ├── autoZOrderValue = 100 - boardY + offset
    └── depthOffset = 19.5 (shader uniform)
           │
           ▼
BaseSprite.WebGLRenderCmd
    │
    ├── setUniform(u_depthOffset, depthOffset)
    └── GPU depth test against depth map
```

---

## 22. Comparison with Sylvanshine

| Property | Sylvanshine | Duelyst |
|----------|-------------|---------|
| Columns | 9 | 9 |
| Rows | 5 | 5 |
| Tile Size | 48px base (×scale) | 95px |
| FOV | 60° | 60° |
| Board Rotation | 16° | 16° |
| Entity Rotation | 26° | 26° |
| Depth Offset | 19.5 | 19.5 |
| Movement | Manhattan | Manhattan |
| Attack Range | Chebyshev | Varies |
| Grid Rendering | Lines + quads | Tile textures |
| Perspective | Custom transforms | Cocos2d matrices |
