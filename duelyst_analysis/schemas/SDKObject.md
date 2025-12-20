# SDKObject

## Source Evidence
- Primary class: `SDKObject` (app/sdk/sdkObject.coffee)
- Related classes: Base class for all SDK entities
- Data shape: Serializable object with index and type properties

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| _cachedStateRecording | object | data_shapes.tsv | no |
| _eventBus | EventBus | classes.tsv:SDKObject | no |
| index | number | serialization | yes |
| type | string | serialization | yes |

## Lifecycle Events
- created: constructor
- destroyed: terminate
- modified: deserialize, before_deserialize

## Dependencies
- Requires: None (base class)
- Used by: Action, Board, Card, Deck, GameTurn, Modifier, Player, Step, BattleMapTemplate

## Description
SDKObject is the base class for all game SDK objects in Duelyst. It provides:
- Event bus for inter-object communication
- Serialization/deserialization support via index and type properties
- State recording for game replay
- Game session association via getGameSession()

## Key Methods (from classes.tsv)
- `constructor` - Initialize the object
- `getGameSession` - Get associated game session
- `deserialize` - Reconstruct from serialized data
- `terminate` - Clean up the object
