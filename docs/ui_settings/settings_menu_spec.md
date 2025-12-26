# Duelyst Settings Menu UI Specification

## Overview

The Settings Menu is a modal dialog providing user-configurable options for visual, audio, gameplay, and account settings. It is accessed from the utility menu (gear icon) in the main UI.

## Architecture

| Component | File | Purpose |
|-----------|------|---------|
| View | `app/ui/views/item/settings_menu.js` | Backbone.Marionette ItemView controller |
| Template | `app/ui/templates/item/settings_menu.hbs` | Handlebars HTML template |
| Styles | `app/ui/styles/item/settings.scss` | SCSS styling |
| Localization | `app/localization/locales/en/settings.json` | UI strings |
| Icon | `app/resources/ui/utility_menu/settings.png` | Gear icon (32x32) |

## Modal Structure

```
.modal.duelyst-modal.settings-menu
├── .modal-dialog
│   ├── button.btn-cancel-primary (close button)
│   ├── .modal-content
│   │   ├── .modal-header
│   │   │   └── h4.modal-title ("Settings")
│   │   ├── .modal-body.settings
│   │   │   └── .container-fluid
│   │   │       └── .row (flexbox)
│   │   │           ├── .col-md-6.col-md-offset-1 (left column)
│   │   │           │   ├── Visual Settings Category
│   │   │           │   └── Audio Settings Category
│   │   │           └── .col-md-6 (right column)
│   │   │               ├── Game Settings Category
│   │   │               └── Account Settings Category
│   │   └── .modal-footer
│   │       ├── button.logout
│   │       ├── button.desktop-quit (desktop only)
│   │       └── #version-tag
```

## Settings Categories

### 1. Visual Settings

| Setting | ID | Type | Values | Storage Key |
|---------|-----|------|--------|-------------|
| Viewport | `#resolution` | Select dropdown | Auto, Tightest Fit, Pixel Perfect | `resolution` |
| Language | `#language` | Select dropdown | English (extendable) | `preferredLanguageKey` |
| HiDPI Mode | `#checkbox-hi-dpi-enabled` | Checkbox toggle | boolean | `hiDPIEnabled` |
| Lighting Quality | `#lighting-quality` | Button group | Low/Med/High | Profile model |
| Shadow Quality | `#shadow-quality` | Button group | Low/Med/High | Profile model |
| Board Quality | `#board-quality` | Button group | Low/High | Profile model |
| Bloom | `#bloom` | Range slider | 0.5 - 0.8 (step: 0.05) | Profile model |

#### Resolution Options
| Value | Description |
|-------|-------------|
| `RESOLUTION_AUTO` (0) | Best Fit (recommended) |
| `RESOLUTION_EXACT` (1) | Tightest Fit |
| `RESOLUTION_PIXEL_PERFECT` (9999) | Pixel Perfect |

#### Quality Constants
```javascript
LIGHTING_QUALITY_LOW = 0.0
LIGHTING_QUALITY_MEDIUM = 0.5
LIGHTING_QUALITY_HIGH = 1.0

SHADOW_QUALITY_LOW = 0.0
SHADOW_QUALITY_MEDIUM = 0.5
SHADOW_QUALITY_HIGH = 1.0

BLOOM_MIN = 0.5
BLOOM_DEFAULT = 0.7
BLOOM_MAX = 0.8
```

### 2. Audio Settings

| Setting | ID | Type | Range | Step |
|---------|-----|------|-------|------|
| Master Volume | `#master-volume` | Range slider | 0.0 - 1.0 | 0.01 |
| Music Volume | `#music-volume` | Range slider | 0.0 - 1.0 | 0.01 |
| Voice Volume | `#voice-volume` | Range slider | 0.0 - 1.0 | 0.01 |
| Effects Volume | `#effects-volume` | Range slider | 0.0 - 1.0 | 0.01 |

### 3. Game Settings

| Setting | ID | Type | Default |
|---------|-----|------|---------|
| Always Show Stats | `#checkbox-always-show-stats` | Checkbox toggle | false |
| Show Tips During Game | `#checkbox-show-in-game-tips` | Checkbox toggle | true |
| Battle Log | `#checkbox-show-battlelog` | Checkbox toggle | false |
| Player Details | `#checkbox-player-details` | Checkbox toggle | false |
| Sticky Targeting | `#checkbox-sticky-targeting` | Checkbox toggle | false |
| Razer Chroma | `#checkbox-razer-chroma-enabled` | Checkbox toggle | false |

**Note:** Razer Chroma setting only appears on desktop builds.

### 4. Account Settings

| Setting | ID | Type | Action |
|---------|-----|------|--------|
| Do Not Disturb | `#checkbox-do-not-disturb` | Checkbox toggle | Blocks friend requests |
| Block Spectators | `#checkbox-block-spectators` | Checkbox toggle | Prevents spectating |
| Change Username | `.change-username` | Button | Opens ChangeUsernameItemView dialog |
| Redeem Gift Code | `.redeem-gift-code` | Button | Opens RedeemGiftCodeModalView |

## UI Controls

### Toggle Switch Pattern
```html
<div class="switch">
  <input type="checkbox" id="checkbox-{name}" value="">
  <label><i></i></label>
</div>
```

### Button Group Pattern (Quality Settings)
```html
<div id="{setting}-quality" class="btn-group" role="group">
  <button type="button" id="{setting}-quality-low" class="btn btn-primary">LOW</button>
  <button type="button" id="{setting}-quality-medium" class="btn btn-primary">MED</button>
  <button type="button" id="{setting}-quality-high" class="btn btn-primary">HIGH</button>
</div>
```
Active button gets `.active` class.

### Range Slider Pattern
```html
<input type="range" id="{name}" class="range" min="0" max="1" step="0.01" />
```

### Select Dropdown Pattern
```html
<select id="{name}">
  <option value="{value}" {{#if selected}}selected{{/if}}>{description}</option>
</select>
```

## Data Model

Settings are stored on the `ProfileManager.getInstance().profile` Backbone model:

| Property | Type | Description |
|----------|------|-------------|
| `bloom` | number | Bloom intensity (0.5-0.8) |
| `doNotDisturb` | boolean | Block friend requests |
| `blockSpectators` | boolean | Block spectating |
| `alwaysShowStats` | boolean | Always show unit stats |
| `showBattleLog` | boolean | Display battle log |
| `showPlayerDetails` | boolean | Show deck count etc. |
| `stickyTargeting` | boolean | Keep unit selected after move |
| `showInGameTips` | boolean | Show tips during gameplay |
| `razerChromaEnabled` | boolean | Enable Razer Chroma |
| `masterVolume` | number | Master audio (0-1) |
| `musicVolume` | number | Music audio (0-1) |
| `voiceVolume` | number | Voice audio (0-1) |
| `effectsVolume` | number | SFX audio (0-1) |
| `lightingQuality` | number | 0.0/0.5/1.0 |
| `shadowQuality` | number | 0.0/0.5/1.0 |
| `boardQuality` | number | 0.0/1.0 |

## Events

### Internal Events
| Event | Trigger | Handler |
|-------|---------|---------|
| `change:lightingQuality` | Model change | `_updateLightingQualityUI()` |
| `change:shadowQuality` | Model change | `_updateShadowQualityUI()` |
| `change:boardQuality` | Model change | `_updateBoardQualityUI()` |

### Global Events
| Event | Purpose |
|-------|---------|
| `EVENTS.resize` | Triggered after resolution change |
| `EVENTS.request_resize` | Request viewport resize |
| `EVENTS.request_reload` | Request app reload (language change) |

## Visual Styling

### Modal Dimensions
- Modal body height: 48.0rem
- Background: `rgba(0, 0, 60, 0.5)`
- Border radius: 1.0rem

### Category Block
```scss
.settings-category {
  background: rgba(0, 0, 60, 0.5);
  border-radius: 1.0rem;
  padding: 0.5rem 1.5rem;
  margin-bottom: 2.0rem;
}
```

### Setting Row
- Height: 3.3rem
- Layout: Flexbox row, space-between
- Label font: 1.4rem, `$off-white` color

### Slider Track
- Height: 1.0rem
- Background: `#7f7f7f`
- Border radius: 1.0rem
- Thumb: 2.5rem x 1.0rem white rounded

### Category Icons (Font Awesome)
| Category | Icon |
|----------|------|
| Visual | `fa-image` |
| Audio | `fa-volume-up` |
| Game | `fa-gamepad` |
| Account | `fa-user` |

## Animation & Effects

### Entry Animation
- `Animations.fadeIn` on show
- Sound: `RSX.sfx_ui_tab_in.audio`

### Exit Animation
- `Animations.fadeOut` on destroy

### Gradient Color Map
On show, applies gradient color map to FX system:
```javascript
{r: 194, g: 203, b: 220, a: 255}  // light
{r: 36, g: 51, b: 65, a: 255}     // dark
```

## Tooltips

| Setting | Tooltip Text |
|---------|-------------|
| HiDPI Mode | "HiDPI mode for monitors with high pixel density. Warning - enabling this may cause Duelyst to run slowly!" |
| Bloom | "Adjust the intensity and brightness of the bloom visual effect." |
| Player Details | "Show additional information like remaining cards in deck at all times." |
| Sticky Targeting | "Units stay selected after moving - click to move then attack without reselecting the unit." |
| Do Not Disturb | "Automatically ignore new friend requests from other players. You can still challenge and chat with your friends." |
| Block Spectators | "Do not allow friends to spectate your games." |

## Footer Actions

| Button | Class | Action | Confirmation |
|--------|-------|--------|--------------|
| Logout | `.logout` | Calls `Session.logout()` | Yes - ConfirmDialogItemView |
| Quit | `.desktop-quit` | Calls `window.quitDesktop()` | Yes - ConfirmDialogItemView |

## Platform Differences

### Desktop Only
- Quit button (`.desktop-quit`)
- Razer Chroma setting (`#razer-chroma-setting`)

### Web Only
- No quit button
- No Razer Chroma setting

## Dependencies

| Module | Purpose |
|--------|---------|
| `Backbone.Marionette.ItemView` | View framework |
| `NavigationManager` | Modal management |
| `ProfileManager` | User profile/settings model |
| `Storage` | Local storage persistence |
| `CONFIG` | Quality/resolution constants |
| `Scene.getInstance().getFX()` | Gradient color map effects |
| `audio_engine` | Sound effects |

## Accessibility

- Tooltips with `data-toggle="tooltip"` on complex settings
- Clear labels for all controls
- Bootstrap tooltip integration

## Localization Keys

```
main_menu.menu_item_settings
settings.category_visual_label
settings.category_audio_label
settings.category_game_label
settings.category_account_label
settings.hiDPI_name / settings.hiDPI_tooltip
settings.lighting_quality
settings.shadow_quality
settings.board_quality
settings.bloom_label / settings.bloom_tooltip
settings.master_volume_label
settings.music_volume_label
settings.voice_volume_label
settings.effects_volume_label
settings.always_show_stats
settings.show_tips_during_gameplay
settings.battle_log
settings.player_details / settings.player_details_tooltip
settings.sticky_targeting / settings.sticky_targeting_tooltip
settings.razer_chroma
settings.do_not_disturb / settings.do_not_disturb_tooltip
settings.block_spectators / settings.block_spectators_tooltip
settings.change_username
settings.redeem_code
settings.logout
settings.quit
settings.settings_button_low
settings.settings_button_medium
settings.settings_button_high
settings.best_fit
settings.tightest_fit
settings.pixel_perfect
settings.viewport_not_supported
```
