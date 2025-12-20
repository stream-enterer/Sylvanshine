# Tile

## Source Evidence
- Primary class: `Tile` (app/sdk/entities/tile.coffee)
- Related classes: Entity, various Tile subclasses
- Data shape: Persistent board effects that occupy spaces

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| occupant | Entity | classes.tsv | no |
| depleted | boolean | classes.tsv | no |
| dieOnDepleted | boolean | classes.tsv | no |
| canBeAppliedAnywhere | boolean | classes.tsv | no |
| obstructsOtherTiles | boolean | classes.tsv | no |

## Lifecycle Events
- created: SpellSpawnEntity, onApplyToBoard
- destroyed: depleted, replaced, dispelled
- modified: occupant changes, modifiers applied

## Dependencies
- Requires: Entity, Board, Player
- Used by: Spell, Modifier, Unit (occupant)

## Tile Types
| Tile | Effect | Faction |
|------|--------|---------|
| Shadow Creep | Damages enemies | Abyssian |
| Hallowed Ground | Heals allies | Lyonar |
| Primal Flourish | Buffs summoned minions | Vanar |
| Exhuming Sand | Spawns dervishes | Vetruvian |
| Mana Tile | Grants mana crystal | Neutral |
| Bonechill Barrier | Wall tile | Vanar |

## Key Methods
- `getOccupant` - Get entity standing on this tile
- `setOccupant` - Set occupying entity
- `getDepleted` - Check if tile is used up
- `setDepleted` - Mark tile as depleted
- `silence` - Remove tile effects

## Description
Tiles are a special type of Entity that exist on the board as persistent area effects. Unlike Units, Tiles typically cannot move or attack but instead provide ongoing effects to entities that stand on them or end their turn on them. Tiles can have an occupant (a Unit standing on them) and may deplete after use.
