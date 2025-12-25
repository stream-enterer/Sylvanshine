# System: Game Session

**Location:** app/sdk/gameSession.coffee

## Purpose
GameSession is the central singleton that manages all game state for a Duelyst match, coordinating players, board, cards, actions, and modifiers while serving as the authority for game logic.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| _GameSession | gameSession.coffee | Main singleton class (~140KB, largest file) |
| GameTurn | gameTurn.coffee | Turn container with step management |
| Step | step.coffee | Action execution unit |
| GameStatus | gameStatus.coffee | Game state enumeration (ACTIVE, OVER) |
| GameType | gameType.coffee | Match type definitions (Ranked, Casual, etc.) |
| GameFormat | gameFormat.coffee | Card pool rules (Standard, Legacy) |
| GameSetup | gameSetup.coffee | Initial game configuration |
| NetworkManager | networkManager.coffee | Network synchronization |

## Data Flow
**Input:** Player actions, network events, AI decisions
**Processing:** Action validation, execution, event propagation, modifier triggers
**Output:** Updated game state, events to UI/network

## Dependencies
**Requires:** Player, Board, Action system, Card system, Modifier system
**Used by:** All game entities, UI layers, Network manager, Replay engine

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.BOARDCOL | 9 | Board columns |
| CONFIG.BOARDROW | 5 | Board rows |
| CONFIG.CARD_DRAW_PER_TURN | 1 | Cards drawn per turn |
| CONFIG.FIRST_WIN_OF_DAY_GOLD_REWARD | 20 | Daily bonus gold |

## Game Types
| Type | Description |
|------|-------------|
| Ranked | Competitive ladder matches |
| Casual | Unranked matches |
| Gauntlet | Arena draft mode |
| Rift | Progressive deck building |
| Friendly | Challenge a friend |
| SinglePlayer | Practice/AI matches |
| BossBattle | Boss encounters |
| Challenge | Puzzle scenarios |

## Key Methods
- `executeAction(action)` - Execute and validate game actions
- `getBoard()` / `getPlayer1()` / `getPlayer2()` - State accessors
- `getCurrentPlayer()` / `getMyPlayer()` - Active player tracking
- `getCardByIndex(index)` - Card retrieval by index
- `serializeToJSON()` / `deserialize()` - State persistence

## Events
| Event | Description |
|-------|-------------|
| step | Each game step completes |
| start_turn | Player turn begins |
| end_turn | Player turn ends |
| action | Action executed |
| game_over | Game concludes |
| rollback_to_snapshot | State rollback |

## Data
See `instances/` TSV files for current counts of cards, actions, and modifiers.
