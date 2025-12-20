# Challenge

## Source Evidence
- Primary class: `Challenge` (app/sdk/challenges/challenge.coffee)
- Related classes: 50+ challenge subclasses by faction
- Data shape: Puzzle/tutorial scenarios

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | challenge identification | yes |
| skipMulligan | boolean | classes.tsv | no |
| instructions | array | turn-by-turn guidance | yes |
| opponentAgent | Agent | AI controller | yes |
| myPlayerDeckData | object | player deck config | yes |
| opponentPlayerDeckData | object | opponent deck config | yes |

## Lifecycle Events
- created: start_challenge event
- destroyed: challenge_completed, challenge_lost
- modified: instruction_triggered, challenge_reset

## Dependencies
- Requires: GameSession, ChallengeFactory, Agent
- Used by: UI, Tutorial system, Daily challenges

## Challenge Categories (from ChallengeCategory)
| Category | Description |
|----------|-------------|
| Tutorial | Basic game mechanics |
| Beginner | Faction introductions |
| Intermediate | Card synergies |
| Advanced | Complex puzzles |
| Expert | Master-level challenges |
| Daily | Rotating daily puzzles |

## Challenge Structure
```
Challenge
├── My Deck Configuration
├── Opponent Deck Configuration
├── Board Setup
├── Instructions per Turn
│   ├── Required actions
│   ├── Hints and guidance
│   └── Victory conditions
└── Opponent Agent AI
```

## Key Methods
- `constructor()` - Initialize challenge
- `getType()` - Get challenge type ID
- `getSkipMulligan()` - Skip starting hand selection
- `getInstructions()` - Get instruction list
- `getCurrentInstruction()` - Get current step
- `getOpponentAgent()` - Get AI agent
- `setupSession()` - Configure GameSession
- `setupBoard()` - Place initial entities
- `activateNextInstruction()` - Progress to next step
- `challengeReset()` - Reset to start
- `challengeRollback()` - Undo last action
- `onChallengeLost()` - Handle failure

## Challenge Events
| Event | Trigger |
|-------|---------|
| start_challenge | Challenge begins |
| challenge_start | Session initialized |
| challenge_completed | Success |
| challenge_lost | Failure |
| challenge_reset | Player resets |
| instruction_triggered | New instruction |

## Faction Challenges
Each faction has a series of challenges:
- Basic (6 per faction)
- Advanced (3+ per faction)
- Gate challenges (unlock content)

## Description
Challenges are predefined puzzle scenarios that teach game mechanics or provide daily puzzle content. Each challenge has a specific board state, deck configurations, and turn-by-turn instructions. The opponent is controlled by an AI agent that follows scripted behavior.
