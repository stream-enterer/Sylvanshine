# Action

## Source Evidence
- Primary class: `Action` (app/sdk/actions/action.coffee)
- Related classes: DamageAction, MoveAction, AttackAction, TeleportAction, PlayCardAction, etc.
- Data shape: Command pattern for game state changes

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | data_shapes.tsv:604 | yes |
| changedByModifierIndices | array | data_shapes.tsv:605 | no |
| isDepthFirst | boolean | data_shapes.tsv:606 | no |
| manaCost | number | data_shapes.tsv:607 | no |
| ownerId | number | data_shapes.tsv:608 | yes |
| sourceIndex | number | data_shapes.tsv:609 | no |
| sourcePosition | {x,y} | data_shapes.tsv:610 | no |
| targetIndex | number | data_shapes.tsv:611 | no |
| targetPosition | {x,y} | data_shapes.tsv:612 | yes |
| timestamp | number | data_shapes.tsv:613 | no |
| triggeringModifierIndex | number | data_shapes.tsv:614 | no |

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
