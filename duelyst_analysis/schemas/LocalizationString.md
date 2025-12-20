# LocalizationString

## Source Evidence
- Primary location: `app/localization/locales/`
- Configuration: `app/localization/index.coffee`
- Usage: i18next.t() throughout codebase

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| namespace | string | JSON filename | yes |
| key | string | Translation key | yes |
| value_en | string | English text | yes |
| has_params | boolean | Contains {{variables}} | no |

## Languages Supported
| Code | Language | Structure |
|------|----------|-----------|
| en | English | 34 separate JSON files |
| de | German | Single consolidated index.json |
| zh-tw | Chinese (Traditional) | 34 separate JSON files |

## Namespaces
| Namespace | Count | Purpose |
|-----------|-------|---------|
| cards | 1660 | Card names and abilities |
| cosmetics | 740 | Cosmetic item names |
| modifiers | 511 | Game mechanic descriptions |
| challenges | 314 | Challenge names/descriptions |
| factions | 166 | Faction names/lore |
| shop | 165 | Shop UI and products |
| tutorial | 111 | Tutorial instructions |
| common | 96 | Shared UI elements |
| achievements | 71 | Achievement names |
| game_tips | 61 | In-game tips |

## Parameter Syntax
```javascript
// Simple substitution
"Hello {{name}}" → i18next.t("key", {name: "Player"})

// Plural forms
"{{count}} card" / "{{count}} cards"

// Nested keys
"faction_1.general.name"
```

## i18next Configuration
| Setting | Value |
|---------|-------|
| Fallback language | en |
| Context separator | $ |
| Default namespace | translation |
| Detection methods | querystring, navigator, localStorage |

## Dependencies
- Requires: i18next, i18next-xhr-backend
- Used by: All UI components, card tooltips, game messages

## Description
Localization strings provide internationalized text for all game content. The system uses i18next with JSON storage, supporting parameterized strings and language fallback. English serves as the primary language with German and Chinese translations available.

## Statistics
- **4,397 localization strings** extracted
- **3 languages** supported
- **34 namespaces** in English locale
- Largest namespace: cards (1,660 strings)
