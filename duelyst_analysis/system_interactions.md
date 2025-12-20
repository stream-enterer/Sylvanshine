# Duelyst System Interactions Documentation

This document traces the complete interaction flows between major game systems, including timing constants, FX/sound events, data transformations, and error handling.

---

## Table of Contents
1. [Action Execution Flow](#1-action-execution-flow)
2. [Attack Action Flow](#2-attack-action-flow)
3. [Play Card Flow](#3-play-card-flow)
4. [Turn Management Flow](#4-turn-management-flow)
5. [Modifier System Flow](#5-modifier-system-flow)
6. [Network Synchronization Flow](#6-network-synchronization-flow)
7. [Visual Feedback (FX) System](#7-visual-feedback-fx-system)

---

## 1. Action Execution Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Create | GameSession | Action instantiated with source/target | app/sdk/actions/action.coffee:30 |
| 2. Validate | Validators | Check action validity against game rules | app/sdk/validators/ |
| 3. Modify | Modifiers | `modify_action_for_execution` event fired | app/sdk/modifiers/modifier.coffee |
| 4. Execute | GameSession | `_execute()` called on action | app/sdk/gameSession.coffee |
| 5. Sub-actions | Action | Spawned actions added to queue | app/sdk/actions/action.coffee |
| 6. Cleanup | GameSession | `cleanup_action` event emitted | app/sdk/gameSession.coffee |
| 7. Visualize | GameLayer | `_showAction()` renders result | app/view/layers/game/GameLayer.js:3321 |

### Data Transformations
**Input:** Action object with `sourceIndex`, `targetIndex`, `ownerId`
```javascript
{
  type: "AttackAction",
  sourceIndex: 5,      // attacking unit
  targetIndex: 12,     // target unit
  ownerId: "player1",
  isValid: true
}
```
**Output:** Modified game state
```javascript
{
  target.hp: reduced by damage amount,
  source.attacksMade: incremented by 1,
  subActions: [DieAction] // if target HP <= 0
}
```
**Side effects:**
- `step` event emitted to GameLayer
- `action` event emitted for UI updates
- Modifier triggers evaluated (onBeforeAction, onAfterAction)

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.ACTION_DELAY` | 0.5s | Default delay between action animations |
| `CONFIG.ANIMATE_FAST_DURATION` | 0.2s | Fast animation duration |
| `CONFIG.ANIMATE_MEDIUM_DURATION` | 0.35s | Medium animation duration |
| `CONFIG.ANIMATE_SLOW_DURATION` | 1.0s | Slow animation duration |
| `CONFIG.PARTICLE_SEQUENCE_DELAY` | 0.5s | Delay for particle system sequencing |

### Error Cases
| Error | Handling |
|-------|----------|
| Invalid action (validator fails) | `invalid_action` event fired, action rejected |
| Target no longer exists | Action skipped, no sub-actions |
| Insufficient mana | Validator prevents execution |

---

## 2. Attack Action Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Initiate | Player/AI | AttackAction created | app/sdk/actions/attackAction.coffee:10 |
| 2. Get Damage | AttackAction | `getDamageAmount()` returns source.ATK | app/sdk/actions/attackAction.coffee:22 |
| 3. Modify Damage | Modifiers | `damageChange`, `damageMultiplier` applied | app/sdk/actions/damageAction.coffee:26 |
| 4. Apply Damage | DamageAction | `target.applyDamage(dmg)` | app/sdk/actions/damageAction.coffee:85 |
| 5. Check Death | DamageAction | If HP <= 0, spawn DieAction | app/sdk/actions/damageAction.coffee:98 |
| 6. Strikeback | GameSession | If allowed, FightAction spawns counter-attack | app/sdk/actions/attackAction.coffee:30 |
| 7. Increment Attacks | AttackAction | `source.attacksMade++` | app/sdk/actions/attackAction.coffee:43 |

### Data Transformations
**Input:**
```javascript
{
  attacker: { atk: 5, attacksMade: 0 },
  target: { hp: 8, maxHp: 8 }
}
```
**Output:**
```javascript
{
  attacker: { atk: 5, attacksMade: 1 },
  target: { hp: 3, maxHp: 8 } // received 5 damage
}
```
**Side effects:**
- `totalDamageDealt` updated on owner
- `totalDamageDealtToGeneral` if target is general
- Strikeback damage applied to attacker

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.ENTITY_ATTACK_DURATION_MODIFIER` | 1.0 | Attack animation speed multiplier |
| `CONFIG.HIGH_DAMAGE` | 7 | Threshold for screen shake effect |
| `CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_DURATION` | 0.35s | Duration of shake on high damage |
| `CONFIG.HIGH_DAMAGE_SCREEN_SHAKE_STRENGTH` | 20.0 | Intensity of screen shake |

### FX/Sound Events
| Event | FX | Sound | Trigger Point |
|-------|-------|-------|---------------|
| Attack start | Source attack animation | Unit attack SFX | `_showActionForSource()` |
| Attack impact | Target impact FX (fxCollisionSparks*) | Impact SFX | Target reaction delay |
| High damage | Screen shake + radial blur | - | `getTotalDamageAmount() >= 7` |
| Unit death | Blood/death particle FX | Death SFX | DieAction execution |

### Error Cases
| Error | Handling |
|-------|----------|
| Target is friendly | Validator rejects (unless specific card allows) |
| Target has Provoke | Validator enforces Provoke targeting |
| Source exhausted | Action validation fails |
| Strikeback blocked | `setIsStrikebackAllowed(false)` |

---

## 3. Play Card Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Select Card | UI/Player | Card selected from hand | app/view/layers/game/GameLayer.js |
| 2. Validate Targets | SpellTargets | Check valid target positions | app/sdk/spells/spell.coffee |
| 3. Pay Mana | PlayCardFromHandAction | Deduct mana cost | app/sdk/actions/playCardFromHandAction.coffee |
| 4. Remove from Hand | Action | Card removed from hand array | app/sdk/actions/removeCardFromHandAction.coffee |
| 5. Apply to Board | ApplyCardToBoardAction | Entity placed or spell cast | app/sdk/actions/applyCardToBoardAction.coffee |
| 6. Opening Gambit | Modifiers | `onOpeningGambit` triggers fire | app/sdk/modifiers/modifierOpeningGambit*.coffee |
| 7. Followup | GameSession | Followup action sequence begins | app/sdk/actions/playCardAction.coffee |

### Data Transformations
**Input:**
```javascript
{
  player: { mana: 5, hand: [cardA, cardB, cardC] },
  card: { manaCost: 3, type: "Unit" },
  targetPosition: { x: 4, y: 2 }
}
```
**Output:**
```javascript
{
  player: { mana: 2, hand: [cardA, cardC] },
  board: { /* new entity at (4,2) */ },
  newEntity: { exhausted: true } // summoning sickness unless Rush
}
```
**Side effects:**
- Entity added to board
- SummonWatch modifiers triggered
- Opening Gambit effects executed
- Exhaustion applied (unless Rush)

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.MY_PLAYED_CARD_TRANSITION_DURATION` | 0.5s | Card reveal transition |
| `CONFIG.MY_PLAYED_CARD_SHOW_DURATION` | 1.5s | Duration card is shown |
| `CONFIG.OPPONENT_PLAYED_CARD_TRANSITION_DURATION` | 1.0s | Opponent card reveal |
| `CONFIG.OPPONENT_PLAYED_CARD_SHOW_DURATION` | 2.0s | Opponent card display time |
| `CONFIG.OPPONENT_PLAYED_CARD_DELAY` | 3.0s | Total delay for opponent cards |
| `CONFIG.GENERAL_CAST_START_DELAY` | 0.25s | General casting animation start |
| `CONFIG.GENERAL_CAST_END_DELAY` | 0.5s | General casting animation end |

### FX/Sound Events
| Event | FX | Sound | Trigger Point |
|-------|-------|-------|---------------|
| Card played | Card reveal animation | Card play SFX | `_showActionForApplyCard()` |
| Spell cast | Spell-specific FX (fx_f1_*, fx_f2_*, etc.) | Spell SFX | After card reveal |
| Unit summon | Summon particle effect | Summon SFX | `ApplyCardToBoardAction` |
| Legendary summon | `fxSummonLegendary` (0.06s frame delay) | `sfx_summonlegendary` | Rarity check |

### Error Cases
| Error | Handling |
|-------|----------|
| Insufficient mana | Card returns to hand, error message |
| Invalid target | Tile highlights removed, no action |
| Hand overflow (>6 cards) | Card burned, `BurnCardAction` |
| Board position occupied | Validator rejects placement |

---

## 4. Turn Management Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. End Turn | EndTurnAction | Current player's turn ends | app/sdk/actions/endTurnAction.coffee |
| 2. Turn Cleanup | Modifiers | `onEndTurn` triggers fire | app/sdk/modifiers/modifier.coffee |
| 3. Switch Player | GameSession | `currentPlayerId` changes | app/sdk/gameSession.coffee |
| 4. Start Turn | StartTurnAction | New turn begins | app/sdk/actions/startTurnAction.coffee |
| 5. Mana Refresh | Player | Mana = min(turnCount+1, 9) | app/sdk/player.coffee |
| 6. Draw Card | DrawCardAction | Player draws 1 card | app/sdk/actions/drawCardAction.coffee |
| 7. Turn Triggers | Modifiers | `onStartTurn` triggers fire | app/sdk/modifiers/modifier.coffee |
| 8. Exhaustion Reset | Units | Move/attack counts reset | app/sdk/actions/refreshExhaustionAction.coffee |

### Data Transformations
**Input:**
```javascript
{
  currentPlayerId: "player1",
  turnCount: 3,
  player2: { mana: 0, maxMana: 4 }
}
```
**Output:**
```javascript
{
  currentPlayerId: "player2",
  turnCount: 4,
  player2: { mana: 5, maxMana: 5 }, // mana refreshed to turn+1
  // +1 card drawn to hand
}
```
**Side effects:**
- All player2 units: `exhausted: false`, `movesMade: 0`, `attacksMade: 0`
- Modifier duration counters decremented
- Signature card cooldown decremented

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.TURN_DELAY` | 0.0s | Delay after turn actions shown |
| `CONFIG.TURN_DURATION` | 90.0s | Maximum turn time |
| `CONFIG.TURN_DURATION_INACTIVE` | 15.0s | Turn time when inactive |
| `CONFIG.TURN_DURATION_LATENCY_BUFFER` | 2.0s | Network latency buffer |
| `CONFIG.TURN_TIME_SHOW` | 20.0s | When to show countdown |
| `CONFIG.CARD_DRAW_PER_TURN` | 1 | Cards drawn at turn start |

### FX/Sound Events
| Event | FX | Sound | Trigger Point |
|-------|-------|-------|---------------|
| Turn start | Turn banner animation | Turn start chime | `show_start_turn` event |
| Turn end | - | Turn end SFX | `show_end_turn` event |
| Card draw | `fx_carddraw` (0.08s frame) | Draw SFX | `DrawCardAction` |
| Deck empty | Hurting damage FX | - | `HurtingDamageAction` |

### Error Cases
| Error | Handling |
|-------|----------|
| Turn timeout | Auto-end turn triggered |
| Empty deck draw | 2 damage to general (`HurtingDamageAction`) |
| Disconnect | Reconnect attempt or forfeit |

---

## 5. Modifier System Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Create | ModifierFactory | Modifier instantiated from context | app/sdk/modifierFactory.coffee |
| 2. Apply | ApplyModifierAction | Modifier attached to card | app/sdk/actions/applyModifierAction.coffee |
| 3. Activate | Modifier | `onActivated()` called | app/sdk/modifiers/modifier.coffee |
| 4. Subscribe | EventBus | Modifier registers for events | app/sdk/modifiers/modifier.coffee |
| 5. Intercept | Modifier | `onBeforeAction/onAfterAction` hooks | app/sdk/modifiers/modifier.coffee |
| 6. Deactivate | Modifier | `onDeactivated()` on removal/silence | app/sdk/modifiers/modifier.coffee |
| 7. Remove | RemoveModifierAction | Modifier detached from card | app/sdk/actions/removeModifierAction.coffee |

### Data Transformations
**Input (Buff Modifier):**
```javascript
{
  target: { atk: 3, hp: 5 },
  modifier: { type: "ModifierBuffAttack", attackBuff: 2 }
}
```
**Output:**
```javascript
{
  target: { atk: 5, hp: 5 }, // +2 ATK from modifier
  target.modifiers: [/* modifier reference */]
}
```
**Side effects:**
- Card stat getters now include modifier bonuses
- Visual indicator added to entity
- Modifier tracked in game state for serialization

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.ENTITY_STATS_CHANGE_DELAY` | 0.75s | Delay between stat callouts |
| `CONFIG.CARD_MODIFIER_ACTIVE_OPACITY` | 255.0 | Active modifier icon opacity |
| `CONFIG.CARD_MODIFIER_INACTIVE_OPACITY` | 100.0 | Inactive modifier opacity |
| `CONFIG.CARD_MODIFIER_PADDING_HORIZONTAL` | 8.0 | Horizontal spacing for icons |
| `CONFIG.CARD_MODIFIER_PADDING_VERTICAL` | 5.0 | Vertical spacing for icons |

### FX/Sound Events
| Event | FX | Sound | Trigger Point |
|-------|-------|-------|---------------|
| Buff applied | `fxBuffSimpleGold` (0.08s) | Buff SFX | `ApplyModifierAction` |
| Forcefield | `ForceField` (0.1s frame) | Shield SFX | On activation |
| Dispel | Cleanse ripple effect | Dispel SFX | `RemoveModifierAction` |
| Provoke | `FloatingShield` (0.2s) | - | On activation |

### Event Hooks Reference
| Hook | When Called | Common Uses |
|------|-------------|-------------|
| `onValidateAction` | Before action execution | Provoke, attack restrictions |
| `onBeforeAction` | Just before execute | Damage modification |
| `onAfterAction` | After execute | DeathWatch, SummonWatch |
| `onStartTurn` | Player's turn starts | Grow, regeneration |
| `onEndTurn` | Player's turn ends | Duration countdown |
| `onApplyToCard` | Modifier applied | Stat grants |
| `onRemoveFromCard` | Modifier removed | Stat removal |

### Error Cases
| Error | Handling |
|-------|----------|
| Max modifiers reached | Oldest non-keyword removed |
| Invalid modifier target | Action cancelled |
| Circular trigger loop | Depth limit prevents infinite loop |

---

## 6. Network Synchronization Flow

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Player Input | Client | Action generated locally | app/view/layers/game/GameLayer.js |
| 2. Send | NetworkManager | Action serialized and sent | app/sdk/networkManager.coffee |
| 3. Validate | Server | Authoritative validation | Server-side gameSession |
| 4. Broadcast | Server | Action sent to all clients | Socket.io emission |
| 5. Receive | NetworkManager | `network_game_event` received | app/sdk/networkManager.coffee |
| 6. Execute | GameSession | Action executed locally | app/sdk/gameSession.coffee |
| 7. Visualize | GameLayer | Action displayed | app/view/layers/game/GameLayer.js |

### Data Transformations
**Input (Client):**
```javascript
{
  action: AttackAction,
  sourceIndex: 5,
  targetIndex: 12,
  timestamp: 1703001234567
}
```
**Output (Broadcast):**
```javascript
{
  type: "game_action",
  action: { /* serialized action */ },
  stepIndex: 42,
  validated: true
}
```

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.RECONNECT_DELAY` | 1.0s | Delay before reconnect attempt |
| `CONFIG.MINUTES_ALLOWED_TO_CONTINUE_GAME` | 45 | Max minutes to reconnect |
| `CONFIG.REPLAY_MAX_STEP_DELAY` | 5.0s | Max step delay in replays |
| `CONFIG.REPLAY_MAX_STEP_DELAY_STARTING_HAND` | 10.0s | Mulligan step delay |

### Events
| Event | Emitter | Listeners |
|-------|---------|-----------|
| `network_game_event` | networkManager | application, performance |
| `network_game_error` | networkManager | application |
| `reconnect_to_game` | networkManager | application |
| `opponent_connection_status_changed` | networkManager | application, UI |
| `game_server_shutdown` | networkManager | application |

### Error Cases
| Error | Handling |
|-------|----------|
| Network timeout | Show reconnect dialog |
| Server validation fail | Rollback to snapshot |
| Desync detected | `rollback_to_snapshot` event |
| Connection lost | Reconnect flow initiated |

---

## 7. Visual Feedback (FX) System

### System Interactions
| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| 1. Action Event | GameSession | `step` event emitted | app/sdk/gameSession.coffee |
| 2. Show Action | GameLayer | `_showAction()` called | app/view/layers/game/GameLayer.js:3316 |
| 3. Get FX Data | FXFactory | `_getActionFXData()` retrieves resources | app/view/fx/ |
| 4. Create Sprites | NodeFactory | FX nodes instantiated | app/view/nodes/fx/ |
| 5. Position | GameLayer | FX positioned at source/target | app/view/layers/game/GameLayer.js |
| 6. Animate | Cocos2d | Animation sequences played | app/view/actions/ |
| 7. Cleanup | FX | Auto-remove after completion | Cocos2d action sequence |

### FX Categories and Frame Delays
| Category | Example FX | Frame Delay | Usage |
|----------|-----------|-------------|-------|
| Buff effects | `fxBuffSimpleGold` | 0.08s | Stat increases |
| Buff effects | `ForceField` | 0.10s | Shield abilities |
| Buff effects | `FloatingShield` | 0.20s | Provoke indicator |
| Damage effects | `fxBloodGround` | 0.08s | Blood splatter |
| Damage effects | `fxDamageDecal` | 0.08s | Ground damage |
| Faction1 (Lyonar) | `fx_f1_divinebond` | 0.08s | Divine Bond spell |
| Faction2 (Songhai) | `fx_f2_phoenixfire` | 0.08s | Phoenix Fire spell |
| Faction3 (Vetruvian) | `fx_f3_blindscorch` | 0.08s | Blind Scorch |
| Faction4 (Abyssian) | `fx_f4_shadownova` | 0.08s | Shadow Nova |
| Faction5 (Magmar) | `fx_f5_naturalselection` | 0.08s | Natural Selection |
| Faction6 (Vanar) | `fx_f6_chromaticcold` | 0.08s | Chromatic Cold |
| Impact effects | `fxCollisionSparksBlue` | 0.08s | Attack impact |
| Spawn effects | `fxSummonLegendary` | 0.06s | Legendary summon |
| Teleport effects | `fxTeleportRecall` | 0.08s | Teleport visuals |
| Tile effects | `fxShadowCreepIdle` | 1.0s | Shadow Creep |

### Timing Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.FADE_FAST_DURATION` | 0.2s | Quick fade in/out |
| `CONFIG.FADE_MEDIUM_DURATION` | 0.35s | Standard fade |
| `CONFIG.FADE_SLOW_DURATION` | 1.0s | Slow fade |
| `CONFIG.PLAYED_SPELL_FX_FADE_IN_DURATION` | 0.5s | Spell FX fade in |
| `CONFIG.PLAYED_SPELL_FX_FADE_OUT_DURATION` | 0.5s | Spell FX fade out |
| `CONFIG.MAX_FX_PER_EVENT` | 5 | Max simultaneous FX |
| `CONFIG.ENTITY_MOVE_DURATION_MODIFIER` | 1.0 | Move animation speed |
| `CONFIG.ENTITY_MOVE_FLYING_FIXED_TILE_COUNT` | 3.0 | Flying unit speed calc |

### Sound Effect Priorities
| Priority | Value | Type |
|----------|-------|------|
| `CONFIG.DEFAULT_SFX_PRIORITY` | 0 | Default sounds |
| `CONFIG.CLICK_SFX_PRIORITY` | 1 | Click feedback |
| `CONFIG.SELECT_SFX_PRIORITY` | 2 | Selection sounds |
| `CONFIG.CANCEL_SFX_PRIORITY` | 3 | Cancel actions |
| `CONFIG.CONFIRM_SFX_PRIORITY` | 4 | Confirmations |
| `CONFIG.ERROR_SFX_PRIORITY` | 7 | Error alerts |
| `CONFIG.MAX_SFX_PRIORITY` | 9999 | Critical sounds |

### Audio Constants
| Constant | Value | Purpose |
|----------|-------|---------|
| `CONFIG.DEFAULT_MASTER_VOLUME` | 1.0 | Master volume |
| `CONFIG.DEFAULT_MUSIC_VOLUME` | 0.04 | Background music |
| `CONFIG.DEFAULT_VOICE_VOLUME` | 0.3 | Voice lines |
| `CONFIG.DEFAULT_SFX_VOLUME` | 0.3 | Sound effects |
| `CONFIG.MUSIC_CROSSFADE_DURATION` | 0.5s | Music transition |
| `CONFIG.SFX_INTERACTION_DELAY` | 0.06s | Interaction SFX delay |
| `CONFIG.SFX_MULTIPLIER_DURATION_THRESHOLD` | 0.35s | Volume adjustment window |
| `CONFIG.INTERACTION_SFX_BLOCKING_DURATION_THRESHOLD` | 0.5s | SFX blocking duration |

### Screen Effects on High Impact
| Trigger | Effect | Value |
|---------|--------|-------|
| Damage >= 7 | Screen shake | 0.35s, strength 20.0 |
| Damage >= 7 | Radial blur | spread 0.25, strength 0.5 |
| High cost card (>=5) | Screen shake | 1.0s, strength 5.0 |
| High cost card (>=5) | Screen focus | fade in 0.1s, out 0.5s |

### Error Cases
| Error | Handling |
|-------|----------|
| Missing FX resource | Fallback to default FX |
| Audio context blocked | Retry on user interaction |
| Too many FX | Limited to `MAX_FX_PER_EVENT` |
| WebGL context lost | Show reload prompt |

---

## Game State Machine

### State Transitions
```
Loading → Main Menu → Matchmaking → Game Setup → Starting Hand
                                          ↓
                              ← Game Loop (turns) →
                                          ↓
                                    Game Over → Results
```

### Key Game Events
| Event | Description | Handlers |
|-------|-------------|----------|
| `step` | Game step completed | GameLayer |
| `start_turn` | Player turn begins | UI, tutorial |
| `end_turn` | Player turn ends | UI, bot bar |
| `action` | Action executed | game layout |
| `game_over` | Game concluded | application, UI |
| `rollback_to_snapshot` | State rollback | GameLayer |

---

## Board Configuration

| Constant | Value | Description |
|----------|-------|-------------|
| `CONFIG.BOARDROW` | 5 | Board height |
| `CONFIG.BOARDCOL` | 9 | Board width |
| `CONFIG.BOARDCENTER` | {x:4, y:2} | Board center position |
| `CONFIG.TILESIZE` | 95 | Tile size in pixels |
| `CONFIG.MAX_MANA` | 9 | Maximum mana cap |
| `CONFIG.STARTING_MANA` | 2 | Initial mana |
| `CONFIG.MAX_HAND_SIZE` | 6 | Maximum cards in hand |
| `CONFIG.STARTING_HAND_SIZE` | 5 | Cards at game start |
| `CONFIG.STARTING_HAND_REPLACE_COUNT` | 2 | Mulligan limit |
| `CONFIG.MAX_ARTIFACTS` | 3 | Max equipped artifacts |
| `CONFIG.MAX_ARTIFACT_DURABILITY` | 3 | Artifact charge count |
| `CONFIG.MAX_DECK_SIZE` | 40 | Maximum deck cards |
| `CONFIG.MAX_DECK_DUPLICATES` | 3 | Max copies per card |

---

## References
- `app/common/config.js` - All timing and configuration constants
- `app/sdk/gameSession.coffee` - Central game state management
- `app/sdk/actions/*.coffee` - Action implementations
- `app/sdk/modifiers/*.coffee` - Modifier system
- `app/view/layers/game/GameLayer.js` - Visual rendering
- `duelyst_analysis/instances/*.tsv` - Entity instances data
- `duelyst_analysis/semantic/events.tsv` - Event mappings
