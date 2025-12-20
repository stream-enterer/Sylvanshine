# GameSession

## Source Evidence
- Primary class: `GameSession` (app/sdk/gameSession.coffee)
- Related classes: Singleton pattern, manages all game state
- Data shape: Central game state container

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| gameId | string | classes.tsv | yes |
| players | array[Player] | classes.tsv | yes |
| board | Board | classes.tsv | yes |
| currentTurn | GameTurn | classes.tsv | yes |
| turns | array[GameTurn] | classes.tsv | yes |
| cardCaches | object | data_shapes.tsv | yes |
| status | GameStatus | classes.tsv | yes |
| gameFormat | GameFormat | classes.tsv | yes |
| gameType | GameType | classes.tsv | yes |
| isOver | boolean | classes.tsv | no |
| winnerId | string | classes.tsv | no |

## Lifecycle Events
- created: start_challenge, matchmaking complete
- destroyed: game_over, resign
- modified: step, action, start_turn, end_turn

## Dependencies
- Requires: Player, Board, Card system, Action system
- Used by: All game entities, UI, Network

## Singleton Pattern
```javascript
GameSession.create()      // Create new instance
GameSession.getInstance() // Get current instance
GameSession.current()     // Alias for getInstance
GameSession.reset()       // Clear current instance
```

## Game Types (from GameType)
| Type | Description |
|------|-------------|
| Ranked | Competitive ladder matches |
| Casual | Unranked matches |
| Gauntlet | Arena mode |
| Rift | Rift mode |
| Friendly | Challenge a friend |
| SinglePlayer | Practice/AI matches |
| BossBattle | Boss encounters |
| Challenge | Puzzle challenges |

## Game Formats (from GameFormat)
| Format | Description |
|--------|-------------|
| Standard | Current card pool |
| Legacy | All cards allowed |

## Key Methods
- `executeAction(action)` - Execute a game action
- `getBoard()` - Get game board
- `getPlayer1/getPlayer2()` - Get players
- `getCurrentPlayer()` - Get active player
- `getMyPlayer()` - Get local player
- `getCardByIndex(index)` - Retrieve card by index
- `getModifierByIndex(index)` - Retrieve modifier
- `getIsRunningAsAuthoritative()` - Server authority check
- `serializeToJSON()` - Save game state
- `deserialize(data)` - Load game state

## Events Emitted
| Event | When |
|-------|------|
| step | Each game step |
| start_turn | Turn begins |
| end_turn | Turn ends |
| action | Action executed |
| game_over | Game ends |
| rollback_to_snapshot | State rollback |

## Description
GameSession is the central singleton that manages all game state for a Duelyst match. It contains references to players, board, cards, modifiers, and actions. All game logic flows through the session, making it the authority for the game's current state. It supports serialization for replays and network synchronization.
