# System: Challenges & Tutorials

**Location:** app/sdk/challenges/

## Purpose
The Challenge system provides puzzle scenarios and tutorial lessons that teach game mechanics through scripted board states with specific win conditions.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Challenge | challenges/challenge.coffee | Base challenge class |
| ChallengeRemote | challenges/challengeRemote.coffee | Network challenges |
| Instruction | challenges/instruction.coffee | Tutorial instructions |
| Validator | validators/validator.coffee | Action validation |
| LessonOne-Four | challenges/tutorial/*.coffee | Core tutorials |
| Sandbox | challenges/sandbox.coffee | Practice mode |

## Data Flow
**Input:** Challenge definition, player actions
**Processing:** State setup → Move validation → Win condition check
**Output:** Pass/fail result, tutorial progression

## Dependencies
**Requires:** GameSession, Card system, Action system
**Used by:** UI challenge views, progression system

## Directory Structure
```
challenges/
├── challenge.coffee         # Base class
├── challengeRemote.coffee   # Network variant
├── instruction.coffee       # Tutorial helpers
├── sandbox.coffee           # Free practice
├── sandboxDeveloper.coffee  # Dev sandbox
├── tutorial/
│   ├── lesson1.coffee       # Basics
│   ├── lesson2.coffee       # Combat
│   ├── lesson3.coffee       # Spells
│   ├── lesson4.coffee       # Advanced
│   ├── BeginnerFlyingChallenge1.coffee
│   └── BeginnerRangedChallenge1.coffee
├── lyonar/
│   ├── AdvancedLyonarChallenge1.coffee
│   ├── AdvancedLyonarChallenge2.coffee
│   └── BeginnerLyonarChallenge1-4.coffee
├── songhai/
│   ├── AdvancedSonghaiChallenge1.coffee
│   ├── MediumSonghaiChallenge1-2.coffee
│   └── BeginnerSonghaiChallenge1-5.coffee
├── vetruvian/
│   ├── AdvancedVetruvianChallenge1-2.coffee
│   ├── MediumVetruvianChallenge1-2.coffee
│   └── BeginnerVetruvianChallenge1-5.coffee
├── abyssian/
│   ├── AdvancedAbyssianChallenge1.coffee
│   ├── MediumAbyssianChallenge1.coffee
│   └── BeginnerAbyssianChallenge1-6.coffee
├── magmar/
│   ├── AdvancedMagmarChallenge1.coffee
│   ├── MediumMagmarChallenge1.coffee
│   └── BeginnerMagmarChallenge1-4.coffee
└── vanar/
    ├── AdvancedVanarChallenge1-2.coffee
    └── BeginnerVanarChallenge1-5.coffee
```

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.ACTION_INSTRUCTIONAL_ARROW_DURATION | 0.75 | Arrow display time |
| CONFIG.ACTION_INSTRUCTIONAL_ARROW_SHOW_PERCENT | 1.0 | Arrow visibility |
| CONFIG.INSTRUCTION_NODE_BACKGROUND_COLOR | rgb | Instruction styling |

## Resources
| Resource | Purpose |
|----------|---------|
| resources/challenges/ | Challenge images |
| resources/tutorial/ | Tutorial assets (814 bytes) |

## Challenge Categories
| Category | Count | Description |
|----------|-------|-------------|
| Tutorial Lessons | 4 | Core game mechanics |
| Keyword Tutorials | 2 | Flying, Ranged |
| Lyonar | 6 | Faction puzzles |
| Songhai | 8 | Faction puzzles |
| Vetruvian | 9 | Faction puzzles |
| Abyssian | 8 | Faction puzzles |
| Magmar | 6 | Faction puzzles |
| Vanar | 7 | Faction puzzles |
| **Total** | **51** | All challenges |

## Difficulty Levels
| Level | Pattern | Focus |
|-------|---------|-------|
| Beginner | BeginnerXChallenge* | Basic mechanics |
| Medium | MediumXChallenge* | Intermediate tactics |
| Advanced | AdvancedXChallenge* | Complex puzzles |

## Challenge Structure
Each challenge defines:
- Initial board state (units, positions)
- Player/opponent hands
- Win condition (usually kill opponent general)
- Optional instructions/hints
- Scripted opponent behavior

## Statistics
- **51 challenges** in instances/challenges.tsv
- 4 tutorial lessons
- 6 faction challenge sets
- 3 difficulty levels
- Sandbox mode for free practice
