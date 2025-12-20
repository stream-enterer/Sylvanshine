# Deck

## Source Evidence
- Primary class: `Deck` (app/sdk/cards/deck.coffee)
- Related classes: SDKObject, Card, Player
- Data shape: Collection of cards for drawing

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| cardIndices | array | card references | yes |
| ownerId | number | player owner | yes |
| drawPile | array | remaining cards | yes |

## Lifecycle Events
- created: GameSession start, deck initialization
- destroyed: game_over
- modified: DrawCardAction, PutCardInDeckAction, shuffle

## Dependencies
- Requires: Card, Player, GameSession
- Used by: Player, DrawCardAction, Spell effects

## Constants (from config.js)
| Constant | Value | Description |
|----------|-------|-------------|
| MAX_DECK_SIZE | 40 | Standard deck size |
| MIN_BASICS_DECK_SIZE | 27 | Minimum for basic decks |
| MAX_DECK_DUPLICATES | 3 | Max copies of a card |
| MAX_DECK_SIZE_GAUNTLET | 31 | Gauntlet deck size |

## Deck Rules
- Maximum 40 cards in constructed
- Maximum 3 copies of any non-general card
- Must include exactly 1 general
- Faction cards only from general's faction (+ neutrals)

## Key Methods
- `drawCard` - Remove and return top card
- `putCardOnTop` - Add card to top of deck
- `putCardOnBottom` - Add card to bottom
- `shuffleCardIntoDeck` - Add card at random position
- `getNumCardsRemaining` - Count remaining cards
- `getCardsInDeck` - Get all cards in deck

## Description
Deck represents a player's draw pile during a game. It's initialized from the player's constructed deck at game start and depleted as cards are drawn. Cards can also be added back to the deck through various game effects.
