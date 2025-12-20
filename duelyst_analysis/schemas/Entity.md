# Entity

## Source Evidence
- Primary class: `Entity` (app/sdk/entities/entity.coffee)
- Related classes: Unit, Tile
- Data shape: Cards that can exist on the game board

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| atk | number | inherited | yes |
| hp | number | inherited | yes |
| maxHP | number | inherited | yes |
| speed | number | inherited | no |
| reach | number | inherited | no |
| exhausted | boolean | inherited | no |
| position | {x,y} | board location | yes |
| ownerId | number | player association | yes |
| isRemoved | boolean | state tracking | no |
| isNew | boolean | state tracking | no |
| wasGeneral | boolean | state tracking | no |

## Lifecycle Events
- created: ApplyCardToBoardAction, onApplyToBoard
- destroyed: DieAction, RemoveAction, onRemoveFromBoard
- modified: TeleportAction, MoveAction, DamageAction, HealAction

## Dependencies
- Requires: Card, Board, Player, GameSession
- Used by: Unit, Tile, Action, Modifier, Spell

## Inheritance Hierarchy
```
Entity (Card)
├── Unit
│   └── General (via isGeneral flag)
└── Tile
    ├── Shadow Creep
    ├── Hallowed Ground
    ├── Primal Flourish
    └── ... (tile variants)
```

## Key Methods
- `onApplyToBoard` - Called when entity is placed on board
- `onRemoveFromBoard` - Called when entity is removed
- `getOwner` - Get owning player
- `getPosition` - Get board position
- `getAtk/getHP` - Get combat stats
- `canMove/canAttack` - Check action availability
- `setExhausted/getIsExhausted` - Exhaustion management

## Description
Entity extends Card to represent cards that physically exist on the game board. Entities have combat stats (ATK/HP), position, exhaustion state, and can be affected by board-level game rules. The two main subtypes are Unit (minions/generals) and Tile (persistent board effects).
