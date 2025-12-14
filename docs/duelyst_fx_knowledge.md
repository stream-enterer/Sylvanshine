# Duelyst Visual Pipeline - Complete Knowledge Base

## 1. FX Definition Structure

FX are defined as arrays of effect objects in `app/data/fx.js`:

```javascript
Faction1: {
  UnitSpawnFX: [
    {
      spriteIdentifier: RSX.fxTeleportRecallWhite.name,
      offset: { x: 0, y: -10 },
      color: { r: 255, g: 255, b: 100 }
    },
    { spriteIdentifier: RSX.fx_f1_holyimmolation.name },
    {
      type: 'Light',
      offset: { x: 0, y: -80 },
      duration: 1,
      castsShadows: false,
      radius: 270,
      intensity: 9,
      color: { r: 255, g: 200, b: 100 }
    },
    {
      type: 'Particles',
      plistFile: RSX.ptcl_dot_square_green.plist
    }
  ]
}
```

### FX Object Types

**Sprite FX:**
- `spriteIdentifier` - RSX resource name
- `offset` - {x, y} position offset
- `color` - {r, g, b} tint (optional)
- `flippedX` - horizontal flip (optional)
- `blendSrc` - blend mode override (optional)

**Light FX:**
- `type: 'Light'`
- `offset` - light position
- `duration` - how long light lasts
- `radius` - light radius
- `intensity` - brightness
- `castsShadows` - shadow casting
- `color` - light color

**Particle FX:**
- `type: 'Particles'`
- `plistFile` - particle system definition

**Decal FX:**
- `type: 'Decal'`
- Projects onto ground plane

**Emitted FX:**
- `emitFX` - nested FX that spawn during parent lifetime

### Faction Visual Identity

Each faction has distinct spawn FX:
- **Lyonar (F1)**: White teleport + holy fire + yellow light
- **Songhai (F2)**: Red smoke puffs + red light
- **Vetruvian (F3)**: Bladestorm + yellow light
- **Abyssian (F4)**: Purple smoke + purple light
- **Magmar (F5)**: Green teleport + lightning + green light
- **Vanar (F6)**: Blue water splashes + swirl rings + cyan light
- **Neutral**: Blue teleport + swirl rings + white light

## 2. FXSprite Base Properties

From `app/view/nodes/fx/FXSprite.js`:

```javascript
{
  // Impact timing
  autoStart: true,          // activate on scene enter
  impactAtStart: true,      // impact immediately (default)
  impactAtEnd: false,       // impact when animation completes
  impactFX: null,           // FX to spawn on impact
  
  // Lifetime
  duration: 0.0,            // extra time beyond animation
  looping: false,           // loop forever
  
  // Fading
  fadeInDuration: 0.0,      // fade in time
  fadeInDurationPct: 0.0,   // fade in as % of anim duration
  fadeOutDuration: 0.0,     // fade out time
  fadeOutDurationPct: 0.0,  // fade out as % of anim duration
  fadeLooping: true,        // fade on each loop
  
  // Position tracking
  sourceBoardPosition: null,
  targetBoardPosition: null,
  sourceScreenPosition: null,
  targetScreenPosition: null,
  offset: null,             // absolute offset from base
  sourceOffset: null,       // offset from source
  targetOffset: null,       // offset from target
  
  // Motion effects
  rotationPerSecond: 0.0,
  xyzRotationPerSecond: null,
  
  // Pulsing
  pulseSmooth: false,       // 0→1→0 vs 0→1 loop
  pulseScaleMin: 1.0,
  pulseScaleMax: 1.0,       // only pulses if min != max
  pulseFadeIn: 0.5,         // fade in at this pulse %
  pulseFadeOut: 0.7,        // fade out at this pulse %
  
  // Spawning sub-FX
  emitFX: null,             // array of FX to emit during lifetime
  
  // Visual
  reverse: false,           // reverse animation playback
  antiAlias: false,         // no AA for pixel art
  autoZOrder: true,
  depthOffset: CONFIG.DEPTH_OFFSET
}
```

### Impact System

```javascript
getImpactDelay() {
  if (this.impactAtEnd) {
    return this.getBaseDuration();  // delay = animation length
  }
  return 0.0;  // impact immediately
}

end() {
  if (this.impactAtEnd) {
    this.impact();  // spawn impactFX at destination
  }
  if (this.removeOnEnd) {
    this.destroy();
  }
}

impact() {
  const impactFXSprites = this.getImpactFXSprites();
  if (impactFXSprites && impactFXSprites.length > 0) {
    this.getScene().getGameLayer().addNodes(impactFXSprites, {
      sourceScreenPosition: transformScreenToBoard(this.getSourceScreenPosition()),
      targetScreenPosition: transformScreenToBoard(this.getPosition())
    });
  }
}
```

**FXProjectileSprite** overrides:
```javascript
impactAtStart: false
impactAtEnd: true  // projectiles always impact at destination
```

## 3. Card → FX Mapping

Cards declare FX via factory methods:

```coffeescript
card.setFXResource(["FX.Cards.Neutral.GhostLynx"])
card.setFXResource(["FX.Cards.Faction2.StormKage"])
card.setFXResource(["FX.Cards.Artifact.LyonarRelic"])
```

FXResource is an **array of string paths** into the FX data structure.

## 4. Action → FX Selection

From `GameLayer._showActionForSource/Target` and FX filtering:

```javascript
// Attack FX layers (line 3968-3989):
fxData += dataForIdentifiersWithFilter(fxResources, FXType.UnitAttackedFX)
if (isPrimary):
  fxData += dataForIdentifiersWithFilter(fxResources, FXType.UnitPrimaryAttackedFX)
fxData += dataForIdentifiersWithFilter(union(attacker.getFXResource(), target.getFXResource()), 
                                        FXType.UnitDamagedFX)

// Spell FX (line 3939-3955):
fxData += dataForIdentifiersWithFilter(union(fxResource, card.getFXResource()), 
                                        FXType.SpellCastFX)
fxData += dataForIdentifiersWithFilter(fxResources, FXType.SpellAppliedFX)
if (isFriend):
  fxData += dataForIdentifiersWithFilter(fxResources, FXType.SpellAppliedFriendFX)
else:
  fxData += dataForIdentifiersWithFilter(fxResources, FXType.SpellAppliedEnemyFX)
if (damages):
  fxData += dataForIdentifiersWithFilter(fxResources, FXType.SpellDamagedFX)
if (heals):
  fxData += dataForIdentifiersWithFilter(fxResources, FXType.SpellHealedFX)

// Spawn FX (line 3961):
fxData += dataForIdentifiersWithFilter(union(fxResource, card.getFXResource()), 
                                        FXType.UnitSpawnFX)

// Death FX (line 3984):
fxData += dataForIdentifiersWithFilter(union(fxResource, target.getFXResource()), 
                                        FXType.UnitDiedFX)

// Modifier FX (line 4010-4014):
fxData += dataForIdentifiersWithFilter(modifier.getFXResource(), FXType.ModifierTriggeredFX)
fxData += dataForIdentifiersWithFilter(modifier.getFXResource(), FXType.ModifierTriggeredSourceFX)
fxData += dataForIdentifiersWithFilter(modifier.getFXResource(), FXType.ModifierTriggeredTargetFX)
```

FX selection uses **union** of multiple FX resources - combines attacker + target FX for richer visuals.

## 5. Attack Choreography

```javascript
// GameLayer orchestrates source and target
_showActionForSource(action, sourceSdkCard, sourceNode, fxSprites) {
  if (Entity):
    if (HealAction):
      sourceNode.showHealerState(action)
    else if (_getIsActionShowingAttackState(action)):
      sourceNode.showAttackState(action)
  
  sourceNode.showFX(fxSprites)  // spawn source FX
  eventBus.trigger(EVENTS.show_action_for_source, ...)
}

_showActionForTarget(action, targetSdkCard, targetNode, fxSprites) {
  if (DamageAction):
    targetNode.showAttackedState(action)
  else if (DieAction):
    targetNode.showDeathState(action)
  else if (HealAction):
    targetNode.showHealedState(action)
  else if (RemoveAction):
    targetNode.showDisappearState(action)
  else if (KillAction):
    targetNode.showDestroyedState(action)
  ...
}

_getIsActionShowingAttackState(action) {
  return (action instanceof AttackAction || action instanceof DamageAsAttackAction)
    || (action.getTriggeringModifierIndex() != null 
        && action.getTargetPosition() != null
        && (action instanceof DamageAction || action instanceof KillAction ...))
}
```

Attack delay added between source and target actions for proper synchronization.

## 6. Duration Accumulation Patterns

Every `show*` method returns `showDuration`:

```javascript
// Parallel (simultaneous) - use Math.max
showDuration = Math.max(showDuration, node.showTeleport(...))
showDuration = Math.max(showDuration, prismaticPlayCardNode.getShowDelay())
showDuration = Math.max(showDuration, this.bottomDeckLayer.showRemoveCard(i))

// Sequential (one after another) - use +=
showDuration += node.showMove(...)
showDuration += CONFIG.NOTIFICATION_DURATION + CONFIG.NOTIFICATION_TRANSITION_DURATION
```

Methods like `showMove`, `showTeleport`, `showDraw` all return their animation duration.

## 7. Critical Timing Constants

From `app/common/config.js`:

```javascript
// Core animation speeds
ENTITY_ATTACK_DURATION_MODIFIER = 1.0
ENTITY_MOVE_DURATION_MODIFIER = 1.0
ENTITY_MOVE_MODIFIER_MAX = 1.0
ENTITY_MOVE_MODIFIER_MIN = 0.75

// Standard fade timings
FADE_FAST_DURATION = ANIMATE_FAST_DURATION
FADE_MEDIUM_DURATION = ANIMATE_MEDIUM_DURATION
FADE_SLOW_DURATION = ANIMATE_SLOW_DURATION

// Movement
MOVE_FAST_DURATION = 0.15
MOVE_MEDIUM_DURATION = ANIMATE_MEDIUM_DURATION
MOVE_SLOW_DURATION = ANIMATE_SLOW_DURATION
PATH_MOVE_DURATION = 1.5

// UI feedback
ENTITY_STATS_CHANGE_DELAY = 0.75
NOTIFICATION_DURATION = 1.0
NOTIFICATION_TRANSITION_DURATION = 0.35

// Card play
OPPONENT_PLAYED_CARD_TRANSITION_DURATION = 1.0
OPPONENT_PLAYED_CARD_SHOW_DURATION = 2.0  // 1.0 + transition
OPPONENT_PLAYED_CARD_DELAY = 3.0  // show + 1.0

REPLAY_PLAYED_CARD_TRANSITION_DURATION = 1.5
REPLAY_PLAYED_CARD_SHOW_DURATION = 2.5  // 1.0 + transition
REPLAY_PLAYED_CARD_DELAY = 3.5  // show + 1.0

// Spell FX
PLAYED_SPELL_FX_FADE_IN_DURATION = 0.5
PLAYED_SPELL_FX_FADE_OUT_DURATION = 0.5

// Dialogue
DIALOGUE_OUT_OF_CARDS_DURATION = 1.0
DIALOGUE_HAND_FULL_DURATION = 1.0
DIALOGUE_RESIGN_DURATION = 2.0
DIALOGUE_ENTER_DURATION = 0.3
BURN_CARD_DISSOLVE_DURATION = 0.75

// Notifications
GAME_MAIN_NOTIFICATION_DURATION = 3.0
GAME_BATTLE_NOTIFICATION_DURATION = 5.0
GAME_PLAYER_NOTIFICATION_DURATION = 4.0
QUEST_NOTIFICATION_DURATION = 4
ACHIEVEMENT_NOTIFICATION_DURATION = 6
```

## 8. Complete Visual Flow Example: Unit Attack

```
1. Action queued: AttackAction
2. GameLayer.getFXDataForAction():
   - Filters UnitAttackedFX from attacker
   - Filters UnitPrimaryAttackedFX if primary attack
   - Filters UnitDamagedFX from union(attacker, target)
   - Returns fxData array
3. NodeFactory.createFX(fxData) → fxSprites[]
4. GameLayer._showActionForSource():
   - sourceNode.showAttackState(action)
     - Plays 'attack' animation
     - Speed modified by CONFIG.ENTITY_ATTACK_DURATION_MODIFIER
     - Returns to base state via showNextState()
   - sourceNode.showFX(fxSprites)
     - Spawns source FX (projectiles, impacts, etc)
5. [Attack delay added]
6. GameLayer._showActionForTarget():
   - targetNode.showAttackedState(action)
     - Plays 'damage' animation
     - Shows HP change
   - targetNode.showFX(fxSprites)
     - Spawns target FX (impacts, blood, etc)
7. If projectile FX with impactAtEnd=true:
   - Projectile travels to target
   - On end(), calls impact()
   - Spawns impactFX at destination
8. All durations accumulate via Math.max/+=
9. Returns total showDuration to step system
```

## 9. FXType Enum (Complete)

From `app/sdk/helpers/fxType.coffee`:

```coffeescript
# Units
UnitSpawnFX             # one time fx when unit spawns
UnitDiedFX              # one time fx when unit dies
UnitPrimaryAttackedFX   # unit's explicit attack only (e.g. blast shows once, not on strikeback)
UnitAttackedFX          # fx shown each time unit makes an attack
UnitDamagedFX           # one time fx when unit is damaged
UnitHealedFX            # one time fx when unit is healed

# Spells
SpellCastFX             # one time fx where spell is cast
SpellAutoFX             # pre-cast fx at each position spell does ANYTHING
SpellAppliedFX          # one time fx at each position spell does ANYTHING
# SpellAppliedFriendFX  # DISABLED - fx at positions affecting friendly units
# SpellAppliedEnemyFX   # DISABLED - fx at positions affecting enemy units
# SpellDamagedFX        # DISABLED - fx at positions where spell damages/kills
# SpellHealedFX         # DISABLED - fx at positions where spell heals

# Artifacts
ArtifactAppliedFX       # one time fx where artifact is applied
ArtifactFX              # continuous fx on entity with artifact

# Modifiers
ModifierFX              # persistent fx to altered unit
ModifierAppliedFX       # one time fx when modifier applied
ModifierTriggeredFX     # one time fx when modifier triggers
ModifierTriggeredSourceFX # one time fx to source when modifier triggers
ModifierTriggeredTargetFX # one time fx to target when modifier triggers
ModifierRemovedFX       # one time fx when modifier removed

# Actions (not usually used)
GameFX                  # one time fx from source position to target position
SourceFX                # one time fx at source position
TargetFX                # one time fx at target position
MoveSourceFX            # teleport fx at source before moving
MoveTargetFX            # teleport fx at target after moving
MoveFX                  # fx moving from source to target
```

**Note**: Several spell FX types are commented out/disabled (SpellAppliedFriendFX, SpellAppliedEnemyFX, SpellDamagedFX, SpellHealedFX). Only SpellAppliedFX is used.

## 10. EmitFX Timing System

From `FXSprite._calculateEmitFXTimings()`:

```javascript
_calculateEmitFXTimings() {
  this._emitFXLifeDuration = 0.0;
  this._emitFXShowDelay = 0.0;
  this._emitFXImpactDelay = 0.0;
  
  const baseDuration = this.getBaseDuration();
  const fxSprites = this.getEmitFXSprites();
  
  for (fxSprite in fxSprites) {
    // Lights inherit parent duration if not specified
    if (fxSprite instanceof Light) {
      if (!fxSprite.duration) {
        fxSprite.duration = baseDuration;
      }
    }
    
    // Track max durations of all emitted FX
    this._emitFXLifeDuration = Math.max(this._emitFXLifeDuration, 
                                         fxSprite.getLifeDuration(baseDuration));
    this._emitFXShowDelay = Math.max(this._emitFXShowDelay, 
                                      fxSprite.getShowDelay(baseDuration));
    this._emitFXImpactDelay = Math.max(this._emitFXImpactDelay, 
                                        fxSprite.getImpactDelay(baseDuration));
  }
}
```

**Key insight**: Child FX inherit parent's baseDuration when their own duration is 0 or not set. This allows lights/particles to automatically last as long as their parent animation.

## 11. Action Class Hierarchy

Attack-related actions:

```coffeescript
DamageAction                # base class for damage
├── AttackAction            # standard unit attack
├── DamageAsAttackAction    # damage that shows attack animation (used in Fight)
└── ForcedAttackAction      # extends AttackAction for forced attacks
```

`_getIsActionShowingAttackState()` triggers on:
- `AttackAction`
- `DamageAsAttackAction`
- `DamageAction` or `KillAction` with `triggeringModifierIndex` and `targetPosition` (modifier-triggered attacks)

## 12. Advanced FX Properties

Complete property examples from `app/data/fx.js`:

```javascript
// Continuous artifact glow with rotation
ArtifactFX: [{
  spriteIdentifier: RSX.decal_artifact.img,
  antiAlias: true,
  looping: true,
  colorByOwner: true,        // tint by player color
  scale: 1,
  opacity: 200,
  zOrder: -1,
  offset: { x: 0, y: -45 },
  xyzRotation: { x: 26, y: 0, z: 0 },
  xyzRotationPerSecond: { x: 0, y: 0, z: 45 },
  blendSrc: 'SRC_ALPHA',
  blendDst: 'ONE'            // additive blending
}]

// Spell with emitted light
SpellCastFX: [{
  spriteIdentifier: RSX.fx_f4_shadownova.name,
  emitFX: {                  // nested FX
    type: 'Light',
    offset: { x: 0, y: -45 },
    radius: 2000,
    intensity: 9,
    color: { r: 255, g: 0, b: 255 }
  }
}]

// Spell with color tint and reverse playback
SpellAppliedFX: [{
  spriteIdentifier: RSX.fxTendrilsGreen.name,
  offset: { x: 0, y: -15 },
  reverse: true,             // play animation backwards
  color: { r: 0, g: 200, b: 255 }
}]
```

### All Known FX Properties

**Visual**:
- `spriteIdentifier` - RSX resource name
- `color` - {r, g, b} color tint
- `colorByOwner` - tint by player color
- `opacity` - transparency (0-255)
- `scale` - uniform scale
- `antiAlias` - enable anti-aliasing
- `reverse` - play animation backwards
- `flippedX` - horizontal flip

**Position**:
- `offset` - {x, y} position offset
- `zOrder` - render order
- `xyzRotation` - {x, y, z} 3D rotation
- `xyzRotationPerSecond` - {x, y, z} rotation speed

**Blending**:
- `blendSrc` - source blend mode ('SRC_ALPHA', etc)
- `blendDst` - destination blend mode ('ONE', etc)

**Timing** (from base class):
- `duration` - extra lifetime
- `looping` - loop forever
- `fadeInDuration` / `fadeInDurationPct`
- `fadeOutDuration` / `fadeOutDurationPct`
- `impactAtStart` / `impactAtEnd`

**Sub-FX**:
- `emitFX` - nested FX object or array
- `impactFX` - FX to spawn on impact

**Lights**:
- `type: 'Light'`
- `radius` - light radius
- `intensity` - brightness
- `castsShadows` - shadow casting

**Particles**:
- `type: 'Particles'`
- `plistFile` - particle system file

**Decals**:
- `type: 'Decal'`
- Projects onto ground

## 13. FX Identifier Resolution

From `app/data/data.coffee`:

```coffeescript
# Path-based lookup with caching
DATA.dataForIdentifier("FX.Cards.Neutral.GhostLynx")
  → splits on "." → DATA[FX][Cards][Neutral][GhostLynx]
  → caches result in _cache["FX.Cards.Neutral.GhostLynx"]

# Array of identifiers
DATA.dataForIdentifiers(["FX.Cards.Neutral.GhostLynx", "FX.Factions.Faction1"])
  → returns concatenated array of all found data

# Identifier + filter key
DATA.dataForIdentifiersWithFilter(identifiers, filterKeys, foundFilterKeys={})
```

### Filter Algorithm (Critical)

```coffeescript
dataForIdentifiersWithFilter(
  identifiers: ["FX.Cards.Neutral.GhostLynx", "FX.Factions.Faction1"],
  filterKeys: ["UnitSpawnFX", "UnitAttackedFX"]
)

# For each identifier (BACKWARDS):
for identifier in identifiers by -1:
  lastKey = last segment of identifier
  
  for filterKey in filterKeys:
    if not foundFilterKeys[filterKey]:  # Only find each filter once
      
      # Build filtered identifier
      if lastKey == filterKey:
        filteredIdentifier = identifier  # Already ends with filter
      else:
        filteredIdentifier = identifier + "." + filterKey
      
      # Example: "FX.Cards.Neutral.GhostLynx" + "UnitSpawnFX"
      #       → "FX.Cards.Neutral.GhostLynx.UnitSpawnFX"
      
      datum = _filterCache[filteredIdentifier] OR dataForIdentifier(filteredIdentifier)
      
      if datum exists:
        foundFilterKeys[filterKey] = true  # Mark as found
        data.concat(datum)

return data
```

**Key behaviors**:
- Works **backwards** through identifiers (highest priority first)
- Each filter key found **only once** (prevents duplicates)
- Caches filtered lookups separately in `_filterCache`
- Short-circuits: if `lastKey == filterKey`, uses identifier as-is

**Example flow**:
```javascript
// Card has: fxResource = ["FX.Cards.Faction2.StormKage"]
// Target has: fxResource = ["FX.Factions.Faction1"]

dataForIdentifiersWithFilter(
  union(attacker.getFXResource(), target.getFXResource()),
  FXType.UnitDamagedFX
)

// Searches (backwards):
1. "FX.Factions.Faction1.UnitDamagedFX" → finds Faction1 damage FX
   foundFilterKeys["UnitDamagedFX"] = true
2. "FX.Cards.Faction2.StormKage.UnitDamagedFX" → skipped (already found)

// Returns: [Faction1.UnitDamagedFX] (not StormKage-specific damage FX)
```

## 14. NodeFactory.createFX

From `app/view/helpers/NodeFactory.js`:

```javascript
createFX(fx, options) {
  // options = {
  //   sourceBoardPosition: {x, y},
  //   targetBoardPosition: {x, y},
  //   noLimit: false  // bypass max FX limit
  // }
  
  // Convert board positions to screen positions
  sourceBoardPosition = options.sourceBoardPosition || targetBoardPosition
  targetBoardPosition = options.targetBoardPosition || sourceBoardPosition
  sourceScreenPosition = transformBoardToScreen(sourceBoardPosition)
  targetScreenPosition = transformBoardToScreen(targetBoardPosition)
  
  fxSprites = []
  
  if (Array.isArray(fx)):
    for each fx object:
      _createFXSprites(fx[i], options, fxSprites, ...)
  else:
    _createFXSprites(fx, options, fxSprites, ...)
  
  return fxSprites
}

_createFXSprites(fx, options, fxSprites, ...) {
  // Determine FX sprite class
  type = fx.type || 'FXSprite'
  fxSpriteClass = getByType(type)  // FXSprite, FXProjectileSprite, etc.
  
  // Determine positioning behavior
  if (fx.sourceToTarget):
    startPosition = sourcePosition
    endPosition = targetPosition
  else if (fx.targetToSource):
    startPosition = targetPosition
    endPosition = sourcePosition
  else if (fx.source):
    startPosition = endPosition = sourcePosition
  else:  # DEFAULT
    startPosition = endPosition = targetPosition
  
  // Apply offsets
  if (fx.offset):
    startScreenOffset = endScreenOffset = fx.offset
  if (fx.sourceOffset):
    startScreenOffset = fx.sourceOffset
  if (fx.targetOffset):
    endScreenOffset = fx.targetOffset
  
  _createFXSprite(fxSpriteClass, fx, options, fxSprites, ...)
}

_createFXSprite(fxSpriteClass, fx, options, fxSprites, 
                startBoardPosition, endBoardPosition, ...) {
  // Create sprite via class factory method
  fxSprite = fxSpriteClass.create(fx, options)
  
  // Set positions
  fxSprite.sourceBoardPosition = startBoardPosition
  fxSprite.targetBoardPosition = endBoardPosition
  fxSprite.sourceScreenPosition = startScreenPosition
  fxSprite.targetScreenPosition = endScreenPosition
  
  // Apply screen offsets
  if (startScreenOffset):
    fxSprite.sourceScreenPosition += startScreenOffset
  if (endScreenOffset):
    fxSprite.targetScreenPosition += endScreenOffset
  
  // Recursively create sub-FX
  if (fx.impactFX):
    fxSprite.impactFX = createFX(fx.impactFX, options)
  if (fx.emitFX):
    fxSprite.emitFX = createFX(fx.emitFX, options)
  
  fxSprites.push(fxSprite)
}
```

### FX Object Structure (Complete)

```javascript
{
  // Sprite class selection
  type: "FXSprite",  // or "Explosion", "Projectile", "Light", etc.
  
  // Positioning modes (mutually exclusive)
  sourceToTarget: true,   // start at source, end at target
  targetToSource: true,   // start at target, end at source
  source: true,           // start and end at source
  // default (none): start and end at target
  
  // Position offsets
  offset: {x, y},         // both start and end
  sourceOffset: {x, y},   // start only
  targetOffset: {x, y},   // end only
  
  // Sub-FX (recursive)
  impactFX: {...},        // spawned on impact()
  emitFX: {...},          // spawned during lifetime
  
  // Layer routing (rarely used)
  layerName: "middlegroundLayer",
  destinationLayerName: "backgroundLayer",
  
  // All FXSprite base properties
  spriteIdentifier: RSX.fx_name.name,
  color: {r, g, b},
  duration: 0.0,
  looping: false,
  // ... etc (see section 2)
}
```

## 15. FX Timing Chain

From `FXSprite` timing methods:

```javascript
// Life duration = how long FX exists
getLifeDuration(baseDuration?) {
  duration = getBaseDuration()
  duration = Math.max(duration, _emitFXLifeDuration)
  return duration
}

// Show delay = how long until all sub-FX complete
getShowDelay(baseDuration?) {
  delay = getBaseDuration()
  delay += getImpactFXShowDelay()  // add impact FX time
  return delay
}

getImpactFXShowDelay() {
  return Math.max(_impactFXShowDelay, _impactFXImpactDelay)
}

// Impact delay = when to spawn impact FX
getImpactDelay(baseDuration?) {
  if (impactAtEnd):
    return getBaseDuration()  // wait for animation
  else:
    return 0.0  // impact immediately
}

// Calculate emit FX timings
_calculateEmitFXTimings() {
  baseDuration = getBaseDuration()
  
  for each emitted FX sprite:
    if (Light without duration):
      sprite.duration = baseDuration  # inherit
    
    _emitFXLifeDuration = max(_emitFXLifeDuration, 
                               sprite.getLifeDuration(baseDuration))
    _emitFXShowDelay = max(_emitFXShowDelay, 
                            sprite.getShowDelay(baseDuration))
    _emitFXImpactDelay = max(_emitFXImpactDelay, 
                              sprite.getImpactDelay(baseDuration))
}

// Calculate impact FX timings (similar)
_calculateImpactFXTimings() {
  for each impact FX sprite:
    _impactFXLifeDuration = max(...)
    _impactFXShowDelay = max(...)
    _impactFXImpactDelay = max(...)
}
```

### Timing Flow

```
FX Sprite Created
  ↓
_calculateEmitFXTimings()  # calc child FX durations
  ↓
_calculateImpactFXTimings()
  ↓
start()
  ├→ startTransform()
  ├→ startEmit()        # spawn emitFX
  ├→ startAnimation()
  └→ startEvents()
  ↓
[animation plays for baseDuration]
  ↓
emitFX continue for _emitFXLifeDuration
  ↓
end()
  ├→ if (impactAtEnd): impact()  # spawn impactFX
  └→ if (removeOnEnd): destroy()
  ↓
impactFX play for _impactFXShowDelay + _impactFXImpactDelay
  ↓
All FX complete
```

**Duration accumulation example**:
```javascript
// Projectile with impact explosion and light
projectile = {
  type: "Projectile",
  spriteIdentifier: "fireball",
  impactAtEnd: true,
  impactFX: {
    type: "Explosion",
    spriteIdentifier: "explosion",
    duration: 0.5,
    emitFX: {
      type: "Light",
      radius: 200,
      duration: 0.0  // will inherit explosion's baseDuration
    }
  }
}

// Timings:
projectile.getBaseDuration() = 1.0 (fireball travel time)
explosion.getBaseDuration() = 0.8 (explosion anim)
explosion.duration = 0.5 (extra hold time)
explosion.getLifeDuration() = 0.8 + 0.5 = 1.3
light.duration = 0.8 (inherited from explosion baseDuration)

projectile.getImpactDelay() = 1.0 (impactAtEnd)
projectile.getShowDelay() = 1.0 + max(1.3, 0) = 2.3

// Total visual duration: 2.3 seconds
```

## Complete FX Pipeline Summary

```
1. Action occurs (AttackAction, DamageAction, etc)
   ↓
2. GameLayer.getFXDataForAction(action)
   ├→ Gets attacker/target fxResource arrays
   ├→ Filters by FXType (UnitAttackedFX, UnitDamagedFX, etc)
   └→ Returns fxData array (via dataForIdentifiersWithFilter)
   ↓
3. NodeFactory.createFX(fxData, options)
   ├→ For each FX definition:
   │  ├→ Determines FX sprite class (FXSprite, FXProjectileSprite, etc)
   │  ├→ Calculates positions (source/target with offsets)
   │  ├→ Creates sprite via class.create()
   │  └→ Recursively creates impactFX and emitFX
   └→ Returns fxSprites array
   ↓
4. GameLayer._showActionForSource/Target(fxSprites)
   ├→ sourceNode.showAttackState()  # play attack animation
   ├→ sourceNode.showFX(fxSprites)  # spawn source FX
   └→ targetNode.showAttackedState() # play hit reaction
   ↓
5. FXSprite lifecycle
   ├→ _calculateEmitFXTimings()
   ├→ _calculateImpactFXTimings()
   ├→ start() → emit emitFX → play animation
   ├→ end() → if impactAtEnd: impact() → spawn impactFX
   └→ All sub-FX complete
   ↓
6. Duration bubbles up
   └→ Returns showDuration to GameLayer step system
```
