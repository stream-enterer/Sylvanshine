# Card

## Source Evidence
- Primary class: `Card` (app/sdk/cards/card.coffee)
- Related classes: Entity, Unit, Tile, Spell, Artifact
- Data shape: Base class for all cards in the game

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | data_shapes.tsv:750 | yes |
| name | string | data_shapes.tsv:751 | yes |
| manaCost | number | data_shapes.tsv:752 | yes |
| modifierIndices | array | data_shapes.tsv:753 | no |
| modifiers | array | data_shapes.tsv:756 | no |
| numInherent | number | data_shapes.tsv:757 | no |
| numManaged | number | data_shapes.tsv:758 | no |
| numActive | number | data_shapes.tsv:759 | no |
| writable | boolean | data_shapes.tsv:754-755 | no |
| id | number | CardFactory | yes |
| factionId | number | CardFactory | yes |
| raceId | number | CardFactory | no |
| rarityId | number | CardFactory | yes |
| baseCardId | number | CardFactory | no |
| cardSetId | number | CardFactory | no |

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
