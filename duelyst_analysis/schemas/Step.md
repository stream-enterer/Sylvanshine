# Step

## Source Evidence
- Primary class: `Step` (app/sdk/step.coffee)
- Related classes: SDKObject, Action, GameTurn
- Data shape: Container for an action and its sub-actions

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| action | Action | primary action | yes |
| playerId | string | acting player | yes |
| timestamp | number | execution time | yes |
| index | number | step index | yes |

## Lifecycle Events
- created: Player takes action
- destroyed: Step completes
- modified: Sub-actions added, resolution

## Dependencies
- Requires: Action, GameTurn, Player
- Used by: GameTurn, GameSession, Replay

## Step Structure
```
Step
├── Primary Action (player-initiated)
└── Sub-Actions (triggered effects)
    ├── Modifier triggers
    ├── Damage/Heal results
    └── Death/spawn events
```

## Step Events
| Event | When |
|-------|------|
| step | Step starts execution |
| start_step | Step begins |
| after_step | Step completes |
| before_show_step | UI about to show |
| after_show_step | UI finished showing |

## Action Resolution Order
1. Primary action queued
2. Before-action modifiers trigger
3. Action executes
4. After-action modifiers trigger
5. Sub-actions resolve (depth-first)
6. Cleanup phase

## Key Concepts
- **Explicit Action**: Player-initiated action (move, attack, play card)
- **Implicit Action**: System-triggered action (damage, triggers)
- **Sub-actions**: Actions caused by the primary action

## Replay Integration
Steps are the fundamental unit for:
- Game replay recording
- Network synchronization
- State rollback

## Description
Step represents a discrete game state change initiated by a player action. Each step contains the primary action and any resulting sub-actions from triggers and effects. Steps are recorded in GameTurn for replay and rollback support.
