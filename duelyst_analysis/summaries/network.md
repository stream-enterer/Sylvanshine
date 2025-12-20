# System: Network & Multiplayer

**Location:** app/sdk/networkManager.coffee, app/common/session2.coffee

## Purpose
The Network system handles multiplayer game synchronization, matchmaking, spectating, and player sessions through Firebase and WebSocket connections.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| NetworkManager | sdk/networkManager.coffee | Game network sync |
| Session | common/session2.coffee | Player authentication |
| GamesManager | ui/managers/games_manager.js | Matchmaking UI |
| ReplayEngine | replay/replayEngine.coffee | Replay playback |

## Data Flow
**Input:** Player actions, server events, matchmaking requests
**Processing:** Action serialization → Network transmission → State sync
**Output:** Synchronized game state, lobby updates

## Dependencies
**Requires:** Firebase, WebSocket, Session API
**Used by:** GameSession, UI managers, Replay

## Network Events
| Event | Emitter | Purpose |
|-------|---------|---------|
| network_game_event | NetworkManager | Game action received |
| network_game_error | NetworkManager | Connection error |
| join_game | NetworkManager | Game joined |
| spectate_game | NetworkManager | Spectate started |
| spectator_joined | NetworkManager | Spectator connected |
| spectator_left | NetworkManager | Spectator left |
| reconnect_to_game | NetworkManager | Reconnection attempt |
| reconnect_failed | NetworkManager | Reconnection failed |
| game_server_shutdown | NetworkManager | Server closing |
| opponent_connection_status_changed | NetworkManager | Opponent online status |

## Matchmaking Events
| Event | Source | Purpose |
|-------|--------|---------|
| matchmaking_start | GamesManager | Queue entered |
| matchmaking_cancel | GamesManager | Queue left |
| matchmaking_error | GamesManager | Queue error |
| matchmaking_velocity | GamesManager | Queue time estimate |
| finding_game | GamesManager | Match search |
| invite_accepted | GamesManager | Friend match accepted |
| invite_rejected | GamesManager | Friend match declined |
| invite_cancelled | GamesManager | Friend invite cancelled |

## Session Events
| Event | Source | Purpose |
|-------|--------|---------|
| session_logged_in | Application | User authenticated |
| session_logged_out | Application | User logged out |

## Authoritative Execution
```
Client A: submitExplicitAction(action)
    ↓ Network
Server: validate + execute
    ↓ Broadcast
Client A/B: applyAuthoritativeAction(action)
```

## Spectating
- Observers can watch live games
- Spectators receive all game events
- No input allowed for spectators
- Multiple spectators supported

## Replay System
| Feature | Description |
|---------|-------------|
| Recording | All actions saved to replay |
| Playback | Step-through replay |
| Speed Control | Play/pause/resume |
| UI Events | Optional UI event replay |

## Configuration
| Constant | Purpose |
|----------|---------|
| CONFIG.ACTIVE_GAME_DELAY | 1.5 | Reconnect delay |
| CONFIG.INCOMING_MESSAGE_SFX_DELAY | 5.0 | Message sound cooldown |

## Network Actions
| Method | Purpose |
|--------|---------|
| submitExplicitAction | Send player action |
| executeAuthoritativeAction | Apply server action |
| getIsRunningAsAuthoritative | Check if server |
| scrubSensitiveData | Remove hidden info |

## Statistics
- Real-time multiplayer via WebSocket
- Firebase for authentication/persistence
- Supports 2-player games
- Unlimited spectators per game
- Replay recording and playback
- Reconnection handling
