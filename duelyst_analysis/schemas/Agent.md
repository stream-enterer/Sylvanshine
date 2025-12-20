# Agent

## Source Evidence
- Primary class: `BaseAgent` (app/sdk/agents/agent.coffee)
- Related classes: StaticAgent, AI system
- Data shape: AI decision-making controller

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| playerId | string | controlled player | yes |
| gameSession | GameSession | reference | yes |
| isActive | boolean | agent running | yes |
| difficulty | number | AI difficulty | no |

## Lifecycle Events
- created: Single-player game start, challenge start
- destroyed: Game end
- modified: Turn changes, action execution

## Dependencies
- Requires: GameSession, Player, Board
- Used by: Challenge, Practice mode, Boss battles

## Agent Types
| Type | Description |
|------|-------------|
| BaseAgent | Base agent class |
| StaticAgent | Scripted behavior for challenges |
| AI Agent | Dynamic decision-making |

## AI Evaluation Factors
| Factor | Weight | Description |
|--------|--------|-------------|
| Board Control | High | Unit positioning |
| Card Advantage | Medium | Hand/deck size |
| Life Total | High | General health |
| Threat Assessment | High | Enemy danger |
| Resource Efficiency | Medium | Mana usage |

## Key Methods
- `onTurnStart()` - Begin AI turn
- `onTurnEnd()` - End AI turn
- `evaluateBoard()` - Assess game state
- `generateMoves()` - Create move options
- `selectBestMove()` - Choose optimal action
- `executeMove(action)` - Perform action

## Agent Behavior in Challenges
- StaticAgent follows scripted sequences
- Responds to specific game states
- Designed for puzzle solutions

## AI Difficulty Levels
| Level | Behavior |
|-------|----------|
| Easy | Random/simple choices |
| Medium | Basic strategy |
| Hard | Advanced evaluation |
| Boss | Special scripted + AI |

## Description
Agents control non-player entities in Duelyst, including practice opponents, challenge opponents, and boss battles. The base agent provides the framework for AI decision-making, while specific implementations handle different scenarios.
