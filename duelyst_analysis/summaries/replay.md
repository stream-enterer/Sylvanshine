# System: Replay & Serialization

**Location:** app/replay/, app/sdk/

## Purpose
The replay system enables game state persistence, playback, and network synchronization. All game state is serialized to JSON for Firebase storage and can be reconstructed for replays or reconnection.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| ReplayEngine | replay/replayEngine.coffee | Replay playback |
| GameSession | sdk/gameSession.coffee | State serialization |
| GameTurn | sdk/gameTurn.coffee | Turn container |
| Step | sdk/step.coffee | Action recording |
| ActionFactory | sdk/actions/actionFactory.coffee | Action deserialization |

## Data Flow
**Input:** Player actions during gameplay
**Processing:** Action → Step → Turn → GameSession → JSON
**Output:** Serialized game state in Firebase

## Serialization Hierarchy
```
GameSession
├── gameId, status, timestamps
├── players[2]
│   ├── hand, deck, mana
│   └── units on board
├── turns[]
│   └── steps[]
│       └── action
│           ├── type, index, owner
│           ├── source/target indices
│           └── sub-action indices
├── cardsByIndex
├── modifiersByIndex
└── board state
```

## Key Serialization Fields
| Class | Fields |
|-------|--------|
| GameSession | gameId, status, players, turns, cardsByIndex |
| GameTurn | playerId, steps, ended, timestamps |
| Step | action, index, timestamp, parentStepIndex |
| Action | type, sourceIndex, targetIndex, ownerId |

## Replay Flow
1. **Fetch** - Load game JSON from Firebase
2. **Parse** - Convert to JavaScript objects
3. **Deserialize** - Reconstruct full object graph
4. **Playback Loop**:
   - Execute steps sequentially
   - Apply timing delays
   - Synchronize UI events
   - Support speed modulation

## Serialization Methods
| Method | Purpose |
|--------|---------|
| serializeToJSON() | Export game state |
| deserializeSessionFromFirebase() | Full reconstruction |
| deserializeStepFromFirebase() | Reconstruct single step |
| addSignature() | Add timestamp/index to action |

## Snapshot System
| Type | Purpose |
|------|---------|
| gameSetupData | Initial state after setup |
| rollbackSnapshotData | State for followup rollback |

## Dependencies
**Requires:** ActionFactory, ModifierFactory, Firebase
**Used by:** Game persistence, replays, spectator mode

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.REPLAY_MAX_STEP_DELAY | 5.0s | Max step delay |
| CONFIG.REPLAY_MAX_STEP_DELAY_STARTING_HAND | 10.0s | Mulligan delay |

## Statistics
- **164 serialization fields** extracted
- **60+ action types** serializable
- **8 core classes** with serialization
- JSON format for Firebase storage
