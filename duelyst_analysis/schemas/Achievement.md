# Achievement

## Source Evidence
- Primary class: `Achievement` (app/sdk/achievements/achievement.coffee)
- Related classes: 30+ achievement subclasses
- Data shape: Meta-game progression tracking

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | string | data_shapes.tsv:115 | yes |
| title | string | data_shapes.tsv:116 | yes |
| description | string | data_shapes.tsv:117 | yes |
| progressRequired | number | data_shapes.tsv:118 | yes |
| rewards | object | data_shapes.tsv:119 | yes |
| enabled | boolean | data_shapes.tsv:120 | yes |

## Lifecycle Events
- created: Account creation
- destroyed: Never (persistent)
- modified: Progress events, completion

## Dependencies
- Requires: AchievementsFactory, Player account
- Used by: UI, ProgressionManager

## Achievement Types (by trigger)
| Type | Trigger | Examples |
|------|---------|----------|
| Game-based | Win/play games | BestOfFriends, TheArtOfWar |
| Quest-based | Complete quests | EpicQuestor, LegendaryQuestor |
| Inventory-based | Card collection | CollectorSupreme, Sister cards |
| Rank-based | Achieve rank | SilverDivision |
| Login-based | Login events | Holiday promos, Frostfire |
| Shop-based | Make purchases | StarterBundle achievements |
| Crafting-based | Craft/disenchant | WelcomeToCrafting |
| Faction-based | Level factions | WorldExplorer, Bloodborn |
| Orb-based | Open spirit orbs | Mythron achievements |
| Referral-based | Referral program | FirstReferralPurchase |

## Reward Types
| Reward | Description |
|--------|-------------|
| gold | In-game currency |
| spirit | Crafting currency |
| cards | Specific cards |
| spiritOrb | Booster packs |
| cosmetics | Visual items |
| gauntletTicket | Arena entry |
| giftChests | Loot crates |

## Key Methods
- `getId()` - Get achievement ID
- `getTitle()` - Get display title
- `getDescription()` - Get description text
- `getRewards()` - Get reward object
- `progressForGameDataForPlayerId()` - Calculate progress from game
- `progressForChallengeId()` - Progress from challenges
- `progressForCardCollection()` - Progress from collection

## Description
Achievements are meta-game progression goals that reward players for various activities. They persist across sessions and provide gold, spirit, cards, and other rewards upon completion. Achievements are managed by the AchievementsFactory and tracked by the ProgressionManager.
