# System: AI Agents

**Location:** app/sdk/agents/

## Purpose
The AI Agent system provides computer-controlled opponents for practice games and tutorials through scripted and evaluated action sequences.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| BaseAgent | agents/baseAgent.coffee | Agent base class |
| StaticAgent | agents/staticAgent.coffee | Scripted turn sequences |
| AgentActions | agents/agentActions.coffee | Action creation helpers |

## Data Flow
**Input:** Game state after player turn
**Processing:** State evaluation → Action sequence generation → Execution
**Output:** AI move sequence (move, attack, play cards)

## Dependencies
**Requires:** GameSession, Card system, Action system
**Used by:** Single player modes, tutorials, challenges

## Agent Types
| Type | Behavior | Use Case |
|------|----------|----------|
| BaseAgent | Foundation class | Inheritance base |
| StaticAgent | Pre-defined moves | Tutorials, puzzles |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.AI_PLAYER_ID | 'ai' | AI player identifier |

## Agent Actions
| Action Type | Method | Purpose |
|-------------|--------|---------|
| Move Unit | createAgentActionMoveUnit | Move to position |
| Attack | createAgentActionAttackWithUnit | Attack target |
| Play Card | createAgentActionPlayCard | Play from hand |
| Play Followup | createAgentActionPlayFollowup | Follow-up cards |
| Tag Unit | createAgentSoftActionTagUnitAtPosition | Mark for reference |
| Show Instructions | createAgentSoftActionShowInstructionLabels | Tutorial hints |

## Agent Action Structure
```javascript
{
  type: 'move' | 'attack' | 'play_card' | 'followup',
  unitTag: 'tag_name',      // Unit identifier
  targetPosition: {x, y},   // Destination
  cardIndex: 0,             // Hand position
  followupIndex: 0          // Followup choice
}
```

## StaticAgent Configuration
```coffeescript
agent = new StaticAgent()
agent.addUnitWithTag(unitIndex, 'my_general')
agent.addActionForTurn 1, [
  AgentActions.createAgentActionMoveUnit('my_general', {x: 3, y: 2})
  AgentActions.createAgentActionAttackWithUnit('my_general', targetPosition)
]
```

## Soft Actions
Non-game-state actions for tutorials:
| Soft Action | Purpose |
|-------------|---------|
| TagUnitAtPosition | Name a unit for reference |
| ShowInstructionLabels | Display tutorial UI |

## Statistics
- **3 agent classes** in instances/agents.tsv
- BaseAgent: Foundation for AI
- StaticAgent: Scripted sequences
- AgentActions: Helper utilities
- Used in 51+ challenges
- Supports tutorial instruction display
