# AgentAction

## Source Evidence
- Primary class: `app/sdk/agents/agentActions.coffee`
- Agent system: `app/sdk/agents/`
- Usage: Challenge files in `app/sdk/challenges/`

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| name | string | Method name | yes |
| type | enum | action_type/action_usage | yes |
| params | string | Method parameters | yes |
| source | string | Source file | yes |
| turn | number | Turn index (for usage) | no |

## Agent Classes
| Class | Purpose | File |
|-------|---------|------|
| BaseAgent | Abstract base | baseAgent.coffee |
| StaticAgent | Scripted sequences | staticAgent.coffee |
| AgentActions | Action factory | agentActions.coffee |

## Hard Actions (Game State)
| Action | Parameters | Purpose |
|--------|------------|---------|
| createAgentActionMoveUnit | unitTag, deltaXY | Move unit by delta |
| createAgentActionAttackWithUnit | unitTag, targetPosition | Attack target |
| createAgentActionPlayCard | handIndex, targetPosition | Play card from hand |
| createAgentActionPlayCardFindPosition | handIndex, positionFilter | Dynamic card placement |
| createAgentActionPlayFollowup | followupId, sourcePos, targetPos | Followup cards |

## Soft Actions (Non-Game)
| Action | Parameters | Purpose |
|--------|------------|---------|
| createAgentSoftActionTagUnitAtPosition | unitTag, position | Mark unit for reference |
| createAgentSoftActionShowInstructionLabels | labels[] | Tutorial hints |

## Usage Pattern
```coffeescript
# Turn-based action scheduling
@_opponentAgent.addActionForTurn(0,
  AgentActions.createAgentActionMoveUnit("general", {x:1, y:-1}))
@_opponentAgent.addActionForTurn(0,
  AgentActions.createAgentActionAttackWithUnit("general", targetPos))
```

## Unit Tagging
```coffeescript
# Tag units for later reference
@_opponentAgent.addUnitWithTag(generalUnit, "general")
@_opponentAgent.addUnitWithTag(minionUnit, "minion1")

# Reference by tag in actions
createAgentActionMoveUnit("general", delta)
```

## Architecture Notes
- **Fully scripted** - No dynamic decision making
- **Turn-indexed** - Actions grouped by turn number
- **Position-based** - Targets specified as {x, y} coordinates
- **Tag system** - Units referenced by string tags

## Dependencies
- Requires: GameSession, Challenge system
- Used by: Tutorials, puzzles, challenges

## Description
AgentActions provide a factory for creating scripted opponent behaviors in challenges and tutorials. The system is entirely pre-scripted with no dynamic evaluation or AI decision-making. Actions are scheduled by turn index and executed sequentially.

## Statistics
- **13 agent actions** extracted
- **3 agent classes** in SDK
- **51+ challenges** use StaticAgent
- No minimax, MCTS, or neural networks
