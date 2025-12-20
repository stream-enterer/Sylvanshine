# Validator

## Source Evidence
- Primary class: `Validator` (app/sdk/validators/validator.coffee)
- Related classes: 7 validator subclasses
- Data shape: Action validation system

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | validator identification | yes |
| gameSession | GameSession | reference | yes |
| eventBus | EventBus | event subscription | yes |

## Lifecycle Events
- created: GameSession initialization
- destroyed: _onTerminate, game end
- modified: validate_action, modify_action_for_validation

## Dependencies
- Requires: GameSession, Action, EventBus
- Used by: GameSession (action execution)

## Validator Types
| Validator | Purpose |
|-----------|---------|
| Validator | Base class |
| ValidatorApplyCardToBoard | Validate card placement |
| ValidatorEntityAction | Validate entity actions |
| ValidatorExecuteExplicitAction | Validate player actions |
| ValidatorFollowup | Manage followup actions |
| ValidatorPlayCard | Validate card plays |
| ValidatorReplaceCardFromHand | Validate card replacement |
| ValidatorScheduledForRemoval | Track removal state |

## Validation Events
| Event | When |
|-------|------|
| modify_action_for_validation | Before validation |
| validate_action | Action being validated |
| invalid_action | Action failed validation |

## Key Methods
- `constructor()` - Initialize validator
- `onEvent(event)` - Handle game events
- `getGameSession()` - Get game session
- `getType()` - Get validator type
- `invalidateAction(action, reason)` - Mark action invalid
- `_onTerminate()` - Cleanup on session end
- `onValidateAction(action)` - Validate specific action

## Validation Flow
```
1. Action created
2. modify_action_for_validation event
3. Each validator.onValidateAction(action)
4. If invalid: invalid_action event, action rejected
5. If valid: action proceeds to execution
```

## Followup Validation (ValidatorFollowup)
- Tracks cards waiting for followup actions
- Validates followup action sequences
- Clears followup state on completion

## Description
Validators ensure game actions are legal before execution. Each validator checks specific rules (mana cost, valid targets, turn state, etc.) and can invalidate actions that break game rules. The validation system is extensible and event-driven.
