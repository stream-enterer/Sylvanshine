# Range

## Source Evidence
- Primary class: `Range` (app/sdk/entities/range.coffee)
- Related classes: Entity, MovementPatterns
- Data shape: Movement and attack range calculation

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| patternMap | object | cached patterns | no |
| patternByDistance | object | distance patterns | no |
| validPositions | array | calculated positions | no |

## Lifecycle Events
- created: Entity initialization
- destroyed: Entity removal
- modified: flushCachedState (state changes)

## Dependencies
- Requires: GameSession, Board, Entity
- Used by: Entity, Action validation, AI

## Range Types
| Type | Value | Description |
|------|-------|-------------|
| Melee | 1 | Adjacent tiles only |
| Ranged | BOARDCOL+BOARDROW | Full board range |

## Key Methods
- `constructor()` - Initialize range
- `getGameSession()` - Get game session
- `flushCachedState()` - Clear cached calculations
- `getValidPositions()` - Get reachable positions
- `getIsPositionValid(pos)` - Check if position reachable
- `getValidPosition()` - Get single valid position
- `getIsPositionInPatternMap(pos)` - Check pattern map
- `getPatternByDistance(dist)` - Get pattern at distance
- `getPatternMapByDistance(dist)` - Get pattern map
- `getPatternMapFromPattern(pattern)` - Convert pattern
- `_generateAndCachePatternAndMap()` - Build cache

## Movement Patterns (from config.js)
| Pattern | Description |
|---------|-------------|
| MOVE_PATTERN_STEP | Standard movement |
| RANGE_PATTERN_STEP | Standard attack range |
| SPAWN_PATTERN_STEP | Spawn positions |
| PATTERN_1SPACE | 1-tile radius |
| PATTERN_2SPACES | 2-tile radius |
| PATTERN_3SPACES | 3-tile radius |

## Speed Constants
| Speed | Value | Description |
|-------|-------|-------------|
| SPEED_BASE | 2 | Standard movement |
| SPEED_FAST | 3 | Enhanced movement |
| SPEED_INFINITE | 14 | Full board movement |

## Description
Range is a utility class that calculates and caches valid positions for movement and attacks. It uses patterns and distance calculations to determine where an entity can move or which tiles it can target. Range calculations respect board boundaries and obstacles.
