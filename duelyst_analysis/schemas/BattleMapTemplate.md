# BattleMapTemplate

## Source Evidence
- Primary class: `BattleMapTemplate` (app/sdk/battleMapTemplate.coffee)
- Related classes: SDKObject, Board, Tile
- Data shape: Battle map configuration

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | number | map identification | yes |
| name | string | display name | yes |
| columns | number | board width | yes |
| rows | number | board height | yes |
| tiles | array | tile positions | no |
| manaPositions | array | mana tile spawns | no |

## Lifecycle Events
- created: Map selection
- destroyed: Game end
- modified: None (read-only template)

## Dependencies
- Requires: SDKObject
- Used by: GameSession, Board initialization

## Battle Maps
| ID | Name | Theme |
|----|------|-------|
| 0 | Standard | Default battlefield |
| 1 | Abyssian | Purple/dark theme |
| 2 | Vanar | Ice/snow theme |
| 3 | Redrock | Desert/red theme |
| 4 | Shimzar | Jungle theme |

## Map Resources (RSX constants)
| Resource Type | Example |
|---------------|---------|
| Background | battlemap0_background |
| Middleground | battlemap0_middleground |
| Foreground | battlemap0_foreground_001 |
| Particles | battlemap_abyssian_river_bubble_particles |

## Mana Tile Spawning
- Mana tiles spawn at predefined positions
- Usually symmetric for fair gameplay
- Standard positions: center column tiles

## Key Methods
- `getId()` - Get map ID
- `getName()` - Get map name
- `getColumns()` - Get board width
- `getRows()` - Get board height
- `getTiles()` - Get initial tile data
- `getManaPositions()` - Get mana spawn points

## Description
BattleMapTemplate defines the visual and gameplay configuration for battle maps. It specifies board dimensions, tile placement, mana tile spawn locations, and visual resources. Maps are selected at random or based on game mode.
