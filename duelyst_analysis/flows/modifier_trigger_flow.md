# Flow: Modifier Trigger

## Trigger
An action executes, firing events that modifiers can respond to.

**Entry Points:**
- Any `Action._execute()` completion
- Turn change events (start_turn, end_turn)
- Modifier activation/deactivation
- Aura application/removal

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Action created | `app/sdk/gameSession.coffee:1685` | Action enters execution queue |
| T+0.0s | Modify for execution | `app/sdk/gameSession.coffee:1735` | `EVENTS.modify_action_for_execution` |
| T+0.0s | Modifiers modify action | `app/sdk/modifiers/modifier.coffee:2072` | `_onModifyActionForExecution()` |
| T+0.0s | Overwatch event | `app/sdk/gameSession.coffee:1749` | `EVENTS.overwatch` (bufferable) |
| T+0.0s | Before action event | `app/sdk/gameSession.coffee:1753` | `EVENTS.before_action` (bufferable) |
| T+0.0s | Modifiers onBeforeAction | `app/sdk/modifiers/modifier.coffee:2078-2083` | `_onBeforeAction()` → `onBeforeAction()` |
| T+0.0s | Action executes | `app/sdk/gameSession.coffee:1767` | `actionToExecute._execute()` |
| T+0.0s | Action event | `app/sdk/gameSession.coffee:1787` | `EVENTS.action` (bufferable) |
| T+0.0s | Modifiers onAction | `app/sdk/modifiers/modifier.coffee:2085-2095` | `_onAction()` → `onAction()` |
| T+0.0s | Artifact durability | `app/sdk/modifiers/modifier.coffee:2089-2090` | Reduce durability on damage |
| T+0.0s | After action event | `app/sdk/gameSession.coffee:1795` | `EVENTS.after_action` (bufferable) |
| T+0.0s | Modifiers onAfterAction | `app/sdk/modifiers/modifier.coffee:2097-2102` | `_onAfterAction()` → `onAfterAction()` |
| T+0.0s | Cache update event | `app/sdk/gameSession.coffee:1806` | `EVENTS.update_cache_action` (NOT bufferable) |
| T+0.0s | Cleanup action event | Event dispatch | `EVENTS.after_cleanup_action` |
| T+0.0s | Modifiers onAfterCleanup | `app/sdk/modifiers/modifier.coffee:2104-2109` | `_onAfterCleanupAction()` |
| T+0.0s | Active change event | `app/sdk/gameSession.coffee:1815+` | `EVENTS.modifier_active_change` |
| T+0.0s | Aura removal event | Event dispatch | `EVENTS.modifier_remove_aura` |
| T+0.0s | Aura addition event | Event dispatch | `EVENTS.modifier_add_aura` |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Dispatch | GameSession | Push event to session | `app/sdk/gameSession.coffee:620-637` |
| Routing | GameSession | Route to validators, players, cards | `app/sdk/gameSession.coffee:625-637` |
| Card Routing | Card | Forward to modifiers | `app/sdk/cards/card.coffee:181` |
| Event Check | Modifier | Check if listening | `app/sdk/modifiers/modifier.coffee:251` |
| Event Type | Modifier | Route by event type | `app/sdk/modifiers/modifier.coffee:251-284` |
| Active Check | Modifier | Verify modifier is active | `app/sdk/modifiers/modifier.coffee:2078` |
| Ancestor Check | Modifier | Prevent self-triggering | `app/sdk/modifiers/modifier.coffee:1735-1740` |
| Stack Push | GameSession | Track triggering modifier | `app/sdk/gameSession.coffee:3369-3376` |
| Handler Call | Modifier | Execute public handler | `app/sdk/modifiers/modifier.coffee:2238-2262` |
| Stack Pop | GameSession | Clear triggering modifier | `app/sdk/gameSession.coffee:3372` |

---

## Data Transformations

**Input:** Event fired from action
```javascript
{
  type: EVENTS.action,
  action: {
    type: "DamageAction",
    source: unitA,
    target: unitB,
    damageAmount: 3
  }
}
```

**Modifier Processing:**
```javascript
{
  modifier: {
    type: "ModifierDealDamageWatch",
    isActive: true,
    card: unitA,
    canReactToAction: true  // Not ancestor
  },
  reaction: {
    // Spawns sub-action based on modifier effect
    subAction: ApplyModifierAction  // e.g., buff self
  }
}
```

**Output:** Triggered effect executed
```javascript
{
  originalAction: DamageAction,
  triggeredModifier: "ModifierDealDamageWatch",
  resultingActions: [ApplyModifierAction],
  modifierStackDepth: 2
}
```

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| Event buffering | true | `app/sdk/gameSession.coffee:1753` | before_action bufferable |
| Event buffering | true | `app/sdk/gameSession.coffee:1787` | action bufferable |
| Event buffering | true | `app/sdk/gameSession.coffee:1795` | after_action bufferable |
| Event buffering | false | `app/sdk/gameSession.coffee:1806` | update_cache NOT bufferable |

**Modifier Duration Properties (modifier.coffee:57-62):**
```coffeescript
durationEndTurn: 0        # Turns until auto-removal
durationStartTurn: 0      # Turns until auto-removal
numEndTurnsElapsed: 0     # Counter
numStartTurnsElapsed: 0   # Counter
```

---

## FX/Sound Events

| Event | FX Resource | Sound | Trigger Point |
|-------|-------------|-------|---------------|
| Watch Trigger | Per-modifier FX | Per-modifier | When watch fires |
| Buff Applied | FX.Modifiers.ModifierGenericBuff | - | Buff modifiers |
| Damage Watch | FX.Modifiers.ModifierGenericDamageSmall | - | Damage triggers |
| Spawn Watch | FX.Modifiers.ModifierGenericSpawn | - | Spawn triggers |
| StartTurnWatch | FX.Modifiers.ModifierStartTurnWatch | - | Turn start |
| EndTurnWatch | FX.Modifiers.ModifierEndTurnWatch | - | Turn end |

**Common FX Patterns (modifier subclasses):**
```coffeescript
# modifierDealDamageWatch.coffee
fxResource: ["FX.Modifiers.ModifierDealDamageWatch"]

# modifierMyAttackWatch.coffee
fxResource: ["FX.Modifiers.ModifierMyAttackWatch"]

# modifierReplaceWatch.coffee
fxResource: ["FX.Modifiers.ModifierReplaceWatch"]
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Infinite loop | Ancestor check | Block self-trigger | N/A (prevented) |
| Invalid target | Action validation | Skip modifier effect | N/A |
| Modifier removed | Active check | Skip callback | N/A |
| Stack overflow | Modifier stack depth | Limit recursion | N/A |
| Desync | State validation | Resync | N/A |

---

## Event Type Constants

**Location:** `app/common/event_types.js`

| Event | Line | Bufferable | Purpose |
|-------|------|------------|---------|
| `modify_action_for_validation` | 189 | - | Validation phase modification |
| `validate_action` | 192 | - | Action validation |
| `modify_action_for_execution` | 204 | - | Execution phase modification |
| `overwatch` | 207 | Yes | Pre-action triggers |
| `before_action` | 210 | Yes | Just before execution |
| `action` | 213 | Yes | Main action event |
| `after_action` | 216 | Yes | Post-execution |
| `after_cleanup_action` | 222 | - | Final cleanup phase |
| `modifier_active_change` | 225 | - | Activation state change |
| `modifier_remove_aura` | 228 | - | Aura removal |
| `modifier_add_aura` | 231 | - | Aura application |
| `modifier_end_turn_duration_change` | 234 | - | Duration decrement |
| `modifier_start_turn_duration_change` | 237 | - | Duration decrement |
| `start_turn` | 259 | - | Turn started |
| `end_turn` | 258 | - | Turn ended |

---

## Core Event Handler

**Location:** `app/sdk/modifiers/modifier.coffee:251-284`

```coffeescript
onEvent: (event) ->
  if @_private.listeningToEvents
    eventType = event.type
    switch eventType
      when EVENTS.terminate, EVENTS.before_deserialize
        @_onTerminate(event)
      when EVENTS.modify_action_for_validation
        @_onModifyActionForValidation(event)
      when EVENTS.validate_action
        @_onValidateAction(event)
      when EVENTS.modify_action_for_execution
        @_onModifyActionForExecution(event)
      when EVENTS.before_action
        @_onBeforeAction(event)
      when EVENTS.action
        @_onAction(event)
      when EVENTS.after_action
        @_onAfterAction(event)
      when EVENTS.after_cleanup_action
        @_onAfterCleanupAction(event)
      when EVENTS.modifier_active_change
        @_onActiveChange(event)
      when EVENTS.modifier_remove_aura
        @_onRemoveAura(event)
      when EVENTS.modifier_add_aura
        @_onAddAura(event)
      when EVENTS.start_turn
        @_onStartTurn(event)
      when EVENTS.end_turn
        @_onEndTurn(event)
```

---

## Ancestor Check (Loop Prevention)

**Location:** `app/sdk/modifiers/modifier.coffee:1735-1740`

```coffeescript
getCanReactToAction: (action) ->
  appliedByAction = @getAppliedByAction()
  if appliedByAction? and appliedByAction.getTarget() != @getCard()
    return appliedByAction.getIndex() < action.getIndex() and
           !@getIsAncestorForAction(action)
  else
    return !@getIsAncestorForAction(action)
```

**Purpose:** Prevents modifiers from reacting to:
1. Actions that created/applied them
2. Actions caused by their own effects (ancestor chain)

This prevents infinite loops like: DamageWatch → DealDamage → DamageWatch → ...

---

## Modifier Stack Management

**Location:** `app/sdk/gameSession.coffee:3369-3376`

```coffeescript
pushTriggeringModifierOntoStack: (modifier) ->
  @modifierStack.push(modifier)

popTriggeringModifierFromStack: () ->
  return @modifierStack.pop()

getTriggeringModifier: () ->
  return @modifierStack[@modifierStack.length - 1]
```

**Usage in Handler (modifier.coffee:2079-2083):**
```coffeescript
_onBeforeAction: (event) ->
  if @_private.cachedIsActive and @getCanReactToAction(action)
    @getGameSession().pushTriggeringModifierOntoStack(@)
    @onBeforeAction(event)
    @getGameSession().popTriggeringModifierFromStack()
```

---

## Watch Modifier Types

**322 Watch Modifiers in Codebase**

| Watch Type | Trigger | Base Class | Example |
|------------|---------|------------|---------|
| DealDamageWatch | This unit deals damage | `ModifierDealDamageWatch` | Buff on damage |
| TakeDamageWatch | This unit takes damage | `ModifierTakeDamageWatch` | Heal on damage |
| MyAttackWatch | This unit attacks | `ModifierMyAttackWatch` | Effect on attack |
| BeforeMyAttackWatch | Before attack resolves | `ModifierBeforeMyAttackWatch` | Pre-attack buff |
| AnySummonWatch | Any unit summoned | `ModifierAnySummonWatch` | React to summons |
| AnyDrawCardWatch | Any card drawn | `ModifierAnyDrawCardWatch` | Draw triggers |
| ReplaceWatch | Card replaced | `ModifierReplaceWatch` | Replace synergy |
| StartTurnWatch | Turn starts | `ModifierStartTurnWatch` | Start-of-turn effect |
| EndTurnWatch | Turn ends | `ModifierEndTurnWatch` | End-of-turn effect |
| DeathWatch | Unit dies | `ModifierDeathWatch` | Death triggers |
| HealWatch | Unit healed | `ModifierHealWatch` | Heal synergy |

---

## Watch Pattern Implementation

**Example: ModifierDealDamageWatch**
```coffeescript
class ModifierDealDamageWatch extends Modifier
  activeOnBoard: true

  onAction: (actionEvent) ->
    if @getIsActionRelevant(actionEvent.action)
      @onDealDamage(actionEvent.action)

  onAfterCleanupAction: (actionEvent) ->
    if @getIsActionRelevant(actionEvent.action)
      @onAfterDealDamage(actionEvent.action)

  willDealDamage: (action) ->
    return action.getTotalDamageAmount() > 0

  onDealDamage: (action) ->
    # Override in subclasses - during EVENTS.action

  onAfterDealDamage: (action) ->
    # Override in subclasses - during EVENTS.after_cleanup_action
```

**Example: ModifierAnySummonWatch (with retroactive)**
```coffeescript
class ModifierAnySummonWatch extends Modifier
  onAction: (e) ->
    if @getIsActionRelevant(e.action)
      @onSummonWatch(e.action)

  onActivate: () ->
    # Retroactive - check past summons on activation
    summonActions = @getGameSession().filterActions(@getIsActionRelevant.bind(@))
    for action in summonActions
      @onSummonWatch(action)
```

---

## ModifiersContextObjects Pattern

**Definition (modifier.coffee:76):**
```coffeescript
modifiersContextObjects: null  # Context objects for modifiers to apply
```

**Helper Method (modifier.coffee:1747-1751):**
```coffeescript
applyManagedModifiersFromModifiersContextObjects: (modifiersContextObjects, card) ->
  if modifiersContextObjects? and card?
    for modifierContextObject in modifiersContextObjects
      @getGameSession().applyModifierContextObject(modifierContextObject, card, @)
```

**Usage Example (modifierAnyDrawCardWatchBuffSelf.coffee:18-27):**
```coffeescript
@createContextObject: (attackBuff, maxHPBuff, options) ->
  contextObject = super(options)
  contextObject.modifiersContextObjects = [
    Modifier.createContextObjectWithAttributeBuffs(attackBuff, maxHPBuff)
  ]
  return contextObject

onDrawCardWatch: (action) ->
  @applyManagedModifiersFromModifiersContextObjects(
    @modifiersContextObjects,
    @getCard()
  )
```

---

## Active State Caching

**Cached Properties (modifier.coffee:125-129):**
```coffeescript
p.cachedIsActive = false
p.cachedIsActiveInLocation = false
p.cachedCard = null
p.cachedWasActive = false
p.cachedWasActiveInLocation = false
```

**Active Check Pattern:**
```coffeescript
_onAction: (event) ->
  action = event.action
  if @_private.cachedIsActive and @getCanReactToAction(action)
    # Safe to trigger - modifier is active and not ancestor
```

---

## Event Dispatch Flow

```
GameSession._executeActionForStep()
    │
    ▼
pushEvent(EVENTS.modify_action_for_execution)
    ├─→ validators.onEvent()
    ├─→ players.onEvent()
    └─→ cards.onEvent()
            └─→ modifiers.onEvent() → _onModifyActionForExecution()
    │
    ▼
pushEvent(EVENTS.overwatch)
    │
    ▼
pushEvent(EVENTS.before_action)
    └─→ modifiers.onEvent() → _onBeforeAction() → onBeforeAction()
    │
    ▼
action._execute()  [EXECUTE pseudo-event on stack]
    │
    ▼
pushEvent(EVENTS.action)
    └─→ modifiers.onEvent() → _onAction() → onAction()
            │
            ▼ (Watch methods fire here)
            onDealDamage(), onDrawCardWatch(), onSummonWatch(), etc.
    │
    ▼
pushEvent(EVENTS.after_action)
    └─→ modifiers.onEvent() → _onAfterAction() → onAfterAction()
    │
    ▼
pushEvent(EVENTS.after_cleanup_action)
    └─→ modifiers.onEvent() → _onAfterCleanupAction() → onAfterCleanupAction()
    │
    ▼
pushEvent(EVENTS.modifier_active_change)
pushEvent(EVENTS.modifier_remove_aura)
pushEvent(EVENTS.modifier_add_aura)
```

---

## Public Override Methods

**Location:** `app/sdk/modifiers/modifier.coffee:2238-2262`

| Method | Line | Timing | Purpose |
|--------|------|--------|---------|
| `onBeforeAction()` | 2238 | EVENTS.before_action | Before action executes |
| `onAction()` | 2241 | EVENTS.action | During action event |
| `onAfterAction()` | 2244 | EVENTS.after_action | After action executes |
| `onAfterCleanupAction()` | 2247 | EVENTS.after_cleanup_action | Final cleanup phase |
| `onEndTurnDurationChange()` | 2250 | Duration event | Turn counter update |
| `onStartTurnDurationChange()` | 2253 | Duration event | Turn counter update |
| `onExpire()` | 2256 | Duration reaches 0 | Modifier about to remove |
| `onStartTurn()` | 2259 | EVENTS.start_turn | Turn begins |
| `onEndTurn()` | 2262 | EVENTS.end_turn | Turn ends |
