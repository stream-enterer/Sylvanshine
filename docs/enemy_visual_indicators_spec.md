# Enemy Visual Indicators System Specification

Forensic analysis of Duelyst's enemy ownership and interaction visual feedback systems.

**Sources:** `EntityNode.js`, `Player.js`, `GameLayer.js`, `config.js`, tile sprite classes

---

## 1. System Overview

Duelyst has three distinct visual indicator systems for enemy units:

| System | Purpose | When Visible |
|--------|---------|--------------|
| **Ownership Indicator** | Persistent red tile under enemy units | Idle state only |
| **Enemy Preview Blob** | Shows enemy attack range on YOUR hover | Hover over enemy (your turn) |
| **Opponent Action Display** | Shows opponent's selection/movement during THEIR turn | Opponent's turn (network sync) |

---

## 2. Ownership Indicator (`tile_grid.png`)

### 2.1 Visual Properties

| Property | Enemy Value | Player Value | Source |
|----------|-------------|--------------|--------|
| Texture | `tile_grid.png` (122×122) | Same | `TileMapGridSprite.js:15` |
| Opacity | **80/255 (31%)** | **0 (hidden)** | `config.js:808,812` |
| Color | `{r:255, g:0, b:0}` (red) | `{r:0, g:200, b:255}` (cyan, unused) | `config.js:809-811` |

### 2.2 Visibility State Machine

**Source:** `EntityNode.js:486-495`

```javascript
getCanShowOwnerSprites() {
  return sdkCard != null
    && (enemy && OPPONENT_OWNER_OPACITY > 0) || (player && PLAYER_OWNER_OPACITY > 0)
    && !this.hovered           // Hidden on hover
    && !this.selected          // Hidden on selection
    && !this.isValidTarget     // Hidden when targetable
    && !this.getIsMoving()     // Hidden during movement
    && this.getIsSpawned()     // Must be spawned
    && gameLayer.getIsActive() // Game must be active
}
```

### 2.3 State Transition Triggers

| Trigger | Function | Line | Effect |
|---------|----------|------|--------|
| Mouse enters unit | `setHovered(true)` | 353 | Hide indicator |
| Mouse exits unit | `setHovered(false)` | 353 | Show indicator (if idle) |
| Unit selected | `setSelected(true)` | 335 | Hide indicator |
| Unit deselected | `setSelected(false)` | 335 | Show indicator (if idle) |
| Marked as target | `setIsValidTarget(true)` | 318 | Hide indicator |
| Target cleared | `setIsValidTarget(false)` | 318 | Show indicator (if idle) |
| Movement starts | `setIsMoving(true)` | 364 | Hide indicator |
| Movement ends | `setIsMoving(false)` | 364 | Show indicator (if idle) |
| Unit spawns | `showSpawn()` | 993 | Show indicator |

### 2.4 Key Insight: Inverted Logic

The ownership indicator is **NOT** an "on hover" effect. It's an **"idle state"** indicator that is **hidden** during interaction states. This creates the visual effect of the red tile "disappearing" when you interact with an enemy unit.

---

## 3. Enemy Preview Blob (Your Turn Hover)

### 3.1 When Triggered

When **you** (the player) hover over an **enemy** unit during **your turn**, while **not** having a unit selected.

**Source:** `Player.js:986-989`

```javascript
// show preview tiles for my player when not taking a selection action
if (isForMyPlayer && !this.getIsTakingSelectionAction()) {
  this.previewEntityTiles();
}
```

### 3.2 What Is Shown

**Source:** `Player.js:1372-1458`

For **enemy** units when previewing (`isMyEntity = false`):

| Element | Shown? | Color | Reason |
|---------|--------|-------|--------|
| **Movement blob** | **NO** | N/A | Line 1385: `isMyEntity` check fails |
| **Attack blob** | **YES** | `AGGRO_OPPONENT_COLOR` (#D22846) | Line 1393: `!isMyEntity` passes |
| **Attackable targets** | NO (preview only) | N/A | Line 1400: `!preview` check |

```javascript
// Line 1385: Movement tiles only for own units
if (moveLocations && moveLocations.length > 1 && isMyEntity) {
  hasMoveTiles = true;  // FALSE for enemy
}

// Line 1393: Attack pattern for enemy units
if (!isMyEntity || CONFIG.SHOW_MERGED_MOVE_ATTACK_TILES) {
  attackPattern = sdkEntity.getAttackRange().getValidPositions(...);
}

// Line 1457: Show attack tiles with opponent color
this.showMergedTilesForAction(..., CONFIG.AGGRO_OPPONENT_COLOR, ...);
```

### 3.3 Color Constants

| Constant | RGB | Hex | Usage |
|----------|-----|-----|-------|
| `AGGRO_OPPONENT_COLOR` | 210, 40, 70 | #D22846 | Enemy attack range blob |
| `AGGRO_OPPONENT_ALT_COLOR` | 130, 25, 45 | #82192D | Enemy attack path |
| `MOVE_OPPONENT_COLOR` | 240, 240, 240 | #F0F0F0 | Enemy movement (not shown in preview) |

### 3.4 Summary: Your Turn Enemy Hover

When you hover an enemy unit on your turn:
1. **Ownership indicator hides** (hover state = true)
2. **Attack range blob appears** in red (#D22846)
3. **Movement range is NOT shown** (only attack)

---

## 4. Opponent Action Display (Their Turn)

### 4.1 Network Synchronization

When the **opponent** hovers/selects units on **their** turn, Duelyst broadcasts network events that trigger visual feedback on **your** screen.

**Source:** `GameLayer.js:6662-6725, 6726-6789`

### 4.2 Network Events

| Event | Trigger | Handler |
|-------|---------|---------|
| `network_game_hover` | Opponent moves mouse | `onNetworkHover()` |
| `network_game_select` | Opponent clicks unit | `onNetworkSelect()` |

### 4.3 Opponent Turn Hover Flow

**Source:** `GameLayer.js:6682-6718`

```javascript
// Line 6682: Only during opponent turn
if ((this.getIsGameActive() || this.getIsChooseHand()) && !this.getIsMyTurn()) {
  // Line 6700-6707: Show hover for opponent's moused-over unit
  if (eventData.cardIndex != null) {
    const entityNode = _.find(this._unitNodes, ...);
    playerActor.setMouseOverEntityNode(entityNode);  // playerActor = _opponent
  }

  // Line 6716-6718: Show hover tiles
  if (eventData.boardPosition != null) {
    playerActor.showHover();
  }
}
```

### 4.4 Opponent Selection Flow

**Source:** `GameLayer.js:6747-6777`

```javascript
// Line 6747: Only during opponent turn
if (this.getIsGameActive() && !this.getIsMyTurn()) {
  // Line 6766-6770: Select unit for opponent player
  if (eventData.cardIndex != null) {
    playerActor.setSelectedEntityNode(entityNode);  // playerActor = _opponent
  }
}
```

### 4.5 What You See During Opponent's Turn

When opponent selects their unit (which is **your enemy**):

| Element | Color | Source |
|---------|-------|--------|
| Selection box | `SELECT_OPPONENT_COLOR` (#D22846) | `Player.js:1369` |
| Movement blob | `MOVE_OPPONENT_COLOR` (#F0F0F0) | `Player.js:1437` |
| Attack blob | `AGGRO_OPPONENT_COLOR` (#D22846) | `Player.js:1457` |
| Movement path | `AGGRO_OPPONENT_ALT_COLOR` (#82192D) | `Player.js:1813` |
| Target reticle | `AGGRO_OPPONENT_COLOR` (#D22846) | `Player.js:1825` |

**Key distinction:** The `isForMyPlayer` check at line 1372-1373 determines if tiles are shown:

```javascript
// only show move/attack tiles for own selection/preview
if (isForMyPlayer) {
```

But `_opponent` player actor has `isForMyPlayer = false`, so... **wait, this means the blobs wouldn't show?**

Let me re-examine. The `_opponent` is a `Player` instance representing the opponent. When `_opponent.setSelectedEntityNode()` is called, inside `showEntityTiles()`:
- `isForMyPlayer = this.getIsMyPlayer()` → FALSE for `_opponent`
- Line 1372: `if (isForMyPlayer)` → Skipped!

**Correction:** The movement/attack tiles are **NOT** shown for opponent selections via network events. Only the selection box is shown.

### 4.6 Revised: Opponent Turn Visual Feedback

| Element | Shown? | Color |
|---------|--------|-------|
| Selection box | YES | `SELECT_OPPONENT_COLOR` (#D22846) |
| Movement blob | **NO** | (skipped by `isForMyPlayer` check) |
| Attack blob | **NO** | (skipped by `isForMyPlayer` check) |
| Hover tile | YES | `MOUSE_OVER_OPPONENT_COLOR` (#D22846) |
| Path | Partial | `AGGRO_OPPONENT_ALT_COLOR` (#82192D) |

---

## 5. TileOpponentSprite

### 5.1 Properties

**Source:** `TileOpponentSprite.js`, `tiles_action.plist`

| Property | Value |
|----------|-------|
| Texture | `tile_opponent.png` |
| Atlas position | 174,178 |
| Size | 66×46 (trimmed from 80×80) |
| Source size | 80×80 |

### 5.2 When Used

**Source:** `Player.js:1746-1748`

```javascript
// Passive hover over enemy unit, no unit selected
if (needsPassiveHover) {
  if (!isForMyPlayer && !isAltPlayer && !selectedSdkEntity) {
    this.showOpponentTile(mouseScreenBoardPosition, this.boardTileZOrder, opacity, fadeDuration, CONFIG.AGGRO_OPPONENT_ALT_COLOR);
  }
}
```

This shows when:
- Mouse is on board
- No unit is selected
- Hovering over enemy unit position
- From opponent's perspective (network sync)

---

## 6. Selection Box Behavior

### 6.1 Color Logic

**Source:** `Player.js:1369`

```javascript
this.showBoxTileForSelect(
  UtilsEngine.transformBoardToScreen(entityPosition),
  opacity,
  fadeDuration,
  (isForMyPlayer || isAltPlayer) ? CONFIG.SELECT_COLOR : CONFIG.SELECT_OPPONENT_COLOR
);
```

| Condition | Color |
|-----------|-------|
| `isForMyPlayer` or `isAltPlayer` | `SELECT_COLOR` (#FFFFFF) |
| Opponent (`_opponent` player) | `SELECT_OPPONENT_COLOR` (#D22846) |

### 6.2 Pulsing Animation

**Source:** `Player.js:2295-2301`

```javascript
_showPulsingScaleTile(tile, position, opacity, fadeDuration, color, scale) {
  if (scale == null) { scale = 0.85; }
  tile.startPulsingScale(CONFIG.PULSE_MEDIUM_DURATION, scale);
}
```

Selection box pulses between 0.85-1.0 scale.

---

## 7. Color Reference Table

| Constant | RGB | Hex | Usage |
|----------|-----|-----|-------|
| `SELECT_COLOR` | 255,255,255 | #FFFFFF | Player selection box |
| `SELECT_OPPONENT_COLOR` | 210,40,70 | #D22846 | Enemy selection box |
| `MOVE_COLOR` | 240,240,240 | #F0F0F0 | Player movement blob |
| `MOVE_OPPONENT_COLOR` | 240,240,240 | #F0F0F0 | Enemy movement blob |
| `AGGRO_COLOR` | 255,217,0 | #FFD900 | Player attack blob |
| `AGGRO_OPPONENT_COLOR` | 210,40,70 | #D22846 | Enemy attack blob |
| `AGGRO_OPPONENT_ALT_COLOR` | 130,25,45 | #82192D | Enemy path/target |
| `MOUSE_OVER_COLOR` | 255,255,255 | #FFFFFF | Player hover |
| `MOUSE_OVER_OPPONENT_COLOR` | 210,40,70 | #D22846 | Enemy hover |
| `OPPONENT_OWNER_COLOR` | 255,0,0 | #FF0000 | Ownership indicator |

---

## 8. Implementation Gaps in Sylvanshine

### 8.1 Current State

| Feature | Duelyst | Sylvanshine | Gap |
|---------|---------|-------------|-----|
| Ownership indicator | Idle-state red tile | Hover-only red tile | Inverted logic |
| Enemy attack preview | Red blob on hover | Not implemented | Missing |
| Opponent turn display | Network-synced tiles | N/A (single player) | Not applicable |

### 8.2 Required Changes

**For ownership indicator:**
```cpp
// Render for IDLE enemies (not hovered, not selected, not moving)
for (const auto& unit : state.units) {
    if (unit.type == UnitType::Enemy && unit.hp > 0) {
        bool is_idle = (unit.board_pos != state.hover_pos) &&
                       (state.selected_unit_idx < 0 ||
                        state.units[state.selected_unit_idx].board_pos != unit.board_pos) &&
                       !unit.is_moving;
        if (is_idle) {
            grid_renderer.render_enemy_indicator(config, unit.board_pos);
        }
    }
}
```

**For enemy attack preview on hover:**
```cpp
// When hovering enemy unit (no friendly unit selected)
if (hovered_enemy && state.selected_unit_idx < 0) {
    auto attack_range = get_attack_pattern(hovered_enemy.board_pos, hovered_enemy.attack_range);
    grid_renderer.render_attack_blob(config, attack_range,
                                      TileOpacity::FULL,
                                      {}, // no alt blob
                                      AGGRO_OPPONENT_COLOR);
}
```

---

## 9. State Diagram

```
                              ┌─────────────────┐
                              │   IDLE STATE    │
                              │ (Red tile shown)│
                              └────────┬────────┘
                                       │
           ┌───────────────────────────┼───────────────────────────┐
           │                           │                           │
           ▼                           ▼                           ▼
  ┌────────────────┐         ┌─────────────────┐         ┌─────────────────┐
  │    HOVERED     │         │    SELECTED     │         │     MOVING      │
  │ (Red tile      │         │ (Red tile       │         │ (Red tile       │
  │  hidden,       │         │  hidden,        │         │  hidden)        │
  │  attack blob   │         │  selection box  │         │                 │
  │  shown)        │         │  + range blobs) │         │                 │
  └────────────────┘         └─────────────────┘         └─────────────────┘
           │                           │                           │
           │    Mouse exits            │    Deselected             │    Movement ends
           └───────────────────────────┴───────────────────────────┘
                                       │
                                       ▼
                              ┌─────────────────┐
                              │   IDLE STATE    │
                              │ (Red tile shown)│
                              └─────────────────┘
```

---

## 10. Complete Tile Sprite Catalogue

### 10.1 Grid Tile Types

| Sprite Class | Asset | Purpose | Player Color | Enemy Color |
|--------------|-------|---------|--------------|-------------|
| `TileMapHoverSprite` | `tile_hover.png` | Hover indicator | #FFFFFF | #D22846 |
| `TileBoxSprite` | `tile_box.png` | Selection box | #FFFFFF | #D22846 |
| `TileMapLargeSprite` | `tile_large.png` | Attack range blob | #FFD900 | #D22846 |
| `TileMapGridSprite` | `tile_grid.png` | Ownership indicator | Hidden | #FF0000 @ 31% |
| `TileGlowSprite` | `tile_glow.png` | Movement destination | #FFFFFF | #F0F0F0 |
| `TileAttackSprite` | `tile_attack.png` | Attack target reticle | #FFD900 | #D22846 |
| `TileSpawnSprite` | `tile_spawn.png` | Unit spawn location | #FFFFFF | N/A |
| `TileSpellSprite` | `tile_spell.png` | Spell target location | #FFFFFF | N/A |
| `TileCardSprite` | `tile_card.png` | Card play location | #FFFFFF | N/A |
| `TileOpponentSprite` | `tile_opponent.png` | Passive enemy hover | N/A | #82192D |

### 10.2 Merged Tile Variants (Corner Detection)

For seamless blobs, Duelyst uses corner-aware sprites:

| Pattern | Sprite | Description |
|---------|--------|-------------|
| `0` | `TileMapMergedLarge0Sprite` | Single tile (no neighbors) |
| `01` | `TileMapMergedLarge01Sprite` | Horizontal pair |
| `03` | `TileMapMergedLarge03Sprite` | Vertical pair |
| `013` | `TileMapMergedLarge013Sprite` | L-shape |
| `0123` | `TileMapMergedLarge0123Sprite` | All neighbors |
| `0Seam` | `TileMapMergedLarge0SeamSprite` | Move/attack boundary |

Hover variants (`TileMapMergedHover*`) exist for lower-opacity hover previews.

### 10.3 Path Tile System

| Sprite | Asset | Usage |
|--------|-------|-------|
| `TileMapPathMoveStartSprite` | `tile_path_start.png` | Path origin |
| `TileMapPathMoveEndSprite` | `tile_path_end.png` | Path destination |
| `TileMapPathMoveStraightSprite` | `tile_path_straight.png` | Horizontal/vertical segment |
| `TileMapPathMoveCornerSprite` | `tile_path_corner.png` | Turn in path |
| `*FromStart` variants | — | When path starts from tile |
| `*Flipped` variants | — | Mirror versions |

### 10.4 Attack Path

| Sprite | Asset | Purpose |
|--------|-------|---------|
| `AttackPathSprite` | Custom | Arc from attacker to target (ranged) |

**Source:** `AttackPathSprite.js` - draws bezier curve for ranged attack visualization.

---

## 11. Entity Visual State Tags

Beyond grid tiles, Duelyst has entity-level visual effects applied via `EntityNodeVisualStateTag`:

### 11.1 Available Tags

| Tag Type | Function | Effect |
|----------|----------|--------|
| `showTargetableTagType` | `createShowTargetableTag()` | Marks entity as valid target |
| `showDeemphasisTagType` | `createShowDeemphasisTag()` | Dims non-active entities |
| `showDissolveTagType` | `createShowDissolveTag()` | Death/removal dissolve |
| `showReadinessForPlayerTagType` | `createShowReadinessForPlayerTag()` | Ready-to-act indicator (player) |
| `showReadinessForOpponentTagType` | `createShowReadinessForOpponentTag()` | Ready-to-act indicator (opponent) |
| `showHoverForPlayerTagType` | `createShowHoverForPlayerTag()` | Player hover highlight |
| `showHoverForOpponentTagType` | `createShowHoverForOpponentTag()` | Opponent hover highlight |
| `showGlowForPlayerTagType` | `createShowGlowForPlayerTag()` | Player selection glow |
| `showGlowForOpponentTagType` | `createShowGlowForOpponentTag()` | Opponent selection glow |
| `showInstructionalGlowTagType` | `createShowInstructionalGlowTag()` | Tutorial highlight |
| `showHighlightTagType` | `createHighlightTag()` | Generic pulsing highlight |

### 11.2 Highlight Tag Parameters

**Source:** `EntityNodeVisualStateTag.js:123-131`

```javascript
createHighlightTag(showHighlight, priority, color, frequency, minAlpha, maxAlpha)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `showHighlight` | boolean | Enable/disable |
| `priority` | number | Stacking priority |
| `color` | {r,g,b} | Highlight color |
| `frequency` | number | Pulse speed |
| `minAlpha` | number | Minimum opacity |
| `maxAlpha` | number | Maximum opacity |

### 11.3 Usage Pattern

```javascript
// Mark entity as targetable with pulsing highlight
entityNode.addInjectedVisualStateTagWithId(
  EntityNodeVisualStateTag.createHighlightTag(
    true,           // show
    0,              // priority
    {r:255, g:0, b:0},  // color
    2.0,            // frequency
    0.3,            // minAlpha
    0.8             // maxAlpha
  ),
  'attack_target_tag'
);
```

---

## 12. Bot vs Network Game Differences

### 12.1 Architecture Distinction

| Game Type | Port | Authoritative | Network Events |
|-----------|------|---------------|----------------|
| Single Player / Bot | 8000 | Client-side | **Disabled** |
| Boss Battle | 8000 | Client-side | **Disabled** |
| Challenge / Sandbox | Local | Client-side | **Disabled** |
| Ranked / Casual / Gauntlet | 8001 | Server-side | **Enabled** |

**Source:** `networkManager.coffee:72`, `gameType.coffee:21-25`

### 12.2 Network Event Suppression (Bot Games)

**Source:** `networkManager.coffee:258-286`

```coffeescript
broadcastGameEvent: (eventData) ->
  # don't broadcast anything if we're a specator or running locally
  if @.spectatorId || gameSession.getIsSpectateMode() || gameSession.getIsRunningAsAuthoritative()
    return false

  validForBroadcast = eventData? and @socket? and @socket.connected and !gameSession.getIsRunningAsAuthoritative()
```

**Result:** In bot/single-player games:
- `network_game_hover` events → **Never sent/received**
- `network_game_select` events → **Never sent/received**
- `network_game_mouse_clear` events → **Never sent/received**

### 12.3 What You See During Bot's Turn

| Feature | Network Game | Bot Game |
|---------|--------------|----------|
| Opponent selection box | YES (red) | **NO** |
| Opponent movement preview | NO* | **NO** |
| Opponent attack range | NO* | **NO** |
| Opponent hover path | YES | **NO** |
| Opponent emotes | YES (network) | YES (hardcoded GLHF/GG) |

*Movement/attack tiles blocked by `isForMyPlayer` check even in network games

### 12.4 Bot Emote System

**Source:** `game.js:260-264, 701-705`

The only "opponent feedback" in bot games is scripted emotes:

```javascript
// On game active
if (SDK.GameSession.getInstance().isSinglePlayer() && !this._aiHasShownGLHF) {
  this._aiHasShownGLHF = true;
  this.showAIEmote(SDK.CosmeticsLookup.Emote.TextGLHF);
}

// On game over
if (SDK.GameSession.getInstance().isSinglePlayer() && !this._aiHasShownGG) {
  this._aiHasShownGG = true;
  this.showAIEmote(SDK.CosmeticsLookup.Emote.TextGG);
}
```

### 12.5 Visual Feedback Comparison

```
┌──────────────────────────────────────────────────────────────────┐
│                     YOUR TURN (Both Game Types)                  │
├──────────────────────────────────────────────────────────────────┤
│  Hover friendly unit:                                            │
│    ✓ White selection glow                                        │
│    ✓ Movement blob (white/gray)                                  │
│    ✓ Attack blob (yellow)                                        │
│    ✓ Ownership indicator HIDDEN                                  │
│                                                                  │
│  Hover enemy unit:                                               │
│    ✓ Red attack blob (#D22846)                                   │
│    ✗ No movement blob (blocked by isMyEntity check)              │
│    ✓ Ownership indicator HIDDEN                                  │
│                                                                  │
│  Idle enemy units:                                               │
│    ✓ Red ownership indicator (#FF0000 @ 31%)                     │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                 OPPONENT'S TURN - NETWORK GAME                   │
├──────────────────────────────────────────────────────────────────┤
│  Opponent hovers their unit:                                     │
│    ✓ Red hover tile on position (via network sync)               │
│    ✗ No movement/attack blobs (blocked by isForMyPlayer)         │
│                                                                  │
│  Opponent selects their unit:                                    │
│    ✓ Red selection box (#D22846) on unit                         │
│    ✗ No movement/attack blobs (blocked by isForMyPlayer)         │
│                                                                  │
│  Opponent drags to move/attack:                                  │
│    ✓ Red path line (#82192D) following cursor                    │
│    ✓ Target indicator at destination                             │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                   OPPONENT'S TURN - BOT GAME                     │
├──────────────────────────────────────────────────────────────────┤
│  AI "thinks":                                                    │
│    ✗ No visual feedback whatsoever                               │
│    ✗ No hover indicators                                         │
│    ✗ No selection boxes                                          │
│    ✗ No path previews                                            │
│                                                                  │
│  AI executes action:                                             │
│    ✓ Unit movement animation plays                               │
│    ✓ Attack animation plays                                      │
│    ✓ Damage numbers appear                                       │
│    (Actions just "happen" without preview)                       │
└──────────────────────────────────────────────────────────────────┘
```

### 12.6 Implications for Sylvanshine

Since Sylvanshine is a **single-player tactics game**:

| System | Relevance | Notes |
|--------|-----------|-------|
| Ownership indicator | **HIGH** | Shows enemy units at a glance |
| Enemy attack preview on hover | **HIGH** | Helps player plan attacks |
| Opponent turn visualization | **DESIGN CHOICE** | Not required, but could add polish |
| Network sync | **NONE** | No multiplayer |

**Recommendation:** Consider adding optional "enemy thinking" visualization during enemy turn for UX polish. This would be a Sylvanshine-original feature (not from Duelyst) since Duelyst's bot provides no such feedback.

---

## 13. Audit Log

| Date | Auditor | Scope | Findings |
|------|---------|-------|----------|
| 2025-12-24 | Claude Code | Ownership indicator | Visibility is conditional on idle state, NOT always visible |
| 2025-12-24 | Claude Code | Enemy preview | Attack blob only (no movement) shown on your hover |
| 2025-12-24 | Claude Code | Network display | Opponent tiles NOT shown due to `isForMyPlayer` check |
| 2025-12-24 | Claude Code | Color verification | All color constants verified against config.js |
| 2025-12-24 | Claude Code | Bot vs Network | Bot games have NO opponent visualization (network events disabled) |
| 2025-12-24 | Claude Code | Tile catalogue | Documented all 10 tile sprite types + merged variants |
| 2025-12-24 | Claude Code | Entity tags | Documented 11 EntityNodeVisualStateTag types |
