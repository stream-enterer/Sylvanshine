# Flow: Turn Sequence

## Trigger
Player ends their turn via EndTurnAction, triggering turn change sequence.

**Entry Points:**
- `EndTurnAction` - Player explicitly ends turn
- `TakeAnotherTurnAction` - Card effect grants extra turn
- Turn timer expiration (automatic end turn)

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | End turn action | `app/sdk/actions/endTurnAction.coffee:16-17` | `@getGameSession().p_endTurn()` |
| T+0.0s | Mark turn ended | `app/sdk/gameSession.coffee:1318` | `@getCurrentTurn().setEnded(true)` |
| T+0.0s | Tag turn owner | `app/sdk/gameSession.coffee:1321` | `@getCurrentTurn().setPlayerId()` |
| T+0.0s | Push to turn stack | `app/sdk/gameSession.coffee:1324` | `@turnStack.push(@currentTurn)` |
| T+0.0s | Fire end_turn event | `app/sdk/gameSession.coffee:1326` | `EVENTS.end_turn` dispatched |
| T+0.0s | Modifiers onEndTurn | `app/sdk/modifiers/modifier.coffee:2215-2219` | `_onEndTurn()` triggers |
| T+0.0s | Create new turn | `app/sdk/gameSession.coffee:2107` | `new GameTurn(@getGameSession())` |
| T+0.0s | Create StartTurnAction | `app/sdk/gameSession.coffee:2113-2118` | Queue start turn action |
| T+0.0s | Start turn execute | `app/sdk/actions/startTurnAction.coffee:16-17` | `@getGameSession().p_startTurn()` |
| T+0.0s | Swap current player | `app/sdk/gameSession.coffee:1337-1348` | Unless bonus turn active |
| T+0.0s | Refresh exhaustion | `app/sdk/gameSession.coffee:1354-1355` | All entities can act again |
| T+0.0s | Increment max mana | `app/sdk/gameSession.coffee:1358-1368` | Up to CONFIG.MAX_MANA (9) |
| T+0.0s | Restore mana | `app/sdk/gameSession.coffee:1368` | `setMana(getMaximumMana())` |
| T+0.0s | Fire start_turn event | `app/sdk/gameSession.coffee:1373` | `EVENTS.start_turn` dispatched |
| T+0.0s | Modifiers onStartTurn | `app/sdk/modifiers/modifier.coffee:2209-2213` | `_onStartTurn()` triggers |
| T+0.0s | Draw card phase | `app/sdk/gameSession.coffee:1834-1841` | Draw CONFIG.CARD_DRAW_PER_TURN |
| T+0.0s | Duration changes | `app/sdk/gameSession.coffee:1845-1868` | Update modifier durations |
| T+0.0s | Activate signature | `app/sdk/gameSession.coffee:1854-1860` | Signature card becomes active |
| T+0.5s | UI end turn show | `app/view/layers/game/GameLayer.js:1713` | `EVENTS.show_end_turn` |
| T+0.8s | After end turn UI | `app/view/layers/game/GameLayer.js:1734` | `EVENTS.after_show_end_turn` |
| T+1.0s | UI start turn show | `app/view/layers/game/GameLayer.js:1776-1792` | Display turn notification |
| T+1.0s | Turn sound effect | `app/view/layers/game/GameLayer.js:1780` | `sfx_ui_yourturn.audio` |
| T+1.5s | Notification fade | `app/view/layers/game/GameLayer.js:1821-1822` | Move and fade out |
| T+1.8s | After start turn UI | `app/view/layers/game/GameLayer.js:1830` | `EVENTS.after_show_start_turn` |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| End Turn | EndTurnAction | Execute end turn | `app/sdk/actions/endTurnAction.coffee:16-17` |
| End Turn | GameSession | Mark turn ended, push stack | `app/sdk/gameSession.coffee:1312-1326` |
| End Turn | Modifiers | Fire onEndTurn callbacks | `app/sdk/modifiers/modifier.coffee:2215-2219` |
| Transition | GameSession | Create new GameTurn | `app/sdk/gameSession.coffee:2104-2119` |
| Start Turn | StartTurnAction | Execute start turn | `app/sdk/actions/startTurnAction.coffee:16-17` |
| Start Turn | GameSession | Swap player, refresh mana | `app/sdk/gameSession.coffee:1332-1373` |
| Start Turn | Modifiers | Fire onStartTurn callbacks | `app/sdk/modifiers/modifier.coffee:2209-2213` |
| Draw Phase | Deck | Generate draw actions | `app/sdk/cards/deck.coffee:196-230` |
| Draw Phase | DrawCardAction | Execute card draw | `app/sdk/actions/drawCardAction.coffee:16-42` |
| Duration | Modifiers | Update turn durations | `app/sdk/modifiers/modifier.coffee:2111-2155` |
| Signature | GameSession | Activate signature card | `app/sdk/gameSession.coffee:1854-1860` |
| View | GameLayer | Show turn notifications | `app/view/layers/game/GameLayer.js:1713-1830` |
| Audio | AudioEngine | Play turn change sound | `app/view/layers/game/GameLayer.js:1780` |

---

## Data Transformations

**Input:** End of player's turn
```javascript
{
  currentPlayer: "player1",
  turnNumber: 5,
  mana: 2,         // Remaining mana
  maxMana: 6,
  entitiesExhausted: [unitA, unitB],
  handSize: 4
}
```

**Output:** Start of opponent's turn
```javascript
{
  currentPlayer: "player2",
  turnNumber: 6,
  mana: 7,         // Fully restored (maxMana + 1)
  maxMana: 7,      // Incremented by 1
  entitiesExhausted: [],  // All refreshed
  handSize: 5      // Drew 1 card (if room)
}
```

**Side Effects:**
- Current player swapped (unless bonus turn)
- Max mana incremented (up to CONFIG.MAX_MANA)
- Current mana fully restored
- All entities exhaustion cleared
- 1 card drawn (CONFIG.CARD_DRAW_PER_TURN)
- Modifier durations decremented
- Expired modifiers removed
- Signature card cooldown updated
- Replace count reset to 0

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.MAX_MANA` | 9 | `app/common/config.js:296` | Maximum mana cap |
| `CONFIG.STARTING_MANA` | 2 | `app/common/config.js:297` | Player 1 starting mana |
| `CONFIG.CARD_DRAW_PER_TURN` | 1 | `app/common/config.js:370` | Cards drawn per turn |
| `CONFIG.MAX_HAND_SIZE` | 6 | `app/common/config.js:299` | Maximum hand capacity |
| `CONFIG.TURN_DURATION` | 90.0s | `app/common/config.js:525` | Active player turn time |
| `CONFIG.TURN_DURATION_INACTIVE` | 15.0s | `app/common/config.js:526` | Inactive player time |
| `CONFIG.TURN_DURATION_LATENCY_BUFFER` | 2.0s | `app/common/config.js:527` | Network buffer |
| `CONFIG.TURN_TIME_SHOW` | 20.0s | `app/common/config.js:524` | When to show timer |
| `CONFIG.TURN_DELAY` | 0.0s | `app/common/config.js:495` | Delay after turn actions |
| `CONFIG.MAX_REPLACE_PER_TURN` | 1 | `app/common/config.js:372` | Replaces allowed per turn |

**Player 2 Bonus Mana (gameSession.coffee:387):**
```coffeescript
# Player 2 starts with +1 mana to compensate for going second
player2.setMana(CONFIG.STARTING_MANA + 1)
```

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| End Turn | - | - | `GameLayer.js:1713` |
| Start Turn (My) | "Your Turn" notification | `sfx_ui_yourturn.audio` | `GameLayer.js:1780` |
| Start Turn (Enemy) | "Enemy Turn" notification | `sfx_ui_yourturn.audio` | `GameLayer.js:1780` |
| Turn Timer Warning | Timer glow | Timer tick | When time < 20s |
| StartTurnWatch Trigger | FX.Modifiers.ModifierStartTurnWatch | - | `modifierStartTurnWatch.coffee:19` |
| EndTurnWatch Trigger | FX.Modifiers.ModifierEndTurnWatch | - | `modifierEndTurnWatch.coffee:19` |

**Turn Notification Animation (GameLayer.js:1799-1827):**
```javascript
// Scale in notification
notification.runAction(cc.sequence(
  cc.scaleTo(0.3, 1.0).easing(cc.easeBackOut()),
  cc.delayTime(showDuration),
  cc.spawn(
    cc.moveBy(0.3, offset),
    cc.fadeOut(0.3)
  )
));
```

**Notification Sprites (GameLayer.js:1788-1792):**
- "Your Turn" - shown to current player
- "Enemy Turn" - shown to non-current player
- "Go" - alternate first turn indicator

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Invalid end turn | `validatorEndTurn.coffee` | Action rejected | "Cannot end turn" |
| Not your turn | Turn ownership check | Action blocked | "Not your turn" |
| Turn timeout | Timer expiration | Auto end turn | Turn ends automatically |
| Disconnect during turn | Network layer | Reconnect or forfeit | Connection warning |
| Desync on turn | State validation | Resync game state | N/A (transparent) |

---

## Modifier Duration System

**Duration Properties (modifier.coffee:57-62):**
```coffeescript
durationEndTurn: 0      # Turns until removal (end of turn)
durationStartTurn: 0    # Turns until removal (start of turn)
numEndTurnsElapsed: 0   # Counter for elapsed end turns
numStartTurnsElapsed: 0 # Counter for elapsed start turns
durationRespectsBonusTurns: true  # Extends with extra turns
```

**Duration Update (modifier.coffee:2111-2155):**
```coffeescript
_onEndTurnDurationChange: (event) ->
  if @durationEndTurn > 0
    @numEndTurnsElapsed++
    if @numEndTurnsElapsed >= @durationEndTurn
      # Remove this modifier - it has expired
      @getGameSession().removeModifier(@)
```

---

## Turn Watch Modifiers

**StartTurnWatch (modifierStartTurnWatch.coffee:21-28):**
```coffeescript
onStartTurn: (event) ->
  # Only trigger on card owner's turn
  if @getCard().isOwnedByPlayer(@getGameSession().getCurrentPlayer())
    @onTurnWatch(event)

onTurnWatch: (event) ->
  # Override in subclasses for specific effects
```

**EndTurnWatch (modifierEndTurnWatch.coffee:36-42):**
```coffeescript
onEndTurn: (event) ->
  # Trigger when owner's turn ends
  if @getCard().isOwnedByPlayer(@getGameSession().getCurrentPlayer())
    @onTurnWatch(event)
```

**Common StartTurnWatch Effects:**
- Deal damage to enemy general
- Heal friendly units
- Draw cards
- Spawn units
- Gain stats

---

## Bonus Turn Handling

**TakeAnotherTurnAction (takeAnotherTurnAction.coffee:14-17):**
```coffeescript
_execute: () ->
  super()
  @getGameSession().skipSwapCurrentPlayerNextTurn()
```

**Skip Swap Logic (gameSession.coffee:1337-1348):**
```coffeescript
p_startTurn: () ->
  if @_private.skipSwapCurrentPlayer
    @_private.skipSwapCurrentPlayer = false
    # Don't swap - same player takes another turn
  else
    @swapCurrentPlayer()
```

---

## Turn Lifecycle Summary

```
1. EndTurnAction executed
   ├── p_endTurn() marks turn ended
   ├── EVENTS.end_turn dispatched
   ├── EndTurnWatch modifiers trigger
   └── Turn pushed to turnStack

2. New GameTurn created
   └── StartTurnAction queued

3. StartTurnAction executed
   ├── p_startTurn() called
   ├── Swap current player (unless bonus turn)
   ├── Refresh all entity exhaustion
   ├── Increment max mana (up to 9)
   ├── Restore current mana to max
   └── EVENTS.start_turn dispatched

4. Post-StartTurn Processing
   ├── StartTurnWatch modifiers trigger
   ├── Draw CONFIG.CARD_DRAW_PER_TURN cards
   ├── Update modifier durations
   ├── Remove expired modifiers
   └── Activate signature card if ready

5. UI Updates
   ├── show_end_turn event
   ├── after_show_end_turn event
   ├── show_start_turn event (with sound)
   └── after_show_start_turn event
```
