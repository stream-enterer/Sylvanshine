# System: Animations

**Location:** app/data/resources.js, app/view/nodes/fx/

## Purpose
The animation system provides sprite-based visual effects for all game elements, from combat FX to UI transitions. It uses Cocos2d's animation framework with texture atlases and frame sequences.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| RSX | data/resources.js | Animation definitions registry |
| PackageManager | ui/managers/package_manager.js | Animation loading |
| UtilsEngine | common/utils/utils_engine.js | Animation execution |
| FXSprite | view/nodes/fx/FXSprite.js | FX animation rendering |

## Data Flow
**Input:** Animation name/identifier from game code
**Processing:** Load frames from cache → Create animation → Execute sequence
**Output:** Visual animation on screen

## Animation Definition
```javascript
fx_bloodbig: {
  name: 'BloodExplosionBig',
  framePrefix: 'fx_bloodbig_',
  frameDelay: 0.04,
  img: 'resources/fx/fx_blood_explosion.png',
  plist: 'resources/fx/fx_blood_explosion.plist',
  is16Bit: true
}
```

## Frame Timing Categories
| Speed | Frame Delay | Use Cases |
|-------|-------------|-----------|
| Very Fast | 0.033s | Fluid effects, rapid FX |
| Fast | 0.04s | Blood, explosions |
| Standard | 0.08s | Most spell/combat FX |
| Medium | 0.10s | Shield effects |
| Slow | 0.20s | Breathing, idle states |

## Loading Pipeline
1. **Resource Registration** - Define in RSX object
2. **Plist Parsing** - Load sprite frame definitions
3. **Frame Extraction** - Get frames matching prefix pattern
4. **Animation Creation** - Build cc.Animation with frame delay
5. **Cache Storage** - Store in cc.animationCache

## Global Speed Modifier
```javascript
// package_manager.js:869
frameDelay * 0.8  // 20% faster than defined
```

## Dependencies
**Requires:** Cocos2d-html5, texture atlas (plist), sprite images
**Used by:** FX system, UnitNode, EntityNode, UI components

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.ANIMATE_FAST_DURATION | 0.2s | Fast animation timing |
| CONFIG.ANIMATE_MEDIUM_DURATION | 0.35s | Standard timing |
| CONFIG.ANIMATE_SLOW_DURATION | 1.0s | Slow timing |

## Statistics
- **318 animations** defined in RSX
- Frame delays: 0.033s to 0.20s
- Categories: FX, units, UI, arena
- Most common delay: 0.08s
