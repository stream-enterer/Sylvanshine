# System: UI (User Interface)

**Location:** app/ui/

## Purpose
The UI system provides the HTML/Backbone.js-based interface for menus, overlays, and game controls, managing user interactions outside the Cocos2d game canvas.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| NavigationManager | managers/navigation_manager.js | Screen flow control |
| InventoryManager | managers/inventory_manager.js | Player inventory |
| GamesManager | managers/games_manager.js | Matchmaking/games |
| QuestsManager | managers/quests_manager.js | Quest tracking |
| ProgressionManager | managers/progression_manager.js | Player progression |
| PackageManager | managers/package_manager.js | Asset loading |
| ChatManager | managers/chat_manager.js | In-game chat |
| CrateManager | managers/crate_manager.js | Loot crate handling |

## Data Flow
**Input:** User clicks/touches, keyboard input, server events
**Processing:** Event routing → State updates → View rendering
**Output:** UI updates, API calls, navigation changes

## Dependencies
**Requires:** Backbone.js, Marionette.js, jQuery, Session
**Used by:** Application, View system

## Directory Structure
```
ui/
├── managers/           # State managers (12 files)
├── views/              # Backbone views
│   ├── composite/      # Complex composed views
│   ├── item/           # Individual item views
│   └── layouts/        # Page layouts
├── views2/             # Newer view implementations
│   ├── collection/     # Card collection
│   ├── shop/           # Store interface
│   ├── profile/        # Player profile
│   ├── quests/         # Quest UI
│   └── rift/           # Rift mode UI
├── collections/        # Backbone collections
├── models/             # Backbone models
├── extensions/         # UI extensions
├── templates/          # Handlebars templates
└── styles/             # SCSS stylesheets
```

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.APP_SELECTOR | '#app' | Main app container |
| CONFIG.GAME_SELECTOR | '#app-game' | Game canvas container |
| CONFIG.COLLECTION_SELECTOR | '#app-collection' | Collection view |
| CONFIG.CONTENT_SELECTOR | '#app-content-region' | Content area |
| CONFIG.DIALOGUE_ENTER_DURATION | 0.3 | Dialog animation |

## Key Managers
| Manager | Responsibility |
|---------|----------------|
| NavigationManager | Screen transitions, confirm/cancel |
| InventoryManager | Cards, decks, currency, cosmetics |
| GamesManager | Matchmaking, game invites |
| QuestsManager | Daily quests, rewards |
| ProgressionManager | XP, levels, achievements |
| PackageManager | Dynamic asset loading |
| ChatManager | Buddy chat, notifications |
| NotificationsManager | System notifications |
| ServerStatusManager | Connection status |
| CrateManager | Loot crate inventory |
| NewPlayerManager | Tutorial progression |
| SoundManager | Audio controls |

## View Types
| View Category | Examples |
|---------------|----------|
| Layouts | game.js, arena.js, play.js |
| Composite | deck_select.js, challenge_select.js |
| Item | main_menu.js, settings_menu.js |
| Dialogs | confirm_dialog.js, error_dialog.js |
| Forms | login_menu.js, form_prompt.js |

## Events
| Event | Emitter | Purpose |
|-------|---------|---------|
| cards_collection_change | InventoryManager | Card inventory update |
| wallet_change | InventoryManager | Currency change |
| decks_collection_change | InventoryManager | Deck modification |
| matchmaking_start/cancel | GamesManager | Queue state |
| user_attempt_confirm | NavigationManager | Confirm pressed |
| show_play/shop/collection | Various | Navigation |

## Statistics
- 12 manager classes
- 8+ view subdirectories
- Manages all non-game UI elements
- Integrates with Marionette.js for composition
- Uses Handlebars templates for rendering
