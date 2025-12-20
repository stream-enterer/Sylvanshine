# Quest

## Source Evidence
- Primary class: `Quest` (app/sdk/quests/quest.coffee)
- Related classes: 20+ quest subclasses
- Data shape: Daily objectives for rewards

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| id | string | quest identification | yes |
| name | string | display name | yes |
| types | array | quest categories | yes |
| description | string | quest text | yes |
| goldReward | number | gold on completion | yes |
| spiritOrbsReward | number | orbs on completion | no |
| isReplaceable | boolean | can be replaced | yes |
| requiresStreak | boolean | streak requirement | no |
| friendlyMatchesCount | number | friendly wins needed | no |

## Lifecycle Events
- created: Daily quest generation
- destroyed: Quest completion/replacement
- modified: Progress updates

## Dependencies
- Requires: QuestFactory, QuestsManager
- Used by: UI, daily_quests_change events

## Quest Types
| Type | Description |
|------|-------------|
| QuestGameGoal | Win games with conditions |
| QuestWinWithFaction | Win with specific faction |
| QuestParticipationWithFaction | Play games with faction |
| QuestBeginner | New player quests |
| QuestCatchUp | Return player quests |
| QuestSeasonal | Limited-time quests |
| QuestFrostfire | Holiday event quests |
| QuestAnniversary | Anniversary quests |

## Quest Slots
- Players have 3 daily quest slots
- One quest can be replaced per day
- Quests generate based on player progress

## Key Methods
- `getId()` - Get quest ID
- `getName()` - Get quest name
- `getTypes()` - Get quest type array
- `getDescription()` - Get quest text
- `getGoldReward()` - Get gold reward amount
- `getSpiritOrbsReward()` - Get orb reward
- `getIsReplaceable()` - Check if replaceable
- `progressForGameDataForPlayerId()` - Calculate progress
- `isAvailableOn(moment)` - Check if available
- `expiresOn()` - Get expiration date

## Quest Events
| Event | Trigger |
|-------|---------|
| daily_quests_change | Quest state changes |
| showing_quest_log | Quest UI opened |

## Description
Quests are daily objectives that reward players with gold and sometimes spirit orbs. Players receive new quests daily and can replace one quest per day. Quest types range from winning games to playing specific factions, with special event quests for holidays and promotions.
