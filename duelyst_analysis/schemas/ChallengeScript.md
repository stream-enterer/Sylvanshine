# ChallengeScript

## Source Evidence
- Primary location: `app/sdk/challenges/`
- Base class: `app/sdk/challenges/challenge.coffee`
- Categories: tutorial/, beginner/, medium/, advanced/, expert/, gate/

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| name | string | File basename | yes |
| className | string | CoffeeScript class name | yes |
| type | string | Subdirectory category | yes |
| totalTurns | number | Number of scripted turns | yes |
| totalActions | number | Total agent actions | yes |
| actionsPerTurn | object | Action count by turn | yes |
| instructionCount | number | Tutorial instruction count | no |
| hasBoardSetup | boolean | Has setupBoard method | yes |
| hasOpponentAgent | boolean | Has setupOpponentAgent | yes |
| cardSetupCount | number | Cards placed in setup | no |

## Challenge Types
| Type | Purpose | Complexity |
|------|---------|------------|
| tutorial | New player introduction | Very simple |
| beginner | Basic mechanics | Simple |
| medium | Intermediate tactics | Moderate |
| advanced | Complex interactions | High |
| expert | Extreme difficulty | Very high |
| gate | Faction unlock puzzles | Moderate |

## Script Structure
```coffeescript
class ChallengeBasic extends Challenge
  @type: "ChallengeBasic"
  name: "Basic Challenge"

  setupBoard: (gameSession) ->
    # Place initial cards
    @applyCardToBoard({id: Cards.Faction1.SilverguardKnight}, 4, 2, playerId1)

  setupOpponentAgent: (gameSession) ->
    @_opponentAgent = new StaticAgent(cpuPlayerId)
    @_opponentAgent.addActionForTurn(0,
      AgentActions.createAgentActionMoveUnit("general", {x:1, y:0}))
```

## Key Methods
| Method | Purpose |
|--------|---------|
| setupBoard | Initialize board state |
| setupOpponentAgent | Configure AI behavior |
| setupPlayer | Configure player state |
| getChallengeSolution | Define win condition |

## Instruction Labels
```coffeescript
{
  label: "Move your General here",
  isSpeech: true,
  yPosition: 0.6,
  duration: 3.0
}
```

## Dependencies
- Requires: GameSession, StaticAgent, AgentActions
- Used by: Tutorial system, puzzle mode

## Description
ChallengeScripts define single-player puzzle scenarios with pre-configured board states and scripted opponent behavior. Each challenge specifies initial card placement, turn-by-turn AI actions, and optional tutorial instructions. The system supports progressive difficulty through categorized challenge sets.

## Statistics
- **51+ challenges** in SDK
- **6 difficulty categories**
- Average actions per challenge: varies by type
- Tutorial challenges have most instructions
