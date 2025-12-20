# Flow: Card Draw

## Trigger
A card is drawn from the deck to the player's hand.

**Entry Points:**
- End of turn automatic draw (`deck.actionsDrawNewCards()`)
- Card effects that draw cards (`DrawCardAction`)
- Cantrip spells (`spell.drawCardsPostPlay`)
- Opening gambit draw effects

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Draw action created | `app/sdk/cards/deck.coffee:196-230` | `actionsDrawNewCards()` or card effect |
| T+0.0s | Execute begins | `app/sdk/actions/drawCardAction.coffee:16` | `_execute()` called |
| T+0.0s | Get draw pile | `app/sdk/actions/drawCardAction.coffee:18-20` | Access deck's draw pile |
| T+0.0s | Select card | `app/sdk/actions/drawCardAction.coffee:24-29` | Random or specific index |
| T+0.0s | Check empty deck | `app/sdk/actions/drawCardAction.coffee:32-39` | If no cards, apply fatigue |
| T+0.0s | Fatigue damage | `app/sdk/actions/hurtingDamageAction.coffee:11` | 2 damage to general |
| T+0.0s | Remove from deck | `app/sdk/cards/deck.coffee:291-303` | `removeCardIndexFromDeck()` |
| T+0.0s | Find hand slot | `app/sdk/cards/deck.coffee:257-269` | `putCardIndexInHand()` |
| T+0.0s | Check hand full | `app/sdk/cards/deck.coffee:170-172` | `getFirstEmptySpaceInHand()` |
| T+0.0s | Burn if full | `app/sdk/gameSession.coffee:2809-2814` | Card discarded if hand full |
| T+0.0s | Apply to hand | `app/sdk/gameSession.coffee:2779-2838` | `applyCardToHand()` |
| T+0.0s | Card onApplyToHand | Card callback | Card enters hand state |
| T+0.0s | DrawWatch triggers | `app/sdk/modifiers/modifierAnyDrawCardWatch.coffee` | Watch modifiers fire |
| T+0.0s | View notified | `app/view/layers/game/GameLayer.js:3585-3646` | Detect draw action |
| T+0.25s | Draw animation | `app/view/nodes/cards/BottomDeckCardNode.js:869-959` | `showDraw()` |
| T+0.25s | Draw SFX | `app/view/nodes/cards/BottomDeckCardNode.js:912` | `sfx_ui_card_draw.audio` |
| T+0.5s | Draw FX | `app/view/nodes/cards/BottomDeckCardNode.js:884` | `FX.Game.CardDrawFX` |
| T+0.75s | Card visible | Animation complete | Card in hand UI |

**Burn Card Timeline (if hand full):**
| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Hand full detected | `app/sdk/cards/deck.coffee:170-172` | No empty slot |
| T+0.0s | Burn card flag | `app/sdk/actions/drawCardAction.coffee` | `burnCard = true` |
| T+0.25s | "Hand Full" dialog | `app/view/layers/game/GameLayer.js:3597` | `showSpeechForPlayer()` |
| T+0.5s | Burn animation | `app/view/layers/game/GameLayer.js:5509-5547` | `showBurnCard()` |
| T+0.75s | Burn FX | `app/data/fx.js:9369-9371` | `CardBurnFX` particles |
| T+1.0s | Burn SFX | `app/view/nodes/cards/BottomDeckCardNode.js:1063` | `sfx_spell_immolation_a.audio` |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Trigger | Deck/CardEffect | Create DrawCardAction | `app/sdk/cards/deck.coffee:196-230` |
| Execution | DrawCardAction | Select card from deck | `app/sdk/actions/drawCardAction.coffee:24-29` |
| Deck Update | Deck | Remove card from drawPile | `app/sdk/cards/deck.coffee:291-303` |
| Hand Check | Deck | Find empty hand slot | `app/sdk/cards/deck.coffee:257-269` |
| Hand Update | GameSession | Apply card to hand | `app/sdk/gameSession.coffee:2779-2838` |
| Fatigue | HurtingDamageAction | Deal 2 damage if empty | `app/sdk/actions/hurtingDamageAction.coffee:11` |
| Burn | GameSession | Discard if hand full | `app/sdk/gameSession.coffee:2809-2814` |
| Watchers | Modifiers | Fire AnyDrawCardWatch | `app/sdk/modifiers/modifierAnyDrawCardWatch.coffee` |
| View | GameLayer | Detect and route action | `app/view/layers/game/GameLayer.js:3585-3646` |
| Animation | BottomDeckCardNode | Show draw animation | `app/view/nodes/cards/BottomDeckCardNode.js:869-959` |
| Audio | AudioEngine | Play draw sound | `app/view/nodes/cards/BottomDeckCardNode.js:912` |

---

## Data Transformations

**Input:** Draw action triggered
```javascript
{
  deckDrawPile: [cardIdx1, cardIdx2, cardIdx3, ...],
  deckSize: 15,
  handCards: [card1, card2, null, null, null, null],
  handSize: 2,
  cardIndexFromDeck: null  // null = random, or specific index
}
```

**Output (Normal Draw):**
```javascript
{
  deckDrawPile: [cardIdx1, cardIdx2, ...],  // One removed
  deckSize: 14,
  handCards: [card1, card2, card3, null, null, null],
  handSize: 3,
  drawnCard: card3,
  indexInHand: 2
}
```

**Output (Hand Full - Burn):**
```javascript
{
  deckDrawPile: [cardIdx1, cardIdx2, ...],
  deckSize: 14,
  handCards: [card1, card2, card3, card4, card5, card6],  // Unchanged
  handSize: 6,
  burnedCard: cardX,
  indexInHand: null  // Burned
}
```

**Output (Empty Deck - Fatigue):**
```javascript
{
  deckDrawPile: [],
  deckSize: 0,
  handCards: [card1, card2, null, null, null, null],
  handSize: 2,  // Unchanged
  fatigueDamage: 2,
  generalHealth: previousHealth - 2
}
```

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.MAX_HAND_SIZE` | 6 | `app/common/config.js:299` | Maximum cards in hand |
| `CONFIG.STARTING_HAND_SIZE` | 5 | `app/common/config.js:301` | Initial hand draw |
| `CONFIG.CARD_DRAW_PER_TURN` | 1 | `app/common/config.js:370` | Cards per turn end |
| `CONFIG.BURN_CARD_SHOW_DURATION` | 0.25s | `app/common/config.js:481` | Burn card movement |
| `CONFIG.BURN_CARD_DELAY` | 0.25s | `app/common/config.js:482` | Pre-dissolve delay |
| `CONFIG.BURN_CARD_DISSOLVE_DURATION` | 0.75s | `app/common/config.js:483` | Dissolve animation |
| `CONFIG.DIALOGUE_OUT_OF_CARDS_DURATION` | 1.0s | `app/common/config.js:477` | "Out of cards" display |
| `CONFIG.DIALOGUE_HAND_FULL_DURATION` | 1.0s | `app/common/config.js:479` | "Hand full" display |
| `CONFIG.ANIMATE_FAST_DURATION` | 0.2s | `app/common/config.js:550` | Fast animations |
| `CONFIG.FADE_FAST_DURATION` | 0.2s | `app/common/config.js:553` | Fast fades |

**Fatigue Damage (hurtingDamageAction.coffee:11):**
```coffeescript
damageAmount: 2  # Fixed 2 true damage per empty draw
```

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Card Draw | `FX.Game.CardDrawFX` | `sfx_ui_card_draw.audio` | `BottomDeckCardNode.js:884,912` |
| Card Remove | `FX.Game.CardRemoveFX` | `sfx_ui_card_draw.audio` | `BottomDeckCardNode.js:988` |
| Card Burn | `FX.Game.CardBurnFX` | `sfx_spell_immolation_a.audio` | `BottomDeckCardNode.js:1047,1063` |
| Out of Cards | - | - | `GameLayer.js:3589` (dialogue) |
| Hand Full | - | - | `GameLayer.js:3597` (dialogue) |
| Glow Rings | Particle system | - | `BottomDeckCardNode.js:906` |

**FX Definitions (fx.js:9359-9371):**
```javascript
CardDrawFX: [
  { spriteIdentifier: RSX.fx_carddraw.name, scale: 1 }
],
CardRemoveFX: [
  { spriteIdentifier: RSX.fxCrossSlash.name, offset: { x: 0, y: 40 } },
  { spriteIdentifier: RSX.fxBladestorm.name }
],
CardBurnFX: [
  { plistFile: RSX.ptcl_fireexplosion.plist, type: 'Particles' }
]
```

**Draw Animation Sequence (BottomDeckCardNode.js:902-955):**
```javascript
// 1. Resume glow rings
this._glowRingsParticleSystem.resumeSystem();

// 2. Play draw SFX
audio_engine.current().play_effect(RSX.sfx_ui_card_draw.audio);

// 3. Fade in mana cost and shadow
this._manaCostLabel.runAction(cc.fadeIn(fadeDuration));
this._shadowSprite.runAction(cc.fadeIn(fadeDuration));

// 4. Animate card sprite with effects
this._cardSprite.setVisible(true);
this._cardSprite.setLeveled(true);
this._cardSprite.setHighlighted(true);
// Fade in with tint animation
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| Empty deck | `drawCardAction.coffee:32-39` | Apply fatigue damage | "OUT OF CARDS" dialogue |
| Hand full | `deck.coffee:170-172` | Burn card (discard) | "HAND IS FULL" dialogue |
| Invalid card index | Card retrieval | Skip draw | N/A |
| Deck desync | State validation | Resync | N/A |

---

## DrawCardAction Implementation

**Location:** `app/sdk/actions/drawCardAction.coffee`

```coffeescript
class DrawCardAction extends PutCardInHandAction
  cardIndexFromDeck: null

  _execute: () ->
    player = @getOwner()
    drawPile = player.getDeck().getDrawPile()

    # Determine card to draw
    if @cardIndexFromDeck?
      @cardDataOrIndex = @cardIndexFromDeck
    else if !@getGameSession().getAreDecksRandomized()
      @cardDataOrIndex = drawPile[drawPile.length - 1]  # Top of deck
    else
      @cardDataOrIndex = drawPile[Math.floor(Math.random() * drawPile.length)]

    # Handle empty deck (fatigue)
    if @getIsDrawFromEmptyDeck() and !@burnCard
      general = @getGameSession().getGeneralForPlayerId(player.getPlayerId())
      hurtingAction = new HurtingDamageAction(@getGameSession())
      hurtingAction.setOwnerId(@getOwnerId())
      hurtingAction.setTarget(general)
      @getGameSession().executeAction(hurtingAction)

    super()  # PutCardInHandAction._execute()

  getIsDrawFromEmptyDeck: () ->
    return !@cardDataOrIndex?
```

---

## Deck Draw Pile Management

**Location:** `app/sdk/cards/deck.coffee`

**Get Draw Pile (deck.coffee:56-68):**
```coffeescript
getDrawPile: () ->
  return @drawPile

getDrawPileExcludingMissing: () ->
  drawPile = []
  for cardIndex in @drawPile
    if cardIndex? and @getGameSession().getCardByIndex(cardIndex)?
      drawPile.push(cardIndex)
  return drawPile
```

**Remove Card from Deck (deck.coffee:291-303):**
```coffeescript
removeCardIndexFromDeck: (cardIndex) ->
  if cardIndex?
    for i in [0...@drawPile.length]
      if @drawPile[i] == cardIndex
        @drawPile.splice(i, 1)
        @flushCachedCards()
        return i
  return null
```

---

## Hand Management

**Location:** `app/sdk/cards/deck.coffee`

**Put Card in Hand (deck.coffee:257-269):**
```coffeescript
putCardIndexInHand: (cardIndex) ->
  if cardIndex?
    # Find first empty slot
    for i in [0...CONFIG.MAX_HAND_SIZE]
      if !@hand[i]?
        @hand[i] = cardIndex
        @flushCachedCards()
        return i
  return null  # Hand full
```

**Check Hand Space (deck.coffee:170-172):**
```coffeescript
getFirstEmptySpaceInHand: () ->
  for i in [0...CONFIG.MAX_HAND_SIZE]
    if !@hand[i]? then return i
  return null  # No space
```

**Count Cards in Hand (deck.coffee:178-182):**
```coffeescript
getNumCardsInHand: () ->
  count = 0
  for i in [0...CONFIG.MAX_HAND_SIZE]
    if @hand[i]? then count++
  return count
```

---

## End of Turn Draw Generation

**Location:** `app/sdk/cards/deck.coffee:196-230`

```coffeescript
actionsDrawNewCards: () ->
  actions = []
  gameSession = @getGameSession()

  # Base draw count
  numCardsToDraw = CONFIG.CARD_DRAW_PER_TURN

  # Check for draw modifiers
  for modifier in @getOwner().getPlayerModifiersByClass(PlayerModifierCardDrawModifier)
    numCardsToDraw += modifier.getCardDrawChange()

  # Draw cards (fill empty slots first)
  for i in [0...numCardsToDraw]
    emptyIndex = @getFirstEmptySpaceInHand()
    if emptyIndex?
      # Normal draw to hand
      drawAction = new DrawCardAction(gameSession)
      drawAction.setOwnerId(@getOwner().getPlayerId())
      actions.push(drawAction)
    else
      # Hand full - burn the card
      drawAction = new DrawCardAction(gameSession)
      drawAction.setOwnerId(@getOwner().getPlayerId())
      drawAction.burnCard = true
      actions.push(drawAction)

  return actions
```

---

## Draw Watch Modifiers

**Base Class:** `app/sdk/modifiers/modifierAnyDrawCardWatch.coffee`

```coffeescript
class ModifierAnyDrawCardWatch extends Modifier
  activeOnBoard: true

  onAction: (e) ->
    action = e.action
    if action instanceof DrawCardAction and !(action instanceof BurnCardAction)
      @onDrawCardWatch(action)

  onDrawCardWatch: (action) ->
    # Override in subclasses
```

**Example Implementations:**
- `ModifierAnyDrawCardWatchBuffSelf` - Gain stats when drawing
- `ModifierAnyDrawCardWatchDamageEnemy` - Deal damage when drawing
- `ModifierMyDrawCardWatch` - Only triggers on own draws

---

## View Display Logic

**Location:** `app/view/layers/game/GameLayer.js:3585-3646`

```javascript
// Check for draw from empty deck
if (action instanceof SDK.DrawCardAction && action.getIsDrawFromEmptyDeck()) {
  // Show "OUT OF CARDS" speech
  this.showSpeechForPlayer(player, "OUT OF CARDS",
    CONFIG.DIALOGUE_OUT_OF_CARDS_DURATION);
}

// Check for burned card (hand full)
if (action.getCard() && handIndex == null) {
  // Hand is at max size
  if (deck.getNumCardsInHand() >= CONFIG.MAX_HAND_SIZE) {
    this.showSpeechForPlayer(player, "HAND IS FULL",
      CONFIG.DIALOGUE_HAND_FULL_DURATION);
  }
  // Show burn animation
  this.showBurnCard(action,
    CONFIG.BURN_CARD_DELAY,
    CONFIG.BURN_CARD_SHOW_DURATION,
    CONFIG.BURN_CARD_DELAY,
    CONFIG.BURN_CARD_DISSOLVE_DURATION);
}

// Normal draw animation
this.bottomDeckLayer.showDrawCard(action);
```

---

## Card Draw Sequence Summary

```
1. Draw Triggered
   ├── End of turn: deck.actionsDrawNewCards()
   ├── Card effect: DrawCardAction created
   └── Cantrip: spell.drawCardsPostPlay > 0

2. DrawCardAction._execute()
   ├── Get player's deck draw pile
   ├── Select card (random or specific)
   ├── If deck empty → HurtingDamageAction (2 damage)
   └── Call super (PutCardInHandAction)

3. PutCardInHandAction
   ├── Find empty hand slot
   ├── If hand full → burn card
   └── Apply card to hand

4. GameSession.applyCardToHand()
   ├── Update deck.hand array
   ├── Call card.onApplyToHand()
   └── Fire update_cache_action event

5. Modifier Triggers
   └── AnyDrawCardWatch modifiers fire

6. UI Updates
   ├── GameLayer detects DrawCardAction
   ├── Show dialogue if empty/full
   ├── BottomDeckCardNode.showDraw()
   ├── Play sfx_ui_card_draw sound
   └── Show CardDrawFX
```
