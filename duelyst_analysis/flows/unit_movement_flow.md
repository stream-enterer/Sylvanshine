# Flow: Unit Movement

## Trigger
Player orders a unit to move to a valid board position.

**Entry Points:**
- `MoveAction` for standard movement
- Player click on valid movement tile
- AI/bot movement decisions

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Action created | `app/sdk/actions/actionFactory.coffee:84` | `MoveAction` instantiated |
| T+0.0s | Validation start | `app/sdk/validators/validatorEntityAction.coffee:16-24` | Entity validation |
| T+0.0s | Can move check | `app/sdk/validators/validatorEntityAction.coffee:21` | `source.getCanMove()` |
| T+0.0s | Position valid check | `app/sdk/validators/validatorEntityAction.coffee:23` | `getMovementRange().getIsPositionValid()` |
| T+0.0s | Path calculation | `app/sdk/entities/movementRange.coffee:78-96` | `getPathTo()` BFS pathfinding |
| T+0.0s | Execute begins | `app/sdk/actions/moveAction.coffee:30-42` | `_execute()` called |
| T+0.0s | Cache path | `app/sdk/actions/moveAction.coffee:38` | Store computed path |
| T+0.0s | Position update | `app/sdk/actions/moveAction.coffee:41` | `entity.setPosition(targetPosition)` |
| T+0.0s | Moves increment | `app/sdk/actions/moveAction.coffee:42` | `entity.setMovesMade(+1)` |
| T+0.0s | View receives | `app/view/layers/game/GameLayer.js:3557-3561` | Detect MoveAction |
| T+0.0s | Show move | `app/view/layers/game/GameLayer.js:3560` | `node.showMove(action, source, target)` |
| T+0.0s | Calculate duration | `app/view/nodes/cards/EntityNode.js:394-414` | `getMoveDuration(tileCount)` |
| T+0.0s | Before move event | `app/view/nodes/cards/EntityNode.js:1260` | `EVENTS.before_show_move` |
| T+0.0s | Walk sound | `app/view/nodes/cards/EntityNode.js:1262-1264` | `play_effect(sfx_walk)` |
| T+0.0s-Xms* | Movement tween | `app/view/nodes/cards/EntityNode.js:1282` | `cc.moveTo(duration, position)` |
| T+0.0s-Xms | Walk animation | `app/view/nodes/cards/EntityNode.js:1281-1284` | Walk cycle plays |
| T+Xms | After move event | `app/view/nodes/cards/EntityNode.js:1239` | `EVENTS.after_show_move` |
| T+Xms | Movement complete | `app/view/nodes/cards/EntityNode.js:1236` | `isMoving = false` |

*X = movement duration based on tile count and animation speed

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Input | Player | Select unit and target tile | `app/view/Player.js` |
| Creation | ActionFactory | Create MoveAction | `app/sdk/actions/actionFactory.coffee:84` |
| Validation | ValidatorEntityAction | Validate movement | `app/sdk/validators/validatorEntityAction.coffee:16-24` |
| Pathfinding | MovementRange | Calculate valid path | `app/sdk/entities/movementRange.coffee:19-76` |
| Execution | MoveAction | Update entity position | `app/sdk/actions/moveAction.coffee:30-42` |
| Execution | Entity | Set new position | `app/sdk/entities/entity.coffee` (setPosition) |
| View | GameLayer | Orchestrate movement display | `app/view/layers/game/GameLayer.js:3557-3561` |
| Animation | EntityNode | Execute move animation | `app/view/nodes/cards/EntityNode.js:1198-1307` |
| Animation | EntityNode | Calculate duration | `app/view/nodes/cards/EntityNode.js:394-414` |
| Audio | AudioEngine | Play walk sound | `app/audio/audio_engine.js` (play_effect) |

---

## Data Transformations

**Input:** Unit at source position
```javascript
{
  position: { x: 2, y: 3 },
  movesMade: 0,
  speed: 2,  // Can move 2 tiles
  isExhausted: false,
  hasMovesLeft: true
}
```

**Output:** Unit at target position
```javascript
{
  position: { x: 4, y: 3 },  // Moved 2 tiles
  movesMade: 1,             // Incremented
  speed: 2,
  isExhausted: false,
  hasMovesLeft: false       // Used move for turn
}
```

**Side Effects:**
- Entity position updated on board
- `entity.movesMade` incremented
- Movement path cached in action
- Board spatial index updated
- EntityNode visual position tweened

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.ENTITY_MOVE_DURATION_MODIFIER` | 1.0 | `app/common/config.js:733` | Scale movement speed |
| `CONFIG.ENTITY_MOVE_MODIFIER_MAX` | 1.0 | `app/common/config.js:736` | Max movement multiplier |
| `CONFIG.ENTITY_MOVE_MODIFIER_MIN` | 0.75 | `app/common/config.js:738` | Min movement multiplier |
| `CONFIG.ENTITY_MOVE_CORRECTION` | 0.2 | `app/common/config.js:740` | Final step timing correction |
| `CONFIG.ENTITY_MOVE_FLYING_FIXED_TILE_COUNT` | 3.0 | `app/common/config.js:744` | Flying unit animation cap |
| `CONFIG.PATH_MOVE_DURATION` | 1.5s | `app/common/config.js:545` | Path visualization duration |
| `CONFIG.MOVE_FAST_DURATION` | 0.15s | `app/common/config.js:562` | Fast movement transition |
| `CONFIG.MOVE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:563` | Medium movement transition |

**Duration Calculation (EntityNode.js:394-414):**
```javascript
getMoveDuration(moveTileCount) {
  const animMove = this.getAnimationActionFromAnimResource('walk');
  if (animMove != null) {
    let animMoveDuration = animMove.getDuration() * CONFIG.ENTITY_MOVE_DURATION_MODIFIER;
    let animMoveCorrection = animMoveDuration * CONFIG.ENTITY_MOVE_CORRECTION;
    for (let i = 0; i < moveTileCount; i++) {
      movementDuration += animMoveDuration;
    }
    // Subtract correction for accurate final positioning
    movementDuration = Math.max(0.0, movementDuration - animMoveCorrection);
  }
  return movementDuration;
}
```

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Move Start | - | `soundResource.walk` | `EntityNode.js:1262-1264` |
| Before Event | `before_show_move` emit | - | `EntityNode.js:1260` |
| Walk Cycle | Walk animation loops | - | `EntityNode.js:1281-1284` |
| Position Tween | `cc.moveTo()` spatial | - | `EntityNode.js:1282` |
| After Event | `after_show_move` emit | - | `EntityNode.js:1239` |
| Path Tiles | Tile highlights | - | During selection |

**Sound Resource:**
```javascript
soundResource: {
  walk: "sfx_footstep.audio"  // Movement sound
}
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Unit cannot move | `validatorEntityAction.coffee:21` | Action invalidated | "Unit cannot move" |
| Invalid position | `validatorEntityAction.coffee:23` | Action invalidated | "Invalid move position" |
| Not an entity | `validatorEntityAction.coffee:20` | Action invalidated | "Not a valid move" |
| Unit exhausted | `entity.getIsExhausted()` | Fails `getCanMove()` | "Unit is exhausted" |
| No moves left | `entity.getHasMovesLeft()` | Fails `getCanMove()` | "No moves remaining" |
| Speed is 0 | `entity.getSpeed() <= 0` | Fails `getCanMove()` | "Unit cannot move" |
| Path blocked | `movementRange.getPathTo()` returns [] | No valid path | Tile not highlighted |
| Off board | `board.isOnBoard(position)` | Position invalid | N/A |

**Validation Code (validatorEntityAction.coffee:16-24):**
```coffeescript
if action.getType() == MoveAction.type
  source = action.getSource()

  if !(source instanceof Entity)
    @invalidateAction(action, action.getTargetPosition(),
      i18next.t("validators.not_a_valid_move_message"))

  if !source.getCanMove()
    @invalidateAction(action, source.getPosition(),
      i18next.t("validators.unit_cannot_move_message"))

  if !source.getMovementRange().getIsPositionValid(action.getTargetPosition())
    @invalidateAction(action, action.getTargetPosition(),
      i18next.t("validators.invalid_move_position_message"))
```

---

## Pathfinding Algorithm

**Location:** `app/sdk/entities/movementRange.coffee:19-76`

**Algorithm:** Breadth-First Search (BFS)

```coffeescript
getValidPositions: () ->
  entity = @getEntity()
  board = entity.getGameSession().getBoard()
  speed = entity.getSpeed()

  # BFS from entity position
  startPosition = entity.getPosition()
  openList = [{ position: startPosition, speed: 0, path: [] }]

  while openList.length > 0
    node = openList.shift()

    # Check 4 directions (CONFIG.MOVE_PATTERN_STEP)
    for step in CONFIG.MOVE_PATTERN_STEP
      movePosition = { x: node.position.x + step.x, y: node.position.y + step.y }
      nextSpeed = node.speed + distance

      # Stop if exceeds entity speed
      if nextSpeed > speed
        continue

      # Check for obstruction
      obstructionAtPosition = board.getObstructionAtPositionForEntity(movePosition, entity)

      # Flying units can pass through friendly units
      if !obstructionAtPosition or entity.hasActiveModifierClass(ModifierFlying)
        # Add to valid positions
        validPositions.push(movePosition)
        path = node.path.concat(movePosition)
        openList.push({ position: movePosition, speed: nextSpeed, path: path })
```

**Movement Pattern (config.js:792-797):**
```javascript
CONFIG.MOVE_PATTERN_STEP = [
  { x: 0, y: 1 },   // Up
  { x: 0, y: -1 },  // Down
  { x: -1, y: 0 },  // Left
  { x: 1, y: 0 }    // Right
];
```

---

## Flying Unit Special Handling

**Detection:** `entity.hasActiveModifierClass(ModifierFlying)`

**Pathfinding Difference (movementRange.coffee:63):**
- Flying units can path through friendly AND enemy units
- Can only stop on unobstructed tiles or same-team tiles

**Animation Difference (EntityNode.js:387-388):**
```javascript
if (this.sdkCard.hasActiveModifierClass(SDK.ModifierFlying)) {
  tileCount = Math.min(tileCount, CONFIG.ENTITY_MOVE_FLYING_FIXED_TILE_COUNT);  // Cap at 3
}
```

Flying units have capped animation tile count (3) for faster visual movement regardless of actual distance.

---

## Movement Capability Check

**Location:** `app/sdk/entities/entity.coffee:395-396`

```coffeescript
getCanMove: () ->
  return !@getIsExhausted() and @getHasMovesLeft() and @getSpeed() > 0
```

**Components:**
- `getIsExhausted()`: Unit has summoning sickness or used all actions
- `getHasMovesLeft()`: `movesMade < moves` (usually 1 move per turn)
- `getSpeed()`: Movement range (tiles per move)

**Related Properties (entity.coffee:42-56):**
```coffeescript
speed: 0        # How far unit can move
moves: 1        # Max moves per turn
movesMade: 0    # Moves already made this turn
exhausted: true # Exhaustion status (summoning sickness)
```

---

## View Animation Sequence

**Location:** `app/view/nodes/cards/EntityNode.js:1198-1307`

```javascript
showMove(action, sourceBoardPosition, targetBoardPosition) {
  // 1. Calculate tile count and duration
  const tileCount = this.getMoveTileCount(source, target);
  const moveDuration = this.getMoveDuration(tileCount);

  // 2. Get walk animation
  const animAction = this.getAnimationActionFromAnimResource('walk');

  // 3. Set moving flag
  this.isMoving = true;

  // 4. Emit before_show_move event
  this.emit(EVENTS.before_show_move, ...);

  // 5. Play walk sound
  audio_engine.current().play_effect(sfx_walk, false);

  // 6. Create animation sequence
  // - Clone walk animation for each tile
  // - Spawn movement tween in parallel with animation
  this.entitySprite.runAction(
    cc.spawn(
      cc.moveTo(moveDuration, targetTileMapPosition),
      animationSequence
    )
  );

  // 7. On completion
  this.isMoving = false;
  this.emit(EVENTS.after_show_move, ...);

  return showDuration;
}
```

---

## Board Coordinate Systems

**Board Position:** Integer tile coordinates (0-8 x, 0-4 y)
- Used by SDK for game logic
- `CONFIG.BOARDCOL = 9`, `CONFIG.BOARDROW = 5`

**TileMap Position:** Pixel coordinates for rendering
- Converted via `UtilsEngine.transformBoardToTileMap()`
- Used by view layer for animations

**Position Conversion (EntityNode.js:1209-1210):**
```javascript
const sourceTileMapPosition = UtilsEngine.transformBoardToTileMap(sourceBoardPosition);
const targetTileMapPosition = UtilsEngine.transformBoardToTileMap(targetBoardPosition);
```
