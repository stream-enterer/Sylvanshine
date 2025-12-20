# GameTurn

## Source Evidence
- Primary class: `GameTurn` (app/sdk/gameTurn.coffee)
- Related classes: SDKObject, Step, GameSession
- Data shape: Container for turn state and steps

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| playerId | string | classes.tsv | yes |
| steps | array[Step] | classes.tsv | yes |
| ended | boolean | classes.tsv | no |
| turnIndex | number | turn counter | yes |

## Lifecycle Events
- created: StartTurnAction, start_turn
- destroyed: EndTurnAction, end_turn
- modified: step additions, action execution

## Dependencies
- Requires: GameSession, Player, Step
- Used by: GameSession, Action, UI

## Turn Structure
1. StartTurnAction executes
2. Player gains mana
3. Cards ready to act
4. Player takes actions (steps)
5. EndTurnAction executes
6. End-of-turn effects resolve
7. Next turn begins

## Key Methods
- `constructor()` - Initialize turn
- `setPlayerId(id)` - Set active player
- `getPlayerId()` - Get active player
- `getSteps()` - Get all steps this turn
- `addStep(step)` - Add step to turn
- `setEnded(bool)` - Mark turn as ended
- `getEnded()` - Check if turn ended
- `deserialize(data)` - Load turn state

## Turn Constants (from config.js)
| Constant | Value | Description |
|----------|-------|-------------|
| TURN_DURATION | 90.0 | Seconds per turn |
| TURN_DURATION_INACTIVE | 15.0 | Inactive timeout |
| TURN_DELAY | 0.0 | Delay between turns |
| TURN_TIME_SHOW | 20.0 | Show timer at |

## Turn Events
| Event | Trigger |
|-------|---------|
| start_turn | Turn begins |
| end_turn | Turn ends |
| show_start_turn | UI notification |
| show_end_turn | UI notification |
| turn_time | Timer updates |

## Description
GameTurn represents a single player's turn in a Duelyst match. It contains all the Steps (player actions) taken during that turn. Turns alternate between players, with each turn having a time limit and consisting of multiple possible actions.
