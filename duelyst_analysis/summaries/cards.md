# System: Cards

**Location:** app/sdk/cards/, app/sdk/entities/, app/sdk/artifacts/, app/sdk/spells/

## Purpose
The Card system defines all playable cards including units, spells, artifacts, and tiles, with support for factions, rarities, card sets, and complex card abilities through modifiers.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Card | cards/card.coffee | Base class for all cards |
| CardFactory | cards/cardFactory.coffee | Create cards by ID |
| Entity | entities/entity.coffee | Cards on board with position/stats |
| Unit | entities/unit.coffee | Minions and Generals |
| Tile | entities/tile.coffee | Persistent board effects |
| Artifact | artifacts/artifact.coffee | General equipment |
| Spell | spells/spell.coffee | One-time effect cards |
| Deck | cards/deck.coffee | Draw pile management |

## Data Flow
**Input:** Card definitions from factory, player actions
**Processing:** Card creation → Hand/Board placement → Modifier application → Effect resolution
**Output:** Board state changes, triggered abilities, visual representations

## Dependencies
**Requires:** GameSession, Modifier system, Action system
**Used by:** Player, Board, UI card nodes, AI evaluation

## Card Type Hierarchy
```
SDKObject
└── Card
    ├── Entity (board presence)
    │   ├── Unit (minions/generals)
    │   └── Tile (ground effects)
    ├── Spell (instant effects)
    └── Artifact (general equipment)
```

## Card Locations
| Location | Description |
|----------|-------------|
| Deck | Draw pile |
| Hand | Player's hand |
| SignatureCards | Bloodbound Spell slot |
| Board | On the game board |
| Void | Destroyed/removed |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.HAND_CARD_SIZE | 140.0 | Hand card display size |
| CONFIG.CARD_MARGIN | 75.0 | Card layout spacing |
| CONFIG.CARD_DRAW_PER_TURN | 1 | Cards drawn per turn |
| CONFIG.DECK_SIZE_INCLUDES_GENERAL | true | General counts in deck |

## Resources
| Resource Category | Count | Purpose |
|-------------------|-------|---------|
| Unit sprites | 40,285 bytes dir | Unit animations |
| Card backgrounds | 4,219 bytes dir | Rarity/faction backgrounds |
| Icons | 25,597 bytes dir | Card type icons |

## Statistics
- **1,116 cards** total in instances/cards.tsv
- **664 units** (minions + generals)
- **301 spells**
- **55 artifacts**
- **7 tiles**
- **9 card sets** (Core, Shimzar, Bloodborn, etc.)
- **7 rarities** (Basic, Common, Rare, Epic, Legendary, Token, Mythron)
- **8 races/tribes** (Golem, Mech, Arcanyst, etc.)

## Factions
| ID | Name | Generals |
|----|------|----------|
| 1 | Lyonar | Argeon, Ziran, Brome |
| 2 | Songhai | Kaleos, Reva, Shidai |
| 3 | Vetruvian | Zirix, Sajj, Ciphyron |
| 4 | Abyssian | Lilithe, Cassyva, Maehv |
| 5 | Magmar | Vaath, Starhorn, Ragnora |
| 6 | Vanar | Kara, Faie, Ilena |
| 100 | Neutral | - |

## Card Sets
| ID | Name | Release Order |
|----|------|---------------|
| 1 | Core | 1 |
| 2 | Denizens of Shimzar | 2 |
| 3 | Rise of the Bloodborn | 3 |
| 4 | Ancient Bonds | 4 |
| 5 | Unearthed Prophecy | 5 |
| 6 | Immortal Vanguard | 6 |
| 7 | Combined Unlockables | 7 |
| 8 | Trials of Mythron | 8 |

## Factory Structure
Card factories are organized by set and faction:
- `factory/core/faction1.coffee` through `faction6.coffee` + `neutral.coffee`
- `factory/shimzar/`, `factory/bloodstorm/`, etc.
- `factory/monthly/` for promotional cards
- `factory/misc/` for bosses, tutorials, tiles
