# System: Actions

**Location:** app/sdk/actions/

## Purpose
The Action system implements the Command pattern for all game state changes, providing a unified interface for executing, validating, and serializing game operations.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Action | action.coffee | Base class with source/target tracking |
| ActionFactory | actionFactory.coffee | Create actions by type |
| AttackAction | attackAction.coffee | Combat damage from unit attacks |
| DamageAction | damageAction.coffee | Apply damage with modifiers |
| HealAction | healAction.coffee | Restore HP to units |
| MoveAction | moveAction.coffee | Unit movement along path |
| PlayCardAction | playCardAction.coffee | Play card from hand |
| TeleportAction | teleportAction.coffee | Instant position change |
| ApplyModifierAction | applyModifierAction.coffee | Add modifiers to cards |
| DrawCardAction | drawCardAction.coffee | Draw from deck |

## Data Flow
**Input:** Player input, modifier triggers, AI decisions
**Processing:** Validation → Modification → Execution → Sub-actions → Events
**Output:** Modified game state, triggered modifiers, visual effects

## Dependencies
**Requires:** GameSession, Board, Card system, Modifier system
**Used by:** GameSession, Player, AI agents, Network

## Action Hierarchy
```
Action (SDKObject)
├── DamageAction
│   ├── AttackAction → ForcedAttackAction
│   ├── DamageAsAttackAction
│   ├── TrueDamageAction → HurtingDamageAction
│   └── RandomDamageAction
├── HealAction
├── MoveAction
├── TeleportAction → RandomTeleportAction, TeleportBehindUnit, TeleportInFrontOfUnit
├── ApplyCardToBoardAction
│   ├── PlayCardAction → PlayCardFromHandAction, PlaySignatureCardAction
│   └── PlayCardSilentlyAction → CloneEntityAction, RandomPlayCardSilently
├── RemoveAction → DieAction → ResignAction
├── ApplyModifierAction
├── RemoveModifierAction
├── PutCardInHandAction → DrawCardAction → BurnCardAction
├── StartTurnAction / EndTurnAction
└── [30+ more action types]
```

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.ACTION_DELAY | 0.5 | Default action animation delay |
| CONFIG.ENTITY_ATTACK_DURATION_MODIFIER | 1.0 | Attack speed multiplier |
| CONFIG.ENTITY_MOVE_DURATION_MODIFIER | 1.0 | Move speed multiplier |

## Resources
| Resource | Purpose |
|----------|---------|
| RSX.ATTACK_FX_* | Attack visual effects |
| RSX.FOLLOWUP_FX_* | Followup action effects |
| sfx_* | Sound effects per action type |

## Statistics
- **64 action types** extracted from codebase
- Action types by category:
  - Combat: 10 (Attack, Damage, Fight, Kill, etc.)
  - Movement: 6 (Move, Teleport variants)
  - Card Management: 15 (Draw, Play, Remove, Replace, etc.)
  - Modifier: 3 (Apply, Remove, Refresh)
  - Game Flow: 8 (StartTurn, EndTurn, Resign, etc.)
  - Resource: 6 (Mana, Artifact charges)
  - Other: 16 (Clone, Transform, Swap, etc.)

## Key Behaviors
- Actions can spawn sub-actions during execution
- Modifiers can intercept and modify actions via onBeforeAction/onAfterAction
- Actions track source/target for effect attribution
- Supports rollback through serialization
- Network-synchronized via authoritative execution
