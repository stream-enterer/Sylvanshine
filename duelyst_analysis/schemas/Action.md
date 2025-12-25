# Action

## Source Evidence
- Primary class: `Action` (app/sdk/actions/action.coffee)
- Related classes: DamageAction, MoveAction, AttackAction, TeleportAction, PlayCardAction, etc.
- Data shape: Command pattern for game state changes

## Fields
| Field | Type | Required? | Description |
|-------|------|-----------|-------------|
| type | string | yes | Action class identifier |
| ownerId | number | yes | Player ID who initiated the action |
| targetPosition | {x,y} | yes | Board position being affected |
| sourceIndex | number | no | Index of source card/entity |
| sourcePosition | {x,y} | no | Board position of source |
| targetIndex | number | no | Index of target card/entity |
| manaCost | number | no | Mana cost (for PlayCardAction) |
| timestamp | number | no | When action was executed |
| triggeringModifierIndex | number | no | Modifier that triggered this action |
| isDepthFirst | boolean | no | Execution order flag |

## Lifecycle Events
- created: before_added_action_to_queue
- destroyed: cleanup_action, after_cleanup_action
- modified: modify_action_for_validation, modify_action_for_execution

## Dependencies
- Requires: Player (ownerId), Card/Entity (source/target)
- Used by: GameSession, Step, Validator, Modifier

## Inheritance Hierarchy
```
Action (SDKObject)
├── ActivateSignatureCardAction
├── ApplyCardToBoardAction
│   ├── PlayCardAction
│   │   ├── PlayCardFromHandAction
│   │   └── PlaySignatureCardAction
│   └── PlayCardSilentlyAction
│       ├── CloneEntityAction
│       │   └── CloneEntityAsTransformAction
│       ├── PlayCardAsTransformAction
│       └── RandomPlayCardSilentlyAction
├── ApplyExhaustionAction
├── ApplyModifierAction
├── BonusManaAction
├── BonusManaCoreAction
├── DamageAction
│   ├── AttackAction
│   │   └── ForcedAttackAction
│   ├── DamageAsAttackAction
│   ├── RandomDamageAction
│   └── TrueDamageAction
│       └── HurtingDamageAction
├── DrawStartingHandAction
├── EndTurnAction
├── FightAction
├── GenerateSignatureCardAction
├── HealAction
├── KillAction
├── MoveAction
├── PutCardInDeckAction
├── PutCardInHandAction
│   ├── DrawCardAction
│   │   └── BurnCardAction
│   └── ReplaceCardFromHandAction
├── RefreshArtifactChargesAction
├── RefreshExhaustionAction
├── RemoveAction
│   └── DieAction
│       └── ResignAction
├── RemoveArtifactsAction
├── RemoveCardFromDeckAction
├── RemoveCardFromHandAction
├── RemoveManaCoreAction
├── RemoveModifierAction
├── RemoveRandomArtifactAction
├── RestoreChargeToAllArtifactsAction
├── RestoreManaAction
├── RevealHiddenCardAction
├── RollbackToSnapshotAction
├── SetDamageAction
├── SetExhaustionAction
├── StartTurnAction
├── StopBufferingEventsAction
│   └── EndFollowupAction
├── SwapGeneralAction
├── SwapUnitAllegianceAction
├── SwapUnitsAction
├── TakeAnotherTurnAction
├── TeleportAction
│   ├── RandomTeleportAction
│   ├── TeleportBehindUnitAction
│   └── TeleportInFrontOfUnitAction
└── UpdateSignatureCardAction
```

## Description
Actions are the command pattern implementation for all game state changes. Every modification to game state must go through an Action to ensure proper validation, event triggering, and replay support.
