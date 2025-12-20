# FX (Effects)

## Source Evidence
- Primary class: `FX` (app/view/fx/FX.js)
- Related classes: FXType, various FX classes
- Data shape: Visual effect system

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| type | string | effect type | yes |
| duration | number | effect length | no |
| position | {x,y} | effect location | yes |
| color | object | effect color | no |
| scale | number | effect size | no |
| opacity | number | transparency | no |

## Lifecycle Events
- created: Action resolution, card play
- destroyed: Effect completes
- modified: Screen effects change

## Dependencies
- Requires: GameLayer, EntityNode, cocos2d-js
- Used by: GameLayer, Card actions, UI

## FX Types (from FXType)
| Type | Description |
|------|-------------|
| Spawn | Entity summoning effects |
| Death | Entity death effects |
| Damage | Damage indication |
| Heal | Healing indication |
| Buff | Buff application |
| Debuff | Debuff application |
| Move | Movement trails |
| Attack | Attack animations |
| Spell | Spell casting effects |
| Impact | Hit effects |
| Ambient | Background effects |

## Screen Effects
| Effect | Method | Description |
|--------|--------|-------------|
| Blur Screen | blur_screen_start/stop | Screen blur |
| Blur Surface | blur_surface_start/stop | Layer blur |
| Cache Screen | caching_screen_setup/start/stop | Render cache |
| Cache Surface | caching_surface_setup/start/stop | Surface cache |

## FX Resources (RSX)
| Category | Examples |
|----------|----------|
| Boss GIFs | alabasterTitanBossGIF, archonBossGIF |
| Particles | battlemap particles |
| Sprites | Effect sprite sheets |
| Shaders | Visual shaders |

## Key FX Methods
- `showForAction(action)` - Show effect for action
- `showAtPosition(pos, type)` - Show at location
- `fadeIn/fadeOut()` - Opacity transitions
- `scale/rotate()` - Transform effects

## Effect Timing (from config.js)
| Constant | Value | Description |
|----------|-------|-------------|
| PLAYED_SPELL_FX_FADE_IN_DURATION | 0.5 | Spell effect fade in |
| PLAYED_SPELL_FX_FADE_OUT_DURATION | 0.5 | Spell effect fade out |
| MAX_FX_PER_EVENT | 5 | Effect limit |

## Description
FX (Effects) is the visual effects system that creates particle effects, screen transitions, and animations for game actions. It integrates with the cocos2d-js rendering engine and responds to game events to display appropriate visual feedback.
