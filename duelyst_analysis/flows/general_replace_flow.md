# Flow: General Replace (Card Replace)

## Trigger
Player replaces a card from their hand, shuffling it into the deck and drawing a different card.

**Entry Points:**
- Player drags card to replace zone
- `ReplaceCardFromHandAction` via player input
- Forced replace effects (e.g., "replace your hand")

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Replace action created | `app/sdk/player.coffee:442-444` | `actionReplaceCardFromHand()` |
| T+0.0s | Validation | `app/sdk/validators/validatorReplaceCardFromHand.coffee:13-20` | Verify can replace |
| T+0.0s | Execute begins | `app/sdk/actions/replaceCardFromHandAction.coffee:19` | `_execute()` called |
| T+0.0s | Increment counter | `app/sdk/actions/replaceCardFromHandAction.coffee:26` | `numCardsReplacedThisTurn++` |
| T+0.0s | Get replaced card | `app/sdk/actions/replaceCardFromHandAction.coffee:30` | From hand index |
| T+0.0s | Search draw pile | `app/sdk/actions/replaceCardFromHandAction.coffee:35-52` | Find different card |
| T+0.0s | Set card to draw | `app/sdk/actions/replaceCardFromHandAction.coffee:58` | `@cardDataOrIndex` |
| T+0.0s | Return to deck | `app/sdk/actions/replaceCardFromHandAction.coffee:61` | `applyCardToDeck()` |
| T+0.0s | Add to draw pile | `app/sdk/cards/deck.coffee:247-250` | `putCardIndexIntoDeck()` |
| T+0.0s | Draw new card | `app/sdk/actions/replaceCardFromHandAction.coffee:64` | `super()` (PutCardInHandAction) |
| T+0.0s | ReplaceWatch triggers | `app/sdk/modifiers/modifierReplaceWatch.coffee:19-26` | Watch modifiers fire |
| T+0.0s | On-replace modifiers | `app/sdk/modifiers/modifierBuffSelfOnReplace.coffee` | Card-specific effects |
| T+0.25s | UI animation | View layer | Card swap animation |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Input | Player | Create replace action | `app/sdk/player.coffee:442-444` |
| Validation | ValidatorReplaceCard | Check can replace | `app/sdk/validators/validatorReplaceCardFromHand.coffee:13-20` |
| Validation | Deck | Check replace count | `app/sdk/cards/deck.coffee:334-343` |
| Execution | ReplaceCardFromHandAction | Execute replace | `app/sdk/actions/replaceCardFromHandAction.coffee:19-64` |
| Deck Update | Deck | Return card to draw pile | `app/sdk/cards/deck.coffee:247-250` |
| Draw | PutCardInHandAction | Draw different card | Parent class `_execute()` |
| Watchers | Modifiers | Fire ReplaceWatch | `app/sdk/modifiers/modifierReplaceWatch.coffee` |
| Card Effects | Modifiers | On-replace effects | `app/sdk/modifiers/modifierBuffSelfOnReplace.coffee` |
| Reset | Player | Reset count on turn end | `app/sdk/player.coffee:513-516` |

---

## Data Transformations

**Input:** Replace action initiated
```javascript
{
  handCards: [cardA, cardB, cardC, null, null, null],
  handSize: 3,
  replaceIndex: 1,          // cardB being replaced
  deckDrawPile: [cardX, cardY, cardZ],
  numCardsReplacedThisTurn: 0,
  maxReplacesPerTurn: 1
}
```

**Output:** Replace completed
```javascript
{
  handCards: [cardA, cardY, cardC, null, null, null],  // cardB → cardY
  handSize: 3,
  replacedCard: cardB,
  drawnCard: cardY,
  deckDrawPile: [cardX, cardZ, cardB],  // cardB added, cardY removed
  numCardsReplacedThisTurn: 1,
  canReplaceAgain: false  // At limit
}
```

**Side Effects:**
- `numCardsReplacedThisTurn` incremented (unless forced)
- Replaced card moved from hand to deck
- New card moved from deck to hand
- ReplaceWatch modifiers triggered
- On-replace card effects triggered

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.MAX_REPLACE_PER_TURN` | 1 | `app/common/config.js:372` | Base replace limit |
| `CONFIG.MAX_HAND_SIZE` | 6 | `app/common/config.js:299` | Maximum hand capacity |
| `CONFIG.STARTING_HAND_REPLACE_COUNT` | 2 | `app/common/config.js:303` | Mulligan replaces |

**Replace Count Management:**
- Reset to 0 at end of turn (`player.coffee:513-516`)
- Incremented on each non-forced replace
- Modified by `PlayerModifierReplaceCardModifier`
- Blocked by `PlayerModifierCannotReplace`

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Replace Watch | `FX.Modifiers.ModifierReplaceWatch` | - | `modifierReplaceWatch.coffee:17` |
| Buff on Replace | `FX.Modifiers.ModifierBuffSelfOnReplace` | - | `modifierBuffSelfOnReplace.coffee:18` |
| Damage on Replace | `FX.Modifiers.ModifierGenericDamageSmall` | - | `modifierReplaceWatchDamageEnemy.coffee:13` |
| Spawn on Replace | `FX.Modifiers.ModifierGenericSpawn` | - | `modifierReplaceWatchSpawnEntity.coffee:18` |
| Generic Buff | `FX.Modifiers.ModifierGenericBuff` | - | Various replace modifiers |

**Modifier FX Resources:**
```coffeescript
# modifierReplaceWatch.coffee:17
fxResource: ["FX.Modifiers.ModifierReplaceWatch"]

# modifierReplaceWatchDamageEnemy.coffee:13
fxResource: ["FX.Modifiers.ModifierReplaceWatch",
             "FX.Modifiers.ModifierGenericDamageSmall"]

# modifierReplaceWatchBuffSelf.coffee:14
fxResource: ["FX.Modifiers.ModifierReplaceWatch",
             "FX.Modifiers.ModifierGenericBuff"]
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| No card at index | `validatorReplaceCardFromHand.coffee:18` | Action invalidated | "Invalid card" |
| Already replaced | `deck.coffee:334-343` | Action invalidated | "Already replaced" |
| Cannot replace | PlayerModifierCannotReplace | Action blocked | "Cannot replace" |
| Empty deck | Draw pile empty check | May draw same card | N/A |
| Only same cards | All deck = same card | Returns replaced card | N/A |

---

## Validation Logic

**Location:** `app/sdk/validators/validatorReplaceCardFromHand.coffee`

```coffeescript
# Line 13-14: Forced replaces skip validation
if action.getIsForcedReplace()
  return

# Line 18: Card must exist in hand
if !deck.getCardIndexInHandAtIndex(indexOfCardInHand)?
  @invalidateAction(action)

# Line 20: Must be able to replace this turn
if !deck.getCanReplaceCardThisTurn()
  @invalidateAction(action)
```

**Can Replace Check (deck.coffee:334-343):**
```coffeescript
getCanReplaceCardThisTurn: () ->
  # Check for blocking modifier
  if @getOwner().getPlayerModifiersByClass(PlayerModifierCannotReplace).length > 0
    return false

  # Calculate allowed replaces
  replacesAllowedThisTurn = CONFIG.MAX_REPLACE_PER_TURN
  replaceCardChange = 0
  for modifier in @getOwner().getPlayerModifiersByClass(PlayerModifierReplaceCardModifier)
    replaceCardChange += modifier.getReplaceCardChange()
  replacesAllowedThisTurn += replaceCardChange

  # Check limit and deck not empty
  return @numCardsReplacedThisTurn < replacesAllowedThisTurn and @drawPile.length > 0
```

---

## ReplaceCardFromHandAction Implementation

**Location:** `app/sdk/actions/replaceCardFromHandAction.coffee`

```coffeescript
class ReplaceCardFromHandAction extends PutCardInHandAction
  replacedCardIndex: null
  forcedReplace: false  # If true, doesn't count toward limit

  _execute: () ->
    deck = @getOwner().getDeck()

    # Increment replace counter (unless forced)
    if !@forcedReplace
      deck.setNumCardsReplacedThisTurn(deck.getNumCardsReplacedThisTurn() + 1)

    # Get the card being replaced
    @replacedCardIndex = deck.getCardIndexInHandAtIndex(@indexOfCardInHand)

    # Search for a DIFFERENT card in deck
    drawPile = deck.getDrawPile()
    cardIndexToDraw = null

    if @getGameSession().getAreDecksRandomized()
      # Random search for different card
      shuffledIndices = _.shuffle([0...drawPile.length])
      for i in shuffledIndices
        if drawPile[i] != @replacedCardIndex
          cardIndexToDraw = drawPile[i]
          break
    else
      # Sequential search from end
      for i in [drawPile.length-1..0]
        if drawPile[i] != @replacedCardIndex
          cardIndexToDraw = drawPile[i]
          break

    # Fallback: if all cards same, take top of deck
    if !cardIndexToDraw? and drawPile.length > 0
      cardIndexToDraw = drawPile[drawPile.length - 1]

    @cardDataOrIndex = cardIndexToDraw

    # Return replaced card to deck
    @getGameSession().applyCardToDeck(deck, @replacedCardIndex,
      @getGameSession().getCardByIndex(@replacedCardIndex), @)

    # Draw the new card (via parent class)
    super()
```

---

## Return Card to Deck

**Location:** `app/sdk/cards/deck.coffee:247-250`

```coffeescript
putCardIndexIntoDeck: (cardIndex) ->
  if cardIndex? and !_.contains(@drawPile, cardIndex)
    @drawPile.push(cardIndex)  # Add to end of draw pile
    @flushCachedCards()
```

**GameSession Integration (gameSession.coffee:2701-2726):**
```coffeescript
applyCardToDeck: (deck, cardDataOrIndex, card, sourceAction) ->
  # Add to deck's draw pile
  deck.putCardIndexIntoDeck(cardIndex)

  # Fire events
  @pushEvent(type: EVENTS.apply_card_to_deck)

  # Notify card
  card.onApplyToDeck(deck, sourceAction)
```

---

## Replace Watch Modifiers

**Base Class:** `app/sdk/modifiers/modifierReplaceWatch.coffee`

```coffeescript
class ModifierReplaceWatch extends Modifier
  type: ModifierType.ReplaceWatch
  activeOnBoard: true
  fxResource: ["FX.Modifiers.ModifierReplaceWatch"]

  onAction: (e) ->
    action = e.action
    # Watch for replace actions by card owner
    if action instanceof ReplaceCardFromHandAction
      if action.getOwnerId() == @getCard().getOwnerId()
        @onReplaceWatch(action)

  onReplaceWatch: (action) ->
    # Override in subclasses
```

**Replace Watch Implementations:**

| Modifier | Effect | File |
|----------|--------|------|
| `ModifierReplaceWatchDamageEnemy` | Deal X damage to random enemy | `modifierReplaceWatchDamageEnemy.coffee` |
| `ModifierReplaceWatchBuffSelf` | Gain +X/+X | `modifierReplaceWatchBuffSelf.coffee` |
| `ModifierReplaceWatchSpawnEntity` | Spawn unit on replace | `modifierReplaceWatchSpawnEntity.coffee` |
| `ModifierReplaceWatchShuffleCardIntoDeck` | Add card to deck | `modifierReplaceWatchShuffleCardIntoDeck.coffee` |
| `ModifierReplaceWatchApplyModifiersToReplaced` | Buff replaced card | `modifierReplaceWatchApplyModifiersToReplaced.coffee` |

---

## On-Replace Card Modifiers

**Cards that trigger when THEY are replaced:**

**ModifierBuffSelfOnReplace (modifierBuffSelfOnReplace.coffee:38-47):**
```coffeescript
class ModifierBuffSelfOnReplace extends Modifier
  activeInHand: true   # Active while in hand
  activeInDeck: true   # Active while in deck

  onAction: (e) ->
    action = e.action
    if action instanceof ReplaceCardFromHandAction
      # Check if THIS card was the one replaced
      if action.replacedCardIndex == @getCard().getIndex()
        @onReplaceWatch(action)
```

**ModifierSummonSelfOnReplace (modifierSummonSelfOnReplace.coffee):**
- Summons the card when replaced
- Deals 2 damage to owner's general
- Example: Dreamgazer

**ModifierDamageBothGeneralsOnReplace:**
- Damages both generals when replaced
- Active in hand and deck

---

## Player Modifiers Affecting Replace

**PlayerModifierCannotReplace (playerModifierCannotReplace.coffee):**
```coffeescript
class PlayerModifierCannotReplace extends PlayerModifier
  # When active, player cannot replace cards
  # Checked in deck.getCanReplaceCardThisTurn()
```

**PlayerModifierReplaceCardModifier (playerModifierReplaceCardModifier.coffee:4-21):**
```coffeescript
class PlayerModifierReplaceCardModifier extends PlayerModifier
  replaceCardChange: 0

  getReplaceCardChange: () ->
    return @replaceCardChange

  @createContextObject: (replaceCardChange, options) ->
    contextObject = super(options)
    contextObject.replaceCardChange = replaceCardChange
    return contextObject
```

---

## Forced Replace (Opening Gambit)

**Location:** `app/sdk/modifiers/modifierOpeningGambitReplaceHand.coffee`

```coffeescript
class ModifierOpeningGambitReplaceHand extends ModifierOpeningGambit
  onOpeningGambit: () ->
    player = @getCard().getOwner()
    deck = player.getDeck()
    hand = deck.getHand()

    # Replace each card in hand
    for cardIndex, i in hand
      if cardIndex?
        replaceAction = new ReplaceCardFromHandAction(@getGameSession())
        replaceAction.setOwnerId(player.getPlayerId())
        replaceAction.setIndexOfCardInHand(i)
        replaceAction.forcedReplace = true  # Doesn't count toward limit
        @getGameSession().executeAction(replaceAction)
```

---

## Reset Replace Counter

**Location:** `app/sdk/player.coffee:513-516`

```coffeescript
# Called on EVENTS.end_turn
onEndTurn: (event) ->
  # Reset replace counter for next turn
  @getDeck().setNumCardsReplacedThisTurn(0)
```

---

## Replace Flow Summary

```
1. Player Initiates Replace
   ├── Drag card to replace zone
   └── Player.actionReplaceCardFromHand(indexOfCardInHand)

2. Validation
   ├── ValidatorReplaceCardFromHand checks:
   │   ├── Card exists at hand index
   │   ├── deck.getCanReplaceCardThisTurn()
   │   └── Not blocked by PlayerModifierCannotReplace
   └── Forced replaces skip validation

3. ReplaceCardFromHandAction._execute()
   ├── Increment numCardsReplacedThisTurn (if not forced)
   ├── Get replaced card from hand
   ├── Search draw pile for DIFFERENT card
   ├── Return replaced card to deck (push to drawPile)
   └── Call super() to draw new card

4. Modifier Triggers
   ├── ReplaceWatch modifiers fire (board minions)
   └── BuffSelfOnReplace modifiers fire (the replaced card)

5. End of Turn
   └── numCardsReplacedThisTurn reset to 0
```

---

## Note: No Mana Refund

The replace mechanic does **NOT** refund mana:
- Replace is a free action (costs 0 mana)
- The replaced card is simply returned to deck
- A new card is drawn in its place
- Limited to CONFIG.MAX_REPLACE_PER_TURN (1) per turn
