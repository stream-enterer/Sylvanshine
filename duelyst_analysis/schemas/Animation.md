# Animation

## Source Evidence
- Primary definition: `app/data/resources.js` (RSX object)
- Loading: `app/ui/managers/package_manager.js`
- Execution: `app/common/utils/utils_engine.js`
- FX Integration: `app/view/nodes/fx/FXSprite.js`

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| key | string | RSX object key | yes |
| name | string | Animation identifier | yes |
| framePrefix | string | Sprite frame name pattern | yes |
| frameDelay | number | Seconds between frames | yes |
| img | string | Texture image path | yes |
| plist | string | Sprite frame config file | yes |
| is16Bit | boolean | 16-bit color flag | no |

## Frame System
- Frames named with prefix + numeric suffix (e.g., `fx_blood_0`, `fx_blood_1`)
- Frame indices extracted from plist via regex: `^${framePrefix}(?=[0-9\.\\b])`
- Loaded into `cc.spriteFrameCache` at runtime
- Animation created via `cc.Animation.create(animFrames, frameDelay)`

## Timing Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| Global speed modifier | 0.8x | Applied in package_manager.js:869 |
| Fast effects | 0.033s | High-speed animations |
| Standard effects | 0.08s | Most FX animations |
| Slow effects | 0.20s | Breathing, idle animations |

## Animation Categories
| Category | Frame Delay Range | Examples |
|----------|-------------------|----------|
| Combat FX | 0.04-0.08s | Blood, impacts, attacks |
| Spell FX | 0.08s | Faction spell effects |
| Unit Idle | 0.10-0.20s | Breathing, hovering |
| UI Effects | 0.033-0.08s | Card reveals, transitions |

## Lifecycle Events
- loaded: `cc.animationCache.addAnimation(animation, name)`
- played: `UtilsEngine.getAnimationAction(spriteIdentifier)`
- completed: Callback or sequence continuation

## Dependencies
- Requires: Cocos2d-html5 animation system, plist files
- Used by: FXSprite, UnitNode, EntityNode, UI animations

## Description
Animations define sprite-based frame sequences for visual effects. Each animation specifies a texture atlas (plist), frame naming pattern, and timing. The system uses Cocos2d's sprite animation framework with a global 0.8x speed modifier for performance optimization.

## Statistics
- **318 animations** extracted from resources.js
- Frame delays range from 0.033s to 0.20s
- Most common delay: 0.08s (standard FX)
