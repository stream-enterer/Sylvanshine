# Flow: Unit Death

## Trigger
Unit HP reaches 0 or below, or unit is killed by a direct kill effect.

**Entry Points:**
- `DamageAction._execute()` when `target.getHP() <= 0` after damage
- `KillAction._execute()` for direct kill effects (destroy, sacrifice)
- `RemoveAction` as base class for board removal

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Damage applied | `app/sdk/actions/damageAction.coffee:86-94` | `target.applyDamage(dmg)` |
| T+0.0s | HP check | `app/sdk/actions/damageAction.coffee:98-100` | `if target.getHP() <= 0` |
| T+0.0s | DieAction created | `app/sdk/entities/entity.coffee:587-591` | `target.actionDie(source)` |
| T+0.0s | DieAction executed | `app/sdk/gameSession.coffee:1550` | `executeAction(dieAction)` |
| T+0.0s | Validation phase | `app/sdk/gameSession.coffee:1550-1555` | Modifiers can invalidate death |
| T+0.0s | EternalHeart check | `app/sdk/modifiers/modifierEternalHeart.coffee:43-55` | May prevent death |
| T+0.0s | RemoveAction execute | `app/sdk/actions/removeAction.coffee:12-18` | `_execute()` calls removal |
| T+0.0s | Remove from board | `app/sdk/gameSession.coffee:3062-3084` | `removeCardFromBoard()` |
| T+0.0s | Board removes card | `app/sdk/gameSession.coffee:3066` | `board.removeCard(card)` |
| T+0.0s | onRemoveFromBoard | `app/sdk/entities/entity.coffee:92-97` | Entity callbacks fired |
| T+0.0s | General death check | `app/sdk/entities/entity.coffee:94-97` | If general: `p_requestGameOver()` |
| T+0.0s | Stats tracked | `app/sdk/gameSession.coffee:3076-3078` | `opponent.totalMinionsKilled++` |
| T+0.0s | View receives | `app/view/layers/game/GameLayer.js:3857-3868` | Detect DieAction |
| T+0.0s | Death state shown | `app/view/layers/game/GameLayer.js:3863` | `targetNode.showDeathState(action)` |
| T+0.0s | State locked | `app/view/nodes/cards/UnitNode.js:335` | `setStateLocked(true)` |
| T+0.0s | Death sound | `app/view/nodes/cards/UnitNode.js:343` | `playSoundDeath()` |
| T+Xms* | Death anim ends | `app/view/nodes/cards/UnitNode.js:357` | Animation sequence complete |
| T+Xms | Dissolve starts | `app/view/nodes/cards/UnitNode.js:363` | `addDissolveVisualTag()` |
| T+Xms | Particles emit | `app/view/nodes/cards/UnitNode.js:377` | Dissolve particle system |
| T+Xms+1.0s | Dissolve ends | `app/view/nodes/cards/UnitNode.js:386` | Tween 0.0→1.0 complete |
| T+Xms+1.0s | Node destroyed | `app/view/nodes/cards/UnitNode.js:389` | `this.destroy()` |
| T+Xms+1.0s | Cleanup phase | `app/sdk/modifiers/modifier.coffee:259-273` | `onAfterCleanupAction` |
| T+Xms+1.0s | Deathwatch fires | `app/sdk/modifiers/modifierDeathWatch.coffee:26-38` | `onDeathWatch(action)` |

*X depends on death animation duration from unit's AnimResource

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Detection | DamageAction | Check HP <= 0 | `app/sdk/actions/damageAction.coffee:98-100` |
| Creation | Entity | Create die action | `app/sdk/entities/entity.coffee:587-591` |
| Creation | ActionFactory | Instantiate DieAction | `app/sdk/actions/actionFactory.coffee` |
| Validation | ModifierEternalHeart | Prevent death | `app/sdk/modifiers/modifierEternalHeart.coffee:43-55` |
| Validation | ModifierDieSpawnNewGeneral | Transform instead | `app/sdk/modifiers/modifierDieSpawnNewGeneral.coffee:54-62` |
| Execution | RemoveAction | Remove from board | `app/sdk/actions/removeAction.coffee:12-18` |
| Execution | GameSession | Process removal | `app/sdk/gameSession.coffee:3062-3084` |
| Execution | Board | Remove card spatially | `app/sdk/board.coffee` (removeCard) |
| Callback | Entity | onRemoveFromBoard | `app/sdk/entities/entity.coffee:92-97` |
| GameOver | GameSession | Request game over | `app/sdk/gameSession.coffee` (p_requestGameOver) |
| Cleanup | Modifier | After cleanup hooks | `app/sdk/modifiers/modifier.coffee:259-273` |
| Deathwatch | ModifierDeathWatch | Trigger effects | `app/sdk/modifiers/modifierDeathWatch.coffee:26-38` |
| View | GameLayer | Show death animation | `app/view/layers/game/GameLayer.js:3857-3868` |
| Animation | UnitNode | Death state + dissolve | `app/view/nodes/cards/UnitNode.js:327-397` |
| Audio | EntityNode | Play death sound | `app/view/nodes/cards/EntityNode.js:836-840` |

---

## Data Transformations

**Input:** Living unit on board
```javascript
{
  index: 1042,
  position: { x: 4, y: 2 },
  isActive: true,
  hp: 1,
  damage: 9,  // About to receive lethal damage
  ownerId: "player2"
}
```

**Output:** Unit removed
```javascript
{
  index: 1042,
  position: null,  // No longer on board
  isActive: false,
  hp: 0,
  damage: 12,  // Final damage value
  ownerId: "player2"
}
// Card moved to "removed" zone
```

**Side Effects:**
- Unit removed from board spatial grid
- `opponent.totalMinionsKilled` incremented
- Deathwatch modifiers triggered for all units
- Death-related modifiers fire (e.g., "on death, summon...")
- If General: game over triggered
- EntityNode destroyed in view layer
- Particle effects spawned and cleaned up

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.FADE_FAST_DURATION` | 0.2s | `app/common/config.js:553` | Quick fade transitions |
| `CONFIG.FADE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:554` | Support nodes fade out |
| `CONFIG.FADE_SLOW_DURATION` | 1.0s | `app/common/config.js:555` | Dissolve effect duration |
| `CONFIG.ANIMATE_SLOW_DURATION` | 1.0s | `app/common/config.js:552` | Death animation timing |

**Death Animation Breakdown:**
- Death animation: Unit-specific (from AnimResource)
- Support nodes fade: 0.35s (`FADE_MEDIUM_DURATION`)
- Dissolve tween: 1.0s (`FADE_SLOW_DURATION`)
- Particle system: 0.1s burst
- Total: ~1.35s + animation length

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Death Triggered | State locked | `soundResource.death` | `UnitNode.js:343` |
| Death Animation | Unit death anim | - | `UnitNode.js:357` |
| Stats Display | Show final stats | - | `UnitNode.js:339-342` |
| Support Fade | Nodes fade out | - | `UnitNode.js:345` |
| Dissolve Start | Dissolve shader | - | `UnitNode.js:363` |
| Particles | `ptcl_unit_dissolve.plist` | - | `UnitNode.js:377` |
| Dissolve Tween | Opacity 0→1 | - | `UnitNode.js:386` |
| Node Destroy | Remove from scene | - | `UnitNode.js:389` |
| Chroma Flash | Red flash (Razer) | - | `GameLayer.js:3858-3860` |

**Dissolve Effect Parameters (EntitySprite.js:12-21):**
```javascript
dissolveFrequency: 15.0,      // Dissolve granularity
dissolveEdgeFalloff: 0.75,    // Edge smoothness
dissolveVignetteStrength: 0.0  // No vignette on entities
```

**Particle System:**
```javascript
particleSystem = BaseParticleSystem.create({
  plistFile: RSX.ptcl_unit_dissolve.plist,
  affectedByWind: true,
  duration: 0.1
});
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Death prevented | `modifierEternalHeart.coffee:43-55` | Invalidate DieAction | "Can't die!" message |
| Transform death | `modifierDieSpawnNewGeneral.coffee:54-62` | Spawn replacement | New unit appears |
| Already dead | `entity.getIsActive()` | Skip removal | N/A |
| Invalid target | `removeAction.coffee:12-18` | Check target exists | N/A |
| General dies | `entity.coffee:94-97` | Trigger game over | Victory/Defeat screen |

**Death Prevention Flow (EternalHeart):**
```coffeescript
# modifierEternalHeart.coffee:43-55
onValidateAction: (event) ->
  action = event.action
  if action instanceof DieAction and action.getTarget() is @getCard()
    # Store action index for healing after
    @_private.eternalHeartAtActionIndexActionIndex = action.getIndex()
    # Prevent death
    @invalidateAction(action, @getCard().getPosition(), "Can't die!")

# Later in onAfterCleanupAction: heal to full
```

---

## Death Types

| Type | Trigger | Class | Notes |
|------|---------|-------|-------|
| Damage Death | HP <= 0 from damage | `DieAction` | Standard death |
| Kill Effect | Destroy spell/ability | `KillAction` → `DieAction` | Bypasses damage |
| Sacrifice | Self-sacrifice ability | `KillAction` → `DieAction` | Self-triggered |
| Transform | Replaced by another unit | N/A | No death, replaced |
| Bounced | Returned to hand | `BounceAction` | Not a death |

**KillAction Flow (killAction.coffee:15-24):**
```coffeescript
_execute: () ->
  super()
  source = @getSource()
  target = @getTarget()
  if target
    dieAction = target.actionDie(source)
    @getGameSession().executeAction(dieAction)
```

---

## Deathwatch System

**Location:** `app/sdk/modifiers/modifierDeathWatch.coffee:26-38`

**Trigger Timing:** `onAfterCleanupAction` (after all game state changes)

```coffeescript
onAfterCleanupAction: (e) ->
  super(e)
  action = e.action
  if @getIsActionRelevant(action)
    @onDeathWatch(action)

getIsActionRelevant: (action) ->
  return action instanceof DieAction and
         action.getTarget()? and
         action.getTarget().getType() is CardType.Unit and
         action.getTarget() != @getCard()  # Not self
```

**Deathwatch Variants:**
- `ModifierDeathWatch` - Base, any unit death
- `ModifierDeathWatchSpawnEntity` - Spawn unit on death
- `ModifierDeathWatchDrawCard` - Draw on death
- `ModifierDeathWatchBuff` - Gain stats on death
- `ModifierDeathWatchDamageEnemy` - Deal damage on death

---

## General Death → Game Over

**Location:** `app/sdk/entities/entity.coffee:92-97`

```coffeescript
onRemoveFromBoard: (board, x, y, sourceAction) ->
  super(board, x, y, sourceAction)

  if @getIsGeneral()
    # Notify the game session this entity is a general and has died
    @getGameSession().p_requestGameOver()
```

**Game Over Flow:**
1. General's `onRemoveFromBoard` called
2. `p_requestGameOver()` invoked on GameSession
3. Winner determined (other player)
4. Game state set to ended
5. Victory/Defeat screens shown

---

## Action Inheritance Hierarchy

```
Action (base)
    ↑
RemoveAction
  - Removes card from board
  - Calls gameSession.removeCardFromBoard()
    ↑
DieAction
  - Extends RemoveAction
  - Triggers death-specific logic
  - Used by damage and kill effects
```
