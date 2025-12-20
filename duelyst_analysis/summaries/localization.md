# System: Localization

**Location:** app/localization/

## Purpose
The localization system provides internationalized text for all game content using i18next. It supports multiple languages with parameterized strings and namespace organization.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| i18next Config | localization/index.coffee | Framework setup |
| English Locale | locales/en/*.json | Primary translations |
| German Locale | locales/de/index.json | German translations |
| Chinese Locale | locales/zh-tw/*.json | Traditional Chinese |

## Data Flow
**Input:** Translation key from game code
**Processing:** Namespace lookup → Parameter substitution → Language fallback
**Output:** Localized string

## i18next Configuration
```coffeescript
i18next.init({
  whitelist: ['en', 'de'],
  fallbackLng: 'en',
  ns: 'translation',
  backend: {
    loadPath: 'resources/locales/{{lng}}/index.json'
  }
})
```

## Namespace Organization
| Namespace | Strings | Content |
|-----------|---------|---------|
| cards | 1,660 | Card names, abilities |
| cosmetics | 740 | Skins, emotes |
| modifiers | 511 | Game mechanics |
| challenges | 314 | Puzzle descriptions |
| factions | 166 | Faction lore |
| shop | 165 | Store content |
| tutorial | 111 | Instructions |
| common | 96 | Shared UI |
| achievements | 71 | Goals |

## String Examples
```json
// Simple string
"success_title": "SUCCESS!"

// Parameterized
"plus_attack_key": "+{{amount}} Attack."

// Nested
"faction_1.general.name": "Argeon Highmayne"
```

## Usage in Code
```javascript
// Simple lookup
i18next.t("common.success_title")

// With parameters
i18next.t("modifiers.plus_attack_key", {amount: 2})

// Namespaced
i18next.t("cards.faction_1_spell_roar_name")
```

## Language Support
| Language | Code | Structure | Status |
|----------|------|-----------|--------|
| English | en | 34 separate JSON files | Complete |
| German | de | Single index.json | Complete |
| Chinese (TW) | zh-tw | 34 separate JSON files | Partial |

## Dependencies
**Requires:** i18next, i18next-xhr-backend
**Used by:** All UI components, tooltips, game messages

## Statistics
- **4,397 localization strings**
- **3 languages** supported
- **34 namespaces** (English)
- Largest: cards (1,660 strings)
