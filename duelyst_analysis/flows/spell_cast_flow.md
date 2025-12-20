# Flow: Spell Cast

## Trigger
Player plays a spell card from hand targeting a valid position.

**Entry Points:**
- `PlayCardFromHandAction` for spells played from hand
- `PlayCardAction` for programmatic spell execution
- Followup spells after initial spell resolves

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Action created | `app/sdk/actions/playCardFromHandAction.coffee:15-19` | `PlayCardFromHandAction` with spell card |
| T+0.0s | Validation start | `app/sdk/validators/validatorPlayCard.coffee:14-61` | Validate card and target |
| T+0.0s | Card exists check | `app/sdk/validators/validatorPlayCard.coffee:22-24` | Verify card in hand |
| T+0.0s | Target valid check | `app/sdk/validators/validatorPlayCard.coffee:28-30` | `card.getIsPositionValidTarget()` |
| T+0.0s | Followup check | `app/sdk/validators/validatorFollowup.coffee:92-108` | If followup, validate target |
| T+0.0s | Execute begins | `app/sdk/actions/applyCardToBoardAction.coffee:99` | `_execute()` called |
| T+0.0s | Apply to board | `app/sdk/gameSession.coffee:2966-3053` | `applyCardToBoard()` |
| T+0.0s | Spell onApply | `app/sdk/spells/spell.coffee:65-97` | `onApplyToBoard()` |
| T+0.0s | Find positions | `app/sdk/spells/spell.coffee:114-138` | `_findApplyEffectPositions()` |
| T+0.0s | Filter positions | `app/sdk/spells/spell.coffee:291-322` | `_filterApplyPositions()` |
| T+0.0s | Apply effects | `app/sdk/spells/spell.coffee:87` | `onApplyEffectToBoardTile()` per position |
| T+0.0s | One-time effect | `app/sdk/spells/spell.coffee:91` | `onApplyOneEffectToBoard()` |
| T+0.0s | Cantrip draw | `app/sdk/spells/spell.coffee:94-97` | If `drawCardsPostPlay > 0` |
| T+0.0s | Stats tracked | `app/sdk/actions/applyCardToBoardAction.coffee:132` | `totalSpellsCast++` |
| T+0.0s | Hand stat | `app/sdk/actions/playCardFromHandAction.coffee:85` | `totalSpellsPlayedFromHand++` |
| T+0.0s | View receives | `app/view/layers/game/GameLayer.js:759-761` | Detect spell action |
| T+0.0s | Card display | `app/view/layers/game/GameLayer.js:5399-5456` | `showPlayCard()` |
| T+0.5s | Card transition | `app/view/layers/game/GameLayer.js:5399` | Card moves to center |
| T+1.5-2.0s | Cast FX | `app/view/layers/game/GameLayer.js:3936-3957` | SpellCastFX displayed |
| T+2.0s | Applied FX | `app/view/layers/game/GameLayer.js:3944` | SpellAppliedFX at targets |
| T+2.5s | Followup prompt | `app/sdk/validators/validatorFollowup.coffee:124-129` | If spell has followups |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Input | Player | Select spell and target | `app/view/Player.js:1472` |
| Creation | ActionFactory | Create PlayCardFromHandAction | `app/sdk/actions/actionFactory.coffee` |
| Validation | ValidatorPlayCard | Validate card and target | `app/sdk/validators/validatorPlayCard.coffee:14-61` |
| Validation | ValidatorFollowup | Check followup requirements | `app/sdk/validators/validatorFollowup.coffee:89-108` |
| Execution | ApplyCardToBoardAction | Apply spell to board | `app/sdk/actions/applyCardToBoardAction.coffee:99-132` |
| Execution | GameSession | Process card application | `app/sdk/gameSession.coffee:2966-3053` |
| Execution | Spell | Execute spell effects | `app/sdk/spells/spell.coffee:65-97` |
| Effects | Various SpellActions | DamageAction, HealAction, etc. | `app/sdk/actions/` |
| Followup | ValidatorFollowup | Queue followup spell | `app/sdk/validators/validatorFollowup.coffee:124-129` |
| View | GameLayer | Show card and FX | `app/view/layers/game/GameLayer.js:5399-5456` |
| Animation | CardNode | Card display animation | `app/view/nodes/cards/CardNode.js:1754-1906` |
| Audio | AudioEngine | Play spell sounds | `app/audio/audio_engine.js` |

---

## Data Transformations

**Input:** Spell card in hand
```javascript
{
  cardIndex: 10501,
  type: CardType.Spell,
  manaCost: 3,
  handIndex: 2,
  targetPosition: { x: 4, y: 2 },
  spellFilterType: SpellFilterType.EnemyDirect,
  damageAmount: 4  // For damage spells
}
```

**Output:** Spell resolved
```javascript
{
  cardIndex: 10501,
  isActive: false,  // Spell consumed
  appliedPositions: [{ x: 4, y: 2 }],  // Where effects landed
  // Target state changes based on spell:
  targetDamaged: true,  // For damage spells
  modifiersApplied: []  // For buff spells
}
```

**Side Effects:**
- Player mana reduced by spell cost
- Card removed from hand
- `totalSpellsCast` incremented
- `totalSpellsPlayedFromHand` incremented
- Target entities affected (damage, buffs, etc.)
- Followup spells queued if applicable
- Cards drawn if cantrip

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.GENERAL_CAST_START_DELAY` | 0.25s | `app/common/config.js:422` | Delay before cast animation |
| `CONFIG.GENERAL_CAST_END_DELAY` | 0.5s | `app/common/config.js:423` | Delay after cast animation |
| `CONFIG.MY_PLAYED_CARD_TRANSITION_DURATION` | 0.5s | `app/common/config.js:595` | Card move to center |
| `CONFIG.MY_PLAYED_CARD_SHOW_DURATION` | 1.5s | `app/common/config.js:596` | Card display time |
| `CONFIG.OPPONENT_PLAYED_CARD_TRANSITION_DURATION` | 1.0s | `app/common/config.js:601` | Opponent card transition |
| `CONFIG.OPPONENT_PLAYED_CARD_SHOW_DURATION` | 2.0s | `app/common/config.js:602` | Opponent card display |
| `CONFIG.PLAYED_SPELL_FX_FADE_IN_DURATION` | 0.5s | `app/common/config.js:617` | Spell FX fade in |
| `CONFIG.PLAYED_SPELL_FX_FADE_OUT_DURATION` | 0.5s | `app/common/config.js:618` | Spell FX fade out |
| `CONFIG.ANIMATE_MEDIUM_DURATION` | 0.35s | `app/common/config.js:551` | Standard animation |

**Followup FX Template (config.js:620-636):**
```javascript
CONFIG.FOLLOWUP_FX_TEMPLATE = [{
  type: 'Light',
  radius: CONFIG.TILESIZE * 2.0,
  fadeInDuration: 0.5,
  intensity: CONFIG.LIGHT_HIGH_INTENSITY
}];
```

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Card Played | Card appears center | Card sound | `GameLayer.js:5399-5456` |
| Cast Start | General cast anim | - | `GameLayer.js:3815-3839` |
| Spell Cast FX | `SpellCastFX` from card | Spell SFX | `GameLayer.js:3939` |
| Spell Applied FX | `SpellAppliedFX` at targets | Impact SFX | `GameLayer.js:3944` |
| Legendary FX | Special legendary effects | - | `GameLayer.js:3788-3800` |
| Prismatic FX | Prismatic glow | - | `GameLayer.js:3803-3809` |
| Followup Prompt | Highlight valid tiles | - | After spell resolves |
| Error | - | `sfx_ui_error.audio` | `Player.js:1019` |

**FX Detection (GameLayer.js:759-761):**
```javascript
getIsActionSpell(action) {
  return action instanceof SDK.ApplyCardToBoardAction &&
         action.getCard() instanceof SDK.Spell;
}
```

**FX Application (GameLayer.js:3936-3957):**
```javascript
// SpellCastFX - at cast origin
const spellCastFX = card.getFXResource().filter(fx => fx.type === SDK.FXType.SpellCastFX);

// SpellAppliedFX - at each target position
const spellAppliedFX = card.getFXResource().filter(fx => fx.type === SDK.FXType.SpellAppliedFX);
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Card not in hand | `validatorPlayCard.coffee:22-24` | Action invalidated | "Invalid card" |
| Invalid target | `validatorPlayCard.coffee:28-30` | Action invalidated | "Invalid target" |
| Card mismatch | `validatorPlayCard.coffee:32-35` | Action invalidated | "Invalid card" |
| Stale action | `validatorPlayCard.coffee:36-52` | Logs warning | N/A |
| Invalid followup | `validatorFollowup.coffee:107-108` | Action invalidated | "Invalid followup target" |
| Insufficient mana | Mana validation | Action rejected | "Not enough mana" |
| No valid targets | `spell.getValidTargetPositions()` | Card unplayable | Card dimmed in hand |

**Validation Code (validatorPlayCard.coffee:22-30):**
```coffeescript
# Card must exist
if !card
  @invalidateAction(action, targetPosition,
    i18next.t("validators.invalid_card_message"))

# Target must be valid
if !card.getIsPositionValidTarget(targetPosition)
  @invalidateAction(action, targetPosition,
    i18next.t("validators.invalid_card_target_message"))
```

---

## Spell Filter Types

**Location:** `app/sdk/spells/spell.coffee:254-326`

| Filter Type | Description | Target |
|-------------|-------------|--------|
| `AllyDirect` | Friendly units directly | Allies |
| `AllyIndirect` | Friendly tiles for effects | Ally tiles |
| `EnemyDirect` | Enemy units directly | Enemies |
| `EnemyIndirect` | Enemy tiles for effects | Enemy tiles |
| `NeutralDirect` | Any unit | All units |
| `NeutralIndirect` | Any tile | All tiles |
| `SpawnSource` | Valid spawn locations | Empty/friendly tiles |
| `None` | No filtering | All positions |

**Filter Pipeline:**
```
1. _getPrefilteredValidTargetPositions() → Base valid positions
2. Remove already-applied positions (followups)
3. _filterPlayPositions() → Apply spell filter type
4. _postFilterPlayPositions() → Custom spell overrides
```

---

## Spell Types & Examples

| Spell Type | Class | Effect | Example |
|------------|-------|--------|---------|
| Damage | `SpellDamage` | Deal damage to targets | Chromatic Cold |
| Buff | `SpellApplyModifiers` | Apply modifiers | Greater Fortitude |
| Summon | `SpellSpawnEntity` | Create units | Wraithlings |
| Teleport | `SpellFollowupTeleport` | Move unit (2-step) | Mist Walking |
| Draw | `SpellDrawCard` | Draw cards | Blaze Hound |
| Dispel | `SpellDispel` | Remove modifiers | Chromatic Cold |
| Transform | `SpellTransform` | Replace unit | Aspect of the Fox |

---

## Spell Execution Methods

**Location:** `app/sdk/spells/spell.coffee`

```coffeescript
# Main entry point after card applied to board
onApplyToBoard: (board, x, y, sourceAction) ->
  super(board, x, y, sourceAction)

  # Find all positions spell affects
  @_findApplyEffectPositions()

  # Remove duplicate positions
  UtilsPosition.removeDuplicatePositions(@applyEffectPositions)

  # Apply effect at each position
  for applyPosition in @applyEffectPositions
    @cardIndicesAtApplyEffectPosition[i] = @getCardAtPosition(applyPosition)?.getIndex()
    @onApplyEffectToBoardTile(board, applyPosition.x, applyPosition.y, sourceAction)

  # One-time effect (not per-position)
  @onApplyOneEffectToBoard(board, x, y, sourceAction)

  # Draw cards if cantrip
  if @drawCardsPostPlay > 0
    drawAction = new DrawCardAction(@getGameSession())
    drawAction.setOwnerId(@getOwnerId())
    @getGameSession().executeAction(drawAction)
```

**Override Points:**
- `onApplyEffectToBoardTile()` - Called for each target position
- `onApplyOneEffectToBoard()` - Called once after all positions

---

## Followup Spells

**Location:** `app/sdk/validators/validatorFollowup.coffee`

**Followup Queue (lines 124-129):**
```coffeescript
# After spell with followups resolves
if card.getHasFollowups()
  @_followupStack.push(card)
  # Player must now play matching followup
```

**Followup Validation (lines 92-108):**
```coffeescript
# Check followup target is valid
currentFollowupCard = @_followupStack[@_followupStack.length - 1]
validPositions = currentFollowupCard.getValidTargetPositions()

if !UtilsPosition.getIsPositionInPositions(validPositions, action.targetPosition)
  @invalidateAction(action, action.getTargetPosition(),
    i18next.t("validators.invalid_followup_target_message"))
```

**Followup Source Position (spell.coffee:111-112):**
```coffeescript
getFollowupSourcePosition: () ->
  return @_private.followupSourcePosition
```

---

## Action Hierarchy

```
Action (base)
    ↑
ApplyCardToBoardAction
  - Applies any card to board
  - Tracks totalSpellsCast
    ↑
PlayCardAction
  - For played cards (not summoned)
    ↑
PlayCardFromHandAction
  - Specifically from player hand
  - Handles mana cost
  - Tracks totalSpellsPlayedFromHand
```

---

## View Display Sequence

**Location:** `app/view/layers/game/GameLayer.js:5399-5456`

```javascript
showPlayCard(card, ownerId) {
  // 1. Create card node
  const cardNode = CardNode.create(card);

  // 2. Position card off-screen
  cardNode.setPosition(startPosition);

  // 3. Animate to center
  cardNode.runAction(cc.sequence(
    cc.moveTo(transitionDuration, centerPosition),
    cc.delayTime(showDuration - transitionDuration),
    cc.callFunc(() => cardNode.destroy())
  ));

  // Duration depends on player:
  // - My card: 0.5s transition + 1.5s show
  // - Opponent: 1.0s transition + 2.0s show
  // - Replay: 1.5s transition + 2.5s show
}
```
