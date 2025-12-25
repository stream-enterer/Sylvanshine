# Card

## Source Evidence
- Primary class: `Card` (app/sdk/cards/card.coffee)
- Related classes: Entity, Unit, Tile, Spell, Artifact
- Data shape: Base class for all cards in the game

## Fields
| Field | Type | Required? |
|-------|------|-----------|
| id | number | yes |
| type | string | yes |
| name | string | yes |
| manaCost | number | yes |
| factionId | number | yes |
| rarityId | number | yes |
| raceId | number | no |
| baseCardId | number | no |
| cardSetId | number | no |
| modifiers | array | no |
| modifierIndices | array | no |

## Lifecycle Events
- created: play_card_start, before_added_action_to_queue
- destroyed: play_card_stop, removed from board
- modified: modifier_active_change, apply/remove modifiers

## Dependencies
- Requires: CardFactory, GameSession, Player
- Used by: Entity, Spell, Artifact, Deck, Hand

## Card Types (from CardType)
| Type | Value | Description |
|------|-------|-------------|
| Card | 0 | Base card type |
| Entity | 1 | On-board entities |
| Unit | 2 | Minions and Generals |
| Spell | 3 | Spell cards |
| Tile | 4 | Board tiles |
| Artifact | 5 | Equipment |

## Card Locations (from CardLocation)
| Location | Value | Description |
|----------|-------|-------------|
| Deck | 0 | In player's deck |
| Hand | 1 | In player's hand |
| Board | 2 | On the game board |
| SignatureCards | 3 | Bloodbound spell |
| Void | 4 | Removed from game |

## Inheritance Hierarchy
```
Card (SDKObject)
├── Artifact
├── Entity
│   ├── Unit
│   └── Tile
└── Spell
    ├── SpellDamage
    ├── SpellHeal
    ├── SpellApplyModifiers
    ├── SpellSpawnEntity
    ├── SpellKillTarget
    └── ... (200+ spell variants)
```

## Description
Card is the base class for all playable cards in Duelyst. It provides core card functionality including mana cost, faction association, modifiers, and serialization. Cards exist in different locations (deck, hand, board, void) and have associated rarities and card sets.
