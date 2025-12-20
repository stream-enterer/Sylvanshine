# Player

## Source Evidence
- Primary class: `Player` (app/sdk/player.coffee)
- Related classes: SDKObject, Deck, General
- Data shape: Player state and resources

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| playerId | string | player identification | yes |
| generalId | number | assigned general card | yes |
| deckId | string | deck reference | yes |
| mana | number | current mana | yes |
| manaMax | number | max mana this turn | yes |
| manaSpent | number | mana used this turn | no |
| bonusMana | number | temporary bonus mana | no |
| hand | array | cards in hand | yes |
| deck | Deck | draw pile | yes |
| signatureCard | Card | bloodbound spell | no |
| isCurrentPlayer | boolean | turn state | no |
| hasReplacedThisTurn | boolean | replace tracking | no |

## Lifecycle Events
- created: GameSession start, start_challenge
- destroyed: game_over, resign
- modified: start_turn, end_turn, mana changes, draw/discard

## Dependencies
- Requires: GameSession, Deck, General (Unit), Hand
- Used by: Action, GameSession, Validator, AI

## Player Resources
| Resource | Max | Description |
|----------|-----|-------------|
| Mana | 9 | Increases each turn, resets to max |
| Hand | 6 | Maximum hand size |
| Deck | 40 | Starting deck size |
| Artifacts | 3 | Max equipped artifacts |

## Key Constants (from config.js)
| Constant | Value |
|----------|-------|
| STARTING_MANA | 2 |
| MAX_MANA | 9 |
| MAX_HAND_SIZE | 6 |
| MAX_DECK_SIZE | 40 |
| STARTING_HAND_SIZE | 5 |
| STARTING_HAND_REPLACE_COUNT | 2 |
| MAX_REPLACE_PER_TURN | 1 |

## Key Methods
- `drawCard` - Draw from deck to hand
- `getRemainingMana` - Get available mana
- `getIsCurrentPlayer` - Check if it's this player's turn
- `getGeneral` - Get the player's general unit
- `getDeck` - Get the player's deck
- `getHand` - Get cards in hand
- `getSignatureCard` - Get bloodbound spell

## Player Modifiers
| Modifier Type | Effect |
|---------------|--------|
| PlayerModifierManaModifier | Modify mana costs |
| PlayerModifierCardDrawModifier | Modify card draw |
| PlayerModifierBattlePetManager | Control battle pets |
| PlayerModifierSummonWatch | Track summons |
| PlayerModifierSpellWatch | Track spells |

## Description
Player represents a participant in a Duelyst match. Each player has resources (mana, hand, deck), a general on the board, and potentially equipped artifacts. The player's state tracks turn information, available actions, and win/loss conditions.
