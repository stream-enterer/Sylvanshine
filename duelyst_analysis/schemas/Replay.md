# Replay (Serialization Format)

## Source Evidence
- Replay engine: `app/replay/replayEngine.coffee`
- Serialization: `app/sdk/gameSession.coffee` (lines 3400-3606)
- Turn structure: `app/sdk/gameTurn.coffee`
- Step structure: `app/sdk/step.coffee`

## GameSession Serialization
| Field | Type | Purpose |
|-------|------|---------|
| gameId | string | Unique game identifier |
| gameType | string | Game mode type |
| gameFormat | string | Format (standard, etc.) |
| status | enum | new/active/over |
| createdAt | number | Creation timestamp |
| updatedAt | number | Last update timestamp |
| players | Player[] | Both player states |
| turns | GameTurn[] | Completed turns |
| currentTurn | GameTurn | Active turn |
| cardsByIndex | object | All cards by index |
| modifiersByIndex | object | Active modifiers |
| board | object | Board grid state |
| battleMapTemplate | object | Map configuration |
| gameSetupData | object | Initial state snapshot |

## GameTurn Structure
| Field | Type | Purpose |
|-------|------|---------|
| playerId | string | Active player ("1" or "2") |
| steps | Step[] | Actions in turn |
| createdAt | number | Turn start time |
| updatedAt | number | Turn last update |
| ended | boolean | Turn completion flag |

## Step Structure
| Field | Type | Purpose |
|-------|------|---------|
| action | Action | The action object |
| playerId | string | Who performed it |
| timestamp | number | Execution time (ms) |
| index | number | Sequence order |
| parentStepIndex | number | Parent step reference |
| childStepIndex | number | Child step reference |
| transmitted | boolean | Network transmission status |
| includedRandomness | boolean | Contains RNG |

## Action Serialization
| Field | Type | Purpose |
|-------|------|---------|
| type | string | Action class name |
| index | number | Unique execution order |
| ownerId | string | Player ID |
| sourceIndex | number | Source card index |
| targetIndex | number | Target card index |
| sourcePosition | {x,y} | Source position |
| targetPosition | {x,y} | Target position |
| parentActionIndex | number | Parent action |
| resolveSubActionIndices | number[] | Child actions |
| timestamp | number | Execution time |
| manaCost | number | Action mana cost |
| isAutomatic | boolean | Auto-triggered flag |
| changedByModifierIndices | number[] | Modifying modifiers |

## Serialization Methods
| Method | Direction | Purpose |
|--------|-----------|---------|
| serializeToJSON | object → JSON | Export game state |
| deserializeSessionFromFirebase | JSON → object | Import game state |
| deserializeStepFromFirebase | JSON → Step | Reconstruct step |
| addSignature | Action → Action | Add timestamp/index |

## Snapshot System
| Snapshot | Purpose |
|----------|---------|
| gameSetupData | Initial state after setup |
| rollbackSnapshotData | State during followups |

## Replay Flow
```
1. Fetch game session JSON from Firebase
2. Deserialize to JavaScript objects
3. Reconstruct object graph with factories
4. Replay loop:
   - Track current turn/step index
   - Apply timing delays
   - Execute steps via executeAuthoritativeStep()
   - Synchronize UI events
```

## Dependencies
- Requires: ActionFactory, ModifierFactory
- Used by: Replay viewer, spectator mode, game history

## Description
The replay/serialization system enables game state persistence and playback. All game state is serialized to JSON for storage in Firebase. Replays reconstruct the full object graph using factory patterns and execute steps sequentially with timing synchronization.

## Statistics
- **164 serialization fields** extracted
- **60+ action types** serializable
- **8 core classes** with serialization
- JSON format for Firebase storage
