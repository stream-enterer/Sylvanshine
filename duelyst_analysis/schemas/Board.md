# Board

## Source Evidence
- Primary class: `Board` (app/sdk/board.coffee)
- Related classes: SDKObject, Entity, Tile
- Data shape: Game board grid and entity positions

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| cardIndices | array | data_shapes.tsv:749 | yes |
| columnCount | number | CONFIG.BOARDCOL (9) | yes |
| rowCount | number | CONFIG.BOARDROW (5) | yes |
| tiles | 2D array | board positions | yes |

## Lifecycle Events
- created: GameSession initialization
- destroyed: game_over
- modified: entity placement/removal, tile changes

## Dependencies
- Requires: GameSession, Entity, Tile
- Used by: Action, Player, Spell, AI, Pathfinding

## Board Dimensions (from config.js)
| Constant | Value | Description |
|----------|-------|-------------|
| BOARDCOL | 9 | Columns (x-axis) |
| BOARDROW | 5 | Rows (y-axis) |
| TILESIZE | 95 | Pixel size of tiles |

## Position System
- Positions are `{x, y}` objects
- x: 0 (left) to 8 (right)
- y: 0 (bottom) to 4 (top)
- Player 1 starts on left (x=0-2)
- Player 2 starts on right (x=6-8)

## Key Methods
- `getUnitAtPosition(pos)` - Get unit at position
- `getTileAtPosition(pos)` - Get tile at position
- `getCardAtPosition(pos)` - Get any card at position
- `getEntitiesAroundPosition(pos)` - Get adjacent entities
- `getEntitiesInRow(y)` - Get entities in row
- `getEntitiesInColumn(x)` - Get entities in column
- `getUnitsForPlayer(playerId)` - Get player's units
- `isValidPosition(pos)` - Check if position is on board
- `isPositionOccupied(pos)` - Check if position has entity

## Movement Patterns (from config.js)
| Pattern | Description |
|---------|-------------|
| MOVE_PATTERN_STEP | Valid movement steps |
| RANGE_PATTERN_STEP | Attack range pattern |
| SPAWN_PATTERN_STEP | Valid spawn positions |
| PATTERN_1SPACE | 1 space in all directions |
| PATTERN_2SPACES | 2 spaces in all directions |
| PATTERN_3SPACES | 3 spaces in all directions |
| PATTERN_BLAST | Full row pattern |
| PATTERN_WHOLE_BOARD | All board positions |

## Description
Board represents the 9x5 game grid where entities exist and actions take place. It tracks all entities on the board, provides position queries, and supports pathfinding for movement. The board is the spatial container for all gameplay interactions.
