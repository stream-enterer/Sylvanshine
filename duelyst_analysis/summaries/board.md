# System: Board

**Location:** app/sdk/board.coffee, app/sdk/battleMapTemplate.coffee

## Purpose
The Board system manages the 9x5 game grid including unit positioning, movement calculations, adjacency queries, and spawn point validation.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Board | board.coffee | 9x5 grid management |
| BattleMapTemplate | battleMapTemplate.coffee | Map visual configuration |
| MovementRange | entities/movementRange.coffee | Movement pathfinding |
| AttackRange | entities/attackRange.coffee | Attack range calculation |

## Data Flow
**Input:** Card placements, movement actions, board queries
**Processing:** Position validation → Entity management → Range calculation
**Output:** Valid positions, adjacency results, path data

## Dependencies
**Requires:** GameSession, Entity system
**Used by:** Actions, Modifiers, AI, View system

## Board Dimensions
| Property | Value | Description |
|----------|-------|-------------|
| Columns | 9 | X-axis positions (0-8) |
| Rows | 5 | Y-axis positions (0-4) |
| Total Tiles | 45 | 9 × 5 grid |
| Player 1 Start | Left side | Columns 0-1 |
| Player 2 Start | Right side | Columns 7-8 |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.BOARDCOL | 9 | Column count |
| CONFIG.BOARDROW | 5 | Row count |
| CONFIG.BOARDCENTER | {x:4, y:2} | Center position |
| CONFIG.ALL_BOARD_POSITIONS | Array | All 45 positions |
| CONFIG.BATTLEMAP_DEFAULT_INDICES | [0-6] | Default maps |

## Board Methods
| Method | Purpose |
|--------|---------|
| getUnits() | All units on board |
| getEntities() | All entities (units + tiles) |
| getCardAtPosition(pos) | Entity at specific tile |
| getUnobstructedPositions() | Empty tiles |
| getCardsAroundPosition(pos) | Adjacent entities |
| getEnemyEntitiesForEntity(entity) | All enemies |
| getFriendlyEntitiesForEntity(entity) | All allies |
| getValidSpawnPositions(entity) | Legal placement tiles |
| getEntitiesInColumn(x) | Column query |
| getEntitiesInRow(y) | Row query |
| getEntitiesInfrontOf(entity) | Directional query |

## Battle Maps
| ID | Name | Theme |
|----|------|-------|
| 0-6 | Standard | Default maps |
| 7 | Battlemap7 | Alternative |
| 8 | Shimzar | Denizens set |
| 9 | Abyssian | Dark theme |
| 10 | Redrock | Desert theme |
| 11 | Vanar | Ice theme |

## Resources
| Resource | Purpose |
|----------|---------|
| RSX.battlemap*_background | Parallax background |
| RSX.battlemap*_middleground | Middle layer |
| RSX.battlemap*_foreground | Front layer |
| resources/maps/ | Map assets (1,907 bytes) |

## Position Patterns
Common patterns used for targeting:
| Pattern | Tiles | Used For |
|---------|-------|----------|
| Adjacent | 4 | Melee range |
| Diagonal | 4 | Some abilities |
| All Adjacent | 8 | Full surround |
| Row | 9 | Blast attacks |
| Column | 5 | Some spells |
| Cross | 4+ | Ranged patterns |

## Statistics
- 45 total board positions
- 2 general spawn zones
- 11+ map themes
- Supports flying (any tile), ranged (long distance)
- Path calculation for movement
- Obstruction checking for placement
