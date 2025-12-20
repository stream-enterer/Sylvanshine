# Flow: Unit Attack

## Trigger
Player orders a unit to attack an enemy unit or General.

**Entry Points:**
- `AttackAction` for standard attacks
- `ForcedAttackAction` for spell/ability-triggered attacks
- `DamageAsAttackAction` for attack-like damage without counting toward attack limit
- `FightAction` for mutual combat (both units attack each other)

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Action created | `app/sdk/entities/entity.coffee:600-610` | `entity.actionAttack(target)` creates AttackAction |
| T+0.0s | Validation start | `app/sdk/gameSession.coffee:1550-1555` | `validateAction(action)` called |
| T+0.0s | Provoke check | `app/sdk/modifiers/modifierProvoked.coffee:29-48` | Validate provoked units attack provoker |
| T+0.0s | Immunity check | `app/sdk/modifiers/modifierImmuneToAttacks.coffee:16-23` | Check target isn't immune |
| T+0.0s | General check | `app/sdk/modifiers/modifierCannotAttackGeneral.coffee:16-21` | Check if source can attack generals |
| T+0.0s | Execute begins | `app/sdk/actions/attackAction.coffee:36-43` | `_execute()` called |
| T+0.0s | Damage calculated | `app/sdk/actions/damageAction.coffee:26-33` | `getTotalDamageAmount()` computes damage |
| T+0.0s | Parent execute | `app/sdk/actions/damageAction.coffee:74-100` | `DamageAction._execute()` applies damage |
| T+0.0s | Damage applied | `app/sdk/entities/entity.coffee` | `target.applyDamage(dmg)` reduces HP |
| T+0.0s | Stats tracked | `app/sdk/actions/damageAction.coffee:96-97` | `source.getOwner().totalDamageDealt += dmg` |
| T+0.0s | Attacks incremented | `app/sdk/actions/attackAction.coffee:41-42` | `attacker.setAttacksMade(+1)` |
| T+0.0s | View receives | `app/view/layers/game/GameLayer.js:3826` | Source node detected |
| T+0.0s | Attack state shown | `app/view/nodes/cards/UnitNode.js:215-257` | `sourceNode.showAttackState(action)` |
| T+Xms* | Release sound | `app/view/nodes/cards/UnitNode.js:252` | `playSoundAttackerRelease()` at releaseDelay |
| T+Xms | Attack delay | `app/view/layers/game/GameLayer.js:3388-3392` | Wait for `attackDelay * ENTITY_ATTACK_DURATION_MODIFIER` |
| T+Xms | Target hit | `app/view/layers/game/GameLayer.js:3856` | `targetNode.showAttackedState(action)` |
| T+Xms | Damage sounds | `app/view/nodes/cards/UnitNode.js:311-312` | `playSoundAttackerDamage()`, `playSoundReceiveDamage()` |
| T+Xms | Strikeback | `app/sdk/modifiers/modifierStrikeback.coffee:41-45` | Counter-attack if in range |
| T+Xms | Death check | `app/sdk/actions/damageAction.coffee:98-100` | If HP <= 0, execute `actionDie()` |
| T+Xms+0.35s | High damage FX | `app/view/layers/game/GameLayer.js` | Screen shake if damage >= 7 |

*X depends on attack animation duration from unit's AnimResource

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Creation | Entity | Create attack action | `app/sdk/entities/entity.coffee:600-610` |
| Creation | ActionFactory | Instantiate AttackAction | `app/sdk/actions/actionFactory.coffee:77-80` |
| Validation | ModifierProvoked | Enforce provoke targeting | `app/sdk/modifiers/modifierProvoked.coffee:29-48` |
| Validation | ModifierImmuneToAttacks | Block attacks on immune | `app/sdk/modifiers/modifierImmuneToAttacks.coffee:16-23` |
| Validation | ModifierCannotAttackGeneral | Restrict general attacks | `app/sdk/modifiers/modifierCannotAttackGeneral.coffee:16-21` |
| Execution | AttackAction | Set damage from ATK | `app/sdk/actions/attackAction.coffee:22-25` |
| Execution | DamageAction | Calculate total damage | `app/sdk/actions/damageAction.coffee:26-33` |
| Execution | DamageAction | Apply damage to target | `app/sdk/actions/damageAction.coffee:74-100` |
| Reaction | ModifierStrikeback | Counter-attack | `app/sdk/modifiers/modifierStrikeback.coffee:37-64` |
| View | GameLayer | Coordinate attack display | `app/view/layers/game/GameLayer.js:3826-3856` |
| Animation | UnitNode | Show attack state | `app/view/nodes/cards/UnitNode.js:215-257` |
| Animation | UnitNode | Show attacked state | `app/view/nodes/cards/UnitNode.js:297-325` |
| Audio | EntityNode | Play attack sounds | `app/view/nodes/cards/EntityNode.js:815-832` |

---

## Data Transformations

**Input:** Attacker and target states
```javascript
attacker: {
  atk: 5,
  attacksMade: 0,
  position: { x: 3, y: 2 },
  hasAttacksLeft: true
}
target: {
  hp: 8,
  maxHP: 10,
  damage: 2,  // Already taken 2 damage
  position: { x: 4, y: 2 }
}
```

**Output:** After attack resolves
```javascript
attacker: {
  atk: 5,
  attacksMade: 1,  // Incremented
  position: { x: 3, y: 2 },
  hasAttacksLeft: false  // Unless multi-attack
}
target: {
  hp: 3,  // 8 - 5 = 3
  maxHP: 10,
  damage: 7,  // 2 + 5 = 7
  position: { x: 4, y: 2 }
}
// If strikeback occurred:
attacker: {
  hp: attacker.hp - target.atk  // Counter damage
}
```

**Side Effects:**
- `attacker.attacksMade` incremented (unless implicit/forced)
- `source.getOwner().totalDamageDealt` updated
- Target HP reduced
- Strikeback may damage attacker
- Death actions triggered if HP <= 0
- Deathwatch modifiers notified

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.ENTITY_ATTACK_DURATION_MODIFIER` | 1.0 | `app/common/config.js:746` | Scale attack animation speed |
| `CONFIG.ENTITY_ATTACK_MODIFIER_MAX` | 1.0 | `app/common/config.js:748` | Max attack speed multiplier |
| `CONFIG.ENTITY_ATTACK_MODIFIER_MIN` | 0.75 | `app/common/config.js:750` | Min attack speed multiplier |
| `CONFIG.HIGH_DAMAGE` | 7 | `app/common/config.js:894` | Threshold for high damage FX |
| `CONFIG.HIGH_DAMAGE_SCREEN_FOCUS_IN_DURATION` | 0.1s | `app/common/config.js:896` | High damage focus fade-in |
| `CONFIG.HIGH_DAMAGE_SCREEN_FOCUS_OUT_DURATION` | 0.25s | `app/common/config.js:900` | High damage focus fade-out |
| `CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_DURATION` | 0.35s | `app/common/config.js:902` | Screen shake duration |
| `CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_STRENGTH` | 20.0 | `app/common/config.js:903` | Screen shake intensity |
| `CONFIG.ANIMATE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:551` | Default animation duration |

**Per-Unit Timing (from AnimResource):**
- `attackDelay`: Time before damage applies (varies per unit)
- `attackReleaseDelay`: Time before attack sound plays

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Attack Wind-up | Attack animation plays | - | `UnitNode.js:240-254` |
| Attack Release | - | `soundResource.attack` | `EntityNode.js:821-825` at releaseDelay |
| Damage Applied | - | `soundResource.attackDamage` | `EntityNode.js:815-819` |
| Damage Received | Damage animation | `soundResource.receiveDamage` | `UnitNode.js:311-312` |
| High Damage | Screen shake + blur | - | `GameLayer.js` if dmg >= 7 |
| Strikeback | Counter attack anim | Attack sounds | `modifierStrikeback.coffee:41-45` |
| Death | Death sequence | `soundResource.death` | If target HP <= 0 |

**Sound Resource Structure:**
```javascript
soundResource: {
  attack: "sfx_attack_swing.audio",      // Wind-up release
  attackDamage: "sfx_impact.audio",      // Hit impact
  receiveDamage: "sfx_hurt.audio",       // Victim reaction
  death: "sfx_death.audio"               // On kill
}
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| No attacks left | `entity.getCanAttack()` | Action invalidated | "Unit cannot attack" |
| Target out of range | `entity.getAttackRange()` | Action invalidated | "Target out of range" |
| Provoked elsewhere | `modifierProvoked.coffee:29-48` | Action invalidated | "Must attack provoker" |
| Target immune | `modifierImmuneToAttacks.coffee:16-23` | Action invalidated | "Target is immune" |
| Cannot attack general | `modifierCannotAttackGeneral.coffee:16-21` | Action invalidated | "Cannot attack General" |
| No target at position | `entity.coffee:606-609` | Logs error, returns null | Console error |
| Source exhausted | `entity.getIsExhausted()` | Action invalidated | "Unit is exhausted" |

**Validation Flow:**
```coffeescript
# modifierProvoked.coffee:29-48
onValidateAction:(actionEvent) ->
  a = actionEvent.action
  if @getCard()? and @getCard() is a.getSource() and a instanceof AttackAction
    if !a.getTarget().getIsProvoker()
      @invalidateAction(a, @getCard().getPosition(),
        i18next.t("modifiers.provoked_attack_error"))
```

---

## Damage Calculation Pipeline

```
1. Base Damage
   └─ attacker.getATK() → base damage value

2. Damage Modifiers (damageAction.coffee:26-33)
   ├─ getDamageChange() → flat bonus/penalty
   ├─ getDamageMultiplier() → percentage modifier
   └─ getFinalDamageChange() → post-multiply adjustment

3. Total Damage Formula:
   totalDamage = floor(max((base + change) * multiplier + finalChange, 0))

4. Application (damageAction.coffee:86-94)
   └─ target.applyDamage(totalDamage)
```

---

## Strikeback (Counter-Attack) System

**Location:** `app/sdk/modifiers/modifierStrikeback.coffee:37-64`

**Conditions for Strikeback:**
1. Target has ATK > 0
2. Target can reach attacker (melee range or ranged)
3. `action.getIsStrikebackAllowed() == true`
4. Not same team

**Strikeback Timing:** Executes in `onBeforeAction` hook
```coffeescript
onBeforeAction: (actionEvent) ->
  a = actionEvent.action
  if @getIsActionRelevant(a)
    attackAction = @getCard().actionAttack(a.getSource())
    @getCard().getGameSession().executeAction(attackAction)
```

**Reach Check:**
```coffeescript
getCanReachEntity: (entity) ->
  reach = @getCard().getReach()
  if reach == 1
    # Check adjacent tiles
    for nearbyEntity in @getCard().getGameSession().getBoard().getEntitiesAroundEntity(@getCard())
      if nearbyEntity == entity
        return true
  else if reach > 1
    # Ranged units can always reach
    return true
  return false
```
