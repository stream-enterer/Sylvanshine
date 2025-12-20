# Flow: Unit Spawn

## Trigger
Player plays a unit card from hand, or a spell/ability summons a unit to the board.

**Entry Points:**
- `PlayCardFromHandAction` for cards played from hand
- `ApplyCardToBoardAction` for programmatic spawns (spells, abilities)
- `PlayCardSilentlyAction` for AI/test spawns without animation

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Action created | `app/sdk/actions/playCardFromHandAction.coffee:15` | `PlayCardFromHandAction` instantiated with card index and position |
| T+0.0s | Validation | `app/sdk/validators/validatorPlayCard.coffee:14-61` | Check mana cost, valid position, card exists |
| T+0.0s | Execute begins | `app/sdk/actions/applyCardToBoardAction.coffee:99` | `_execute()` called |
| T+0.0s | Card data applied | `app/sdk/gameSession.coffee:2971` | `card.applyCardData(cardDataOrIndex)` |
| T+0.0s | Skin/prismatic inject | `app/sdk/gameSession.coffee:2974` | `_injectSkinAndPrismaticPropertiesIntoCard()` |
| T+0.0s | Index card | `app/sdk/gameSession.coffee:2978` | `_indexCardAsNeeded()` assigns unique index |
| T+0.0s | Validate position | `app/sdk/gameSession.coffee:2982-2985` | Check board position valid and unobstructed |
| T+0.0s | Flush cache | `app/sdk/gameSession.coffee:2992-2995` | Push `update_cache_action` event |
| T+0.0s | Push event | `app/sdk/gameSession.coffee:2998-3000` | Emit `apply_card_to_board` event |
| T+0.0s | Add to board | `app/sdk/gameSession.coffee:3026` | `board.addCard(card)` |
| T+0.0s | Trigger callbacks | `app/sdk/gameSession.coffee:3029` | `card.onApplyToBoard(board, x, y, sourceAction)` |
| T+0.0s | Death check | `app/sdk/gameSession.coffee:3032-3033` | If HP <= 0, execute `actionDie()` |
| T+0.0s | Stat tracking | `app/sdk/actions/applyCardToBoardAction.coffee:130` | Increment `totalMinionsSpawned` |
| T+0.0s | View receives action | `app/view/layers/game/GameLayer.js:3527` | Detect `ApplyCardToBoardAction` |
| T+0.0s | Node created | `app/view/layers/game/GameLayer.js:3540-3546` | Get/create `EntityNode` for unit |
| T+0.0s | Spawn triggered | `app/view/layers/game/GameLayer.js:3545` | `node.showSpawn()` called |
| T+0.0s | Sound plays | `app/view/nodes/cards/EntityNode.js:986` | Play apply sound effect |
| T+0.0s | Sprites shown | `app/view/nodes/cards/UnitNode.js:107-126` | `showSpawnSprites()` executes |
| T+0.0s-0.35s | Fade in | `app/view/nodes/cards/UnitNode.js:121` | Opacity 0→255 over `FADE_MEDIUM_DURATION` |
| T+0.35s | Spawn complete | `app/view/nodes/cards/UnitNode.js:125` | Shadow sprite visible, state ready |
| T+0.4s* | Prismatic glow | `app/view/nodes/cards/EntityNode.js:994-1010` | If prismatic: tint sequence (0.2s each) |

*Prismatic cards add additional 0.6s for glow effect sequence

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Input | ActionFactory | Create action from type | `app/sdk/actions/actionFactory.coffee:77-80` |
| Validation | ValidatorPlayCard | Validate card, mana, position | `app/sdk/validators/validatorPlayCard.coffee:14-61` |
| Validation | ModifierCustomSpawn | Allow spawn on obstructed tiles | `app/sdk/modifiers/modifierCustomSpawn.coffee` |
| Execution | GameSession | Apply card to board | `app/sdk/gameSession.coffee:2966-3053` |
| Execution | Board | Add card to spatial grid | `app/sdk/board.coffee` (addCard method) |
| Execution | Entity | Initialize on board | `app/sdk/entities/entity.coffee` (onApplyToBoard) |
| Modifiers | ModifierOnSummon | Trigger summon effects | `app/sdk/modifiers/modifierOnSummon.coffee` |
| View | GameLayer | Orchestrate spawn display | `app/view/layers/game/GameLayer.js:3527-3553` |
| Animation | EntityNode | Show spawn sequence | `app/view/nodes/cards/EntityNode.js:979-1015` |
| Animation | UnitNode | Execute spawn sprites | `app/view/nodes/cards/UnitNode.js:107-126` |
| Audio | AudioEngine | Play spawn sound | `app/audio/audio_engine.js` (play_effect) |

---

## Data Transformations

**Input:** Card in hand or card data
```javascript
{
  cardDataOrIndex: { id: 10301, ... } | 42,
  targetPosition: { x: 4, y: 2 },
  ownerId: "player1",
  cardInHand: true,
  isActive: false,
  position: null
}
```

**Output:** Unit on board
```javascript
{
  index: 1042,  // Unique card index
  position: { x: 4, y: 2 },
  ownerId: "player1",
  isActive: true,
  hp: entity.getMaxHP(),
  atk: entity.getATK(),
  exhausted: true,  // Summoning sickness (except rush)
  movesMade: 0,
  attacksMade: 0
}
```

**Side Effects:**
- Player mana reduced by card cost
- Card removed from hand (if played from hand)
- `totalMinionsSpawned` incremented on player stats
- Modifiers triggered: `onSummon`, `onApply` events
- Board cache invalidated and refreshed
- EntityNode created in view layer

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.FADE_FAST_DURATION` | 0.2s | `app/common/config.js:553` | Fast fade transitions |
| `CONFIG.FADE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:554` | Default spawn fade-in |
| `CONFIG.FADE_SLOW_DURATION` | 1.0s | `app/common/config.js:555` | Slow fade effects |
| `CONFIG.ANIMATE_FAST_DURATION` | 0.2s | `app/common/config.js:550` | Fast animations |
| `CONFIG.ANIMATE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:551` | Medium animations |
| `CONFIG.ANIMATE_SLOW_DURATION` | 1.0s | `app/common/config.js:552` | Slow animations |
| `CONFIG.ACTION_DELAY` | 0.5s | `app/common/config.js:497` | Delay between actions |

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Spawn Start | None (or apply anim) | `soundResource.apply` | `EntityNode.js:986` |
| Fade In | Opacity tween 0→255 | - | `UnitNode.js:121` |
| Shadow Appear | Shadow sprite fade | - | `UnitNode.js:124-125` |
| Prismatic Glow | Black tint → restore | - | `EntityNode.js:997-999` |
| Apply Animation | Unit-specific anim | - | `UnitNode.js:114-116` |

**Sound Resource Structure:**
```javascript
soundResource: {
  apply: "sfx_unit_deploy_1.audio",  // Spawn sound
  attack: "sfx_attack.audio",
  receiveDamage: "sfx_receive_damage.audio",
  death: "sfx_death.audio"
}
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Invalid position | `gameSession.coffee:2982-2985` | `isValidApplication = false` | Tile highlight red |
| Position obstructed | `board.getObstructionAtPositionForEntity()` | Reject unless `ModifierCustomSpawn` | "Can't spawn here" |
| Insufficient mana | `validatorPlayCard.coffee:22-24` | Action invalidated | "Not enough mana" |
| Card not in hand | `validatorPlayCard.coffee:32-35` | Action invalidated | "Invalid card" |
| Unit dies on spawn | `gameSession.coffee:3032-3033` | Execute `actionDie()` immediately | Death animation plays |
| Off board position | `board.isOnBoard(targetPosition)` | Reject spawn | N/A |

**Recovery Behavior:**
- Invalid spawns: Card not removed from hand, action rejected
- Death on spawn: Unit appears briefly then dies (triggers Deathwatch)
- Network desync: Action replay from authoritative server

---

## Modifier Hooks

Modifiers can interact with spawn at these points:

| Hook | Timing | Example Modifier |
|------|--------|------------------|
| `onValidateAction` | Before execution | Prevent specific spawns |
| `onApplyToBoard` | After board add | Apply buffs on summon |
| `onSummon` | After spawn complete | Opening Gambit effects |
| `onAfterApplyToBoard` | Post-spawn cleanup | Spawn additional units |
