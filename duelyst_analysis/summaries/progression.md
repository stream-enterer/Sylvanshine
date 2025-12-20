# System: Progression (Achievements & Quests)

**Location:** app/sdk/achievements/, app/sdk/quests/, app/sdk/progression/

## Purpose
The Progression system tracks player advancement through achievements, daily quests, faction leveling, and rewards, providing long-term engagement goals.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Achievement | achievements/achievement.coffee | Achievement base class |
| AchievementsFactory | achievements/achievementsFactory.coffee | Achievement lookup |
| Quest | quests/quest.coffee | Quest base class |
| QuestsFactory | quests/questsFactory.coffee | Quest lookup |
| Rank | rank/*.coffee | Ladder ranking |
| Ribbons | ribbons/*.coffee | Profile ribbons |

## Data Flow
**Input:** Game events, login, purchases, collection changes
**Processing:** Progress tracking → Completion check → Reward grant
**Output:** Rewards (gold, spirit, orbs, cosmetics)

## Dependencies
**Requires:** GameSession events, Inventory, Server API
**Used by:** UI progression views, notification system

## Achievement Categories
| Category | Trigger | Examples |
|----------|---------|----------|
| Game-based | Win/play games | BestOfFriendsAchievement, HomeTurfAchievement |
| Quest-based | Complete quests | EpicQuestorAchievement, JourneymanQuestorAchievement |
| Inventory-based | Card collection | CollectorSupremeAchievement, SisterAchievements |
| Crafting-based | Craft/disenchant | WelcomeToCraftingAchievement |
| Rank-based | Achieve rank | SilverDivisionAchievement |
| Login-based | Log in during event | ChristmasLoginAchievement, HalloweenLoginAchievement |
| Shop-based | Purchase items | StarterBundleAchievements, NamasteAchievement |
| Faction-based | Faction progression | BloodbornAchievement, WorldExplorerAchievement |
| Orb-based | Open spirit orbs | MythronOrb1-7Achievement |
| Referral-based | Referral actions | FirstReferralPurchaseAchievement |

## Quest Categories
| Type | ID Range | Reward | Description |
|------|----------|--------|-------------|
| Daily Short | 101-106 | 150g | Play 4 games with faction |
| Daily Long | 1500-1800 | 300g | Play 8 games, deal damage, destroy minions |
| Beginner | 9901-9910 | 150g/orbs | New player quests |
| Seasonal | 30001+ | Cosmetics | Monthly challenges |
| Promotional | 40001+ | Special | Limited time events |
| Excluded | Various | - | Disabled quests |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.FIRST_WIN_OF_DAY_GOLD_REWARD | 20 | Daily win bonus |
| Quest gold rewards | 150-300 | Per quest type |
| Spirit orb rewards | 1 | Some quests |

## Statistics
- **57 achievements** in instances/achievements.tsv
- **49 quests** in instances/quests.tsv
- Achievement breakdown:
  - Login achievements: 18
  - Game achievements: 5
  - Quest achievements: 4
  - Inventory achievements: 9
  - Shop achievements: 5
  - Faction achievements: 2
  - Orb achievements: 7
  - Wartech achievements: 6
  - Other: 1

## Quest Types
| Type | Count | Description |
|------|-------|-------------|
| QuestParticipationWithFaction | 6 | Play with specific faction |
| QuestWinWithFaction | 6 | Win with specific faction |
| QuestGameGoal | 10 | Gameplay objectives |
| QuestBeginner* | 10 | New player quests |
| QuestSeasonal* | 7 | Monthly quests |
| QuestPromotional* | 3 | Event quests |
| QuestCatchUp | 1 | Returning player |

## Reward Types
| Reward | Source |
|--------|--------|
| Gold | Quests, achievements, wins |
| Spirit | Quests, achievements |
| Spirit Orbs | Quests, achievements |
| Cosmetics | Seasonal quests, achievements |
| Cards | Specific achievements |
| Keys | Crate achievements |

## Progression Events
| Event | Purpose |
|-------|---------|
| progressForGameDataForPlayerId | Track game results |
| progressForCompletingQuestId | Quest completion |
| progressForCrafting | Craft tracking |
| progressForDisenchanting | Disenchant tracking |
| progressForCardCollection | Collection size |
| progressForAchievingRank | Rank milestones |
| progressForLoggingIn | Login streaks |
