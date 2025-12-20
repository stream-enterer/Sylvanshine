# Flow: Artifact Equip

## Trigger
Player plays an artifact card, equipping it to their general.

**Entry Points:**
- `PlayCardFromHandAction` with artifact card
- `PlayCardSilentlyAction` for programmatic equip
- Dying wish/opening gambit artifact effects

---

## Timeline

| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | Play artifact | `app/sdk/actions/playCardFromHandAction.coffee` | Artifact card played |
| T+0.0s | Apply to board | `app/sdk/artifacts/artifact.coffee:38` | `onApplyToBoard()` called |
| T+0.0s | Get general | `app/sdk/artifacts/artifact.coffee:44` | Find owner's general |
| T+0.0s | Get existing artifacts | `app/sdk/artifacts/artifact.coffee:45` | `getArtifactModifiersGroupedByArtifactCard()` |
| T+0.0s | Check slot limit | `app/sdk/artifacts/artifact.coffee:47` | Compare to CONFIG.MAX_ARTIFACTS |
| T+0.0s | Remove oldest | `app/sdk/artifacts/artifact.coffee:48-50` | FIFO removal if at max |
| T+0.0s | Prepare modifiers | `app/sdk/artifacts/artifact.coffee:53-62` | Set durability, flags |
| T+0.0s | Apply modifiers | `app/sdk/artifacts/artifact.coffee:65` | `applyModifierContextObject()` |
| T+0.0s | ApplyModifierAction | `app/sdk/actions/applyModifierAction.coffee:72-97` | Execute modifier application |
| T+0.0s | Modifier attached | `app/sdk/gameSession.coffee:3215-3285` | Modifier added to general |
| T+0.0s | Equip SFX | Resource system | `sfx_artifact_equip.audio` |
| T+0.5s | Artifact UI update | View layer | Show in artifact slots |

**Durability Loss Timeline (on damage):**
| Time | Event | Code Location | Action |
|------|-------|---------------|--------|
| T+0.0s | General takes damage | `DamageAction` executes | Damage to general |
| T+0.0s | Modifier onAction | `app/sdk/modifiers/modifier.coffee:2085-2090` | Detect damage to card |
| T+0.0s | Apply durability loss | `app/sdk/modifiers/modifier.coffee:1891-1896` | `applyDamage()` |
| T+0.0s | Check destroyed | `app/sdk/modifiers/modifier.coffee:1898-1899` | `getIsDestroyed()` |
| T+0.0s | Remove if destroyed | Modifier removal | Durability <= 0 |
| T+0.25s | Hit FX | `app/view/nodes/cards/ArtifactNode.js:530` | `fxArtifactHit` |
| T+0.5s | Break FX (if destroyed) | `app/view/nodes/cards/ArtifactNode.js:446` | `fxArtifactBreak` |

---

## System Interactions

| Phase | System | Action | Key Code |
|-------|--------|--------|----------|
| Play | PlayCardFromHandAction | Play artifact card | `app/sdk/actions/playCardFromHandAction.coffee` |
| Apply | Artifact | onApplyToBoard callback | `app/sdk/artifacts/artifact.coffee:38-66` |
| Slot Check | Artifact | Check max artifacts | `app/sdk/artifacts/artifact.coffee:47` |
| Overflow | Artifact | Remove oldest (FIFO) | `app/sdk/artifacts/artifact.coffee:48-50` |
| Modifier Setup | Artifact | Configure modifier flags | `app/sdk/artifacts/artifact.coffee:53-62` |
| Apply | GameSession | applyModifierContextObject | `app/sdk/gameSession.coffee:3215-3285` |
| Execute | ApplyModifierAction | Execute application | `app/sdk/actions/applyModifierAction.coffee:72-97` |
| Attach | Card | Add modifier to general | `app/sdk/cards/card.coffee:onAddModifier()` |
| Durability | Modifier | Track damage/charges | `app/sdk/modifiers/modifier.coffee:1876-1899` |
| View | ArtifactNode | Display artifact UI | `app/view/nodes/cards/ArtifactNode.js` |

---

## Data Transformations

**Input:** Artifact played
```javascript
{
  artifactCard: {
    type: CardType.Artifact,
    durability: 3,
    targetModifiersContextObjects: [
      { type: "ModifierProvoke", ... },
      { type: "ModifierFirstBlood", ... }
    ]
  },
  general: {
    artifactModifiers: [existingArtifact1, existingArtifact2]
  },
  artifactSlots: 2  // Currently equipped
}
```

**Output:** Artifact equipped
```javascript
{
  general: {
    artifactModifiers: [
      existingArtifact1,
      existingArtifact2,
      newArtifactModifier1,  // Provoke
      newArtifactModifier2   // First Blood
    ]
  },
  artifactSlots: 3,
  newArtifactDurability: 3,
  modifiersApplied: [
    {
      isHiddenToUI: true,
      isRemovable: false,
      maxDurability: 3,
      durability: 3,
      sourceCard: artifactCard
    }
  ]
}
```

**Output (Slot Overflow):**
```javascript
{
  // If was at 3 artifacts, oldest removed
  removedArtifact: existingArtifact1,  // FIFO - oldest removed
  general: {
    artifactModifiers: [
      existingArtifact2,
      existingArtifact3,  // Was newest, now middle
      newArtifactModifier  // New artifact
    ]
  },
  artifactSlots: 3  // Still at max
}
```

---

## Timing Constants

| Constant | Value | Location | Purpose |
|----------|-------|----------|---------|
| `CONFIG.MAX_ARTIFACTS` | 3 | `app/common/config.js:343` | Maximum equipped artifacts |
| `CONFIG.MAX_ARTIFACT_DURABILITY` | 3 | `app/common/config.js:345` | Default durability/charges |

**Durability Properties (modifier.coffee:1876-1899):**
```coffeescript
maxDurability: 0      # Maximum charges
durability: 0         # Current charges

getIsFromArtifact: () ->
  return @getMaxDurability() > 0

applyDamage: (dmg) ->
  @durability -= 1    # Always lose 1 charge per hit

getIsDestroyed: () ->
  return @durability <= 0
```

---

## FX/Sound Events

| Event | FX | Sound | Trigger Point |
|-------|------|-------|---------------|
| Artifact Equip | - | `sfx_artifact_equip.audio` | `resources.js:3926` |
| Artifact Equip (Mask) | - | `sfx_artifact_equipmask.audio` | `resources.js:3927` |
| Artifact Equip (Weapon) | - | `sfx_artifact_equipweapon.audio` | `resources.js:3928` |
| Artifact Hit | `fxArtifactHit` | - | `ArtifactNode.js:530` |
| Artifact Break | `fxArtifactBreak` | - | `ArtifactNode.js:446` |
| Refresh Charges | `FX.Actions.RefreshArtifacts` | - | `refreshArtifactChargesAction.coffee:7` |

**FX Resources (resources.js:689-693):**
```javascript
fxArtifactBreak: {
  name: 'fxArtifactBreak',
  img: 'resources/fx/fx_artifactbreak.png',
  framePrefix: 'fx_artifactbreak_',
  frameDelay: 0.08
}

fxArtifactHit: {
  name: 'fxArtifactHit',
  img: 'resources/fx/fx_artifacthit.png',
  framePrefix: 'fx_artifacthit_',
  frameDelay: 0.08
}
```

**Refresh Artifacts FX (fx.js:8744-8752):**
```javascript
RefreshArtifacts: {
  SpellAppliedFX: [
    { spriteIdentifier: RSX.ForceField.name, offset: { x: 0, y: -4.5 } },
    { spriteIdentifier: RSX.fxBuffSimpleGold.name }
  ]
}
```

**Per-Artifact Sound Setup (faction1.coffee:1161-1162):**
```coffeescript
card.setBaseSoundResource(
  apply: RSX.sfx_victory_crest.audio
)
```

---

## Error Cases

| Error | Detection Point | Handling | User Feedback |
|-------|----------------|----------|---------------|
| No general | General lookup | Action fails | N/A |
| Invalid artifact | Card type check | Action rejected | N/A |
| Slot overflow | Count check | Remove oldest artifact | N/A (automatic) |
| Durability 0 | Damage check | Modifier removed | Artifact break FX |

---

## Artifact Class Implementation

**Location:** `app/sdk/artifacts/artifact.coffee`

```coffeescript
class Artifact extends Card
  type: CardType.Artifact
  targetModifiersContextObjects: null  # Modifiers to apply to general
  durability: CONFIG.MAX_ARTIFACT_DURABILITY  # Default 3

  onApplyToBoard: (board, x, y, sourceAction) ->
    super(board, x, y, sourceAction)

    # Get owner's general
    general = @getGameSession().getGeneralForPlayerId(@getOwnerId())

    # Get existing artifacts grouped by source card
    modifiersByArtifact = general.getArtifactModifiersGroupedByArtifactCard()

    # Check slot limit (FIFO removal)
    if modifiersByArtifact.length >= CONFIG.MAX_ARTIFACTS
      # Remove OLDEST artifact's modifiers
      artifactModifiers = modifiersByArtifact.shift()
      for modifier in artifactModifiers
        @getGameSession().removeModifier(modifier)

    # Apply each modifier to general
    for modifierContextObject in @targetModifiersContextObjects
      # Configure modifier properties
      modifierContextObject.isHiddenToUI = true      # Don't show in UI
      modifierContextObject.isRemovable = false      # Can't be dispelled
      modifierContextObject.maxDurability = @durability
      modifierContextObject.durability = @durability

      # Apply to general
      @getGameSession().applyModifierContextObject(modifierContextObject, general)
```

---

## ApplyModifierAction Implementation

**Location:** `app/sdk/actions/applyModifierAction.coffee:72-97`

```coffeescript
_execute: () ->
  target = @getTarget()

  if @modifierContextObject?
    # Create modifier from context object
    modifier = @getGameSession().getOrCreateModifierFromContextObject(@modifierContextObject)

    # Apply via GameSession
    @getGameSession().p_applyModifier(
      modifier,
      target,
      @parentModifier,
      @modifierContextObject,
      @auraModifierId
    )

    # Store applied modifier index
    @modifierIndex = modifier.getIndex()
```

**GameSession.applyModifierContextObject (gameSession.coffee:3215-3285):**
```coffeescript
applyModifierContextObject: (modifierContextObject, card, parentModifier) ->
  # Create ApplyModifierAction if actions executing
  if @getIsRunningAsAuthoritative() and @_private.actionsCurrentlyExecuting
    action = new ApplyModifierAction(@)
    action.setTarget(card)
    action.modifierContextObject = modifierContextObject
    action.parentModifier = parentModifier
    @executeAction(action)
  else
    # Direct application
    @p_applyModifier(modifier, card, parentModifier, modifierContextObject)
```

---

## Durability System

**Location:** `app/sdk/modifiers/modifier.coffee`

**Durability Check on Damage (modifier.coffee:2085-2090):**
```coffeescript
_onAction: (event) ->
  action = event.action
  # If this modifier has durability and general takes damage
  if @maxDurability > 0 and
     action instanceof DamageAction and
     action.getTarget() == @getCard()
    @applyDamage(action.getTotalDamageAmount())
```

**Apply Damage (modifier.coffee:1891-1896):**
```coffeescript
applyDamage: (dmg) ->
  # Always reduce by 1, regardless of damage amount
  @durability -= 1
  # Note: Artifact loses 1 charge per hit, not per damage point
```

**Check Destroyed (modifier.coffee:1898-1899):**
```coffeescript
getIsDestroyed: () ->
  return @durability <= 0
```

---

## Artifact Modifier Grouping

**Location:** `app/sdk/cards/card.coffee`

**Get Artifact Modifiers (card.coffee:1831-1835):**
```coffeescript
getArtifactModifiers: () ->
  modifiers = []
  for m in @getModifiers()
    if m? and m.getIsFromArtifact()
      modifiers.push(m)
  return modifiers
```

**Group by Source Card (card.coffee:1837-1838):**
```coffeescript
getArtifactModifiersGroupedByArtifactCard: () ->
  return UtilsGameSession.groupModifiersBySourceCard(@getArtifactModifiers())
```

---

## Artifact Actions

**RefreshArtifactChargesAction (refreshArtifactChargesAction.coffee:13-22):**
```coffeescript
class RefreshArtifactChargesAction extends Action
  fxResource: ["FX.Actions.RefreshArtifacts"]

  _execute: () ->
    general = @getTarget()
    for modifier in general.getArtifactModifiers()
      # Reset durability to max
      modifier.durability = modifier.maxDurability
```

**RestoreChargeToAllArtifactsAction (restoreChargeToAllArtifactsAction.coffee:12-22):**
```coffeescript
class RestoreChargeToAllArtifactsAction extends Action
  fxResource: ["FX.Actions.RefreshArtifacts"]

  _execute: () ->
    general = @getTarget()
    for modifier in general.getArtifactModifiers()
      # Add 1 durability (up to max)
      if modifier.durability < modifier.maxDurability
        modifier.durability += 1
```

**RemoveArtifactsAction (removeArtifactsAction.coffee:12-22):**
```coffeescript
class RemoveArtifactsAction extends Action
  _execute: () ->
    general = @getTarget()
    for modifier in general.getArtifactModifiers()
      @getGameSession().removeModifier(modifier)
```

**RemoveRandomArtifactAction (removeRandomArtifactAction.coffee:13-30):**
```coffeescript
class RemoveRandomArtifactAction extends Action
  _execute: () ->
    general = @getTarget()
    artifactGroups = general.getArtifactModifiersGroupedByArtifactCard()

    if artifactGroups.length > 0
      # Pick random artifact
      randomGroup = artifactGroups[Math.floor(Math.random() * artifactGroups.length)]
      # Remove all modifiers from that artifact
      for modifier in randomGroup
        @getGameSession().removeModifier(modifier)
```

---

## Example Artifacts

**Skywind Glaives (faction1.coffee:1142-1163):**
```coffeescript
# Artifact that gives +2 attack aura to nearby allies
card = new Artifact(gameSession)
card.setFactionId(Factions.Faction1)
card.setManaCost(3)

# Durability
durability = 3

# Aura modifier for nearby allies
attackBuffContextObject = Modifier.createContextObjectWithAttributeBuffs(2, 0)
auraContextObject = Modifier.createContextObjectWithAuraForNearbyAllies(
  [attackBuffContextObject],
  { ... }
)
auraContextObject.maxDurability = durability
auraContextObject.durability = durability

card.targetModifiersContextObjects = [auraContextObject]
```

**Sunstone Bracers (faction1.coffee:1165-1187):**
```coffeescript
# Simple +1 attack to general
card = new Artifact(gameSession)
card.setManaCost(1)

durability = 3
buffContextObject = Modifier.createContextObjectWithAttributeBuffs(1, 0)
buffContextObject.maxDurability = durability
buffContextObject.durability = durability

card.targetModifiersContextObjects = [buffContextObject]
```

---

## Artifact Modifier Flags

**Properties Set During Equip (artifact.coffee:53-62):**

| Property | Value | Purpose |
|----------|-------|---------|
| `isHiddenToUI` | `true` | Don't show in modifier list |
| `isRemovable` | `false` | Cannot be dispelled |
| `maxDurability` | artifact.durability | Maximum charges |
| `durability` | artifact.durability | Current charges |

**Note:** `isRemovable: false` means standard dispel effects cannot remove artifact modifiers. They can only be removed by:
- Durability reaching 0
- Slot overflow (FIFO)
- Specific artifact removal effects

---

## Artifact Equip Flow Summary

```
1. Artifact Played
   ├── PlayCardFromHandAction executes
   └── Artifact.onApplyToBoard() called

2. Slot Management
   ├── Get general's existing artifact modifiers
   ├── Group by source artifact card
   ├── If count >= CONFIG.MAX_ARTIFACTS (3)
   │   └── Remove OLDEST artifact (FIFO)
   └── Proceed with equip

3. Modifier Configuration
   ├── For each modifier in targetModifiersContextObjects:
   │   ├── Set isHiddenToUI = true
   │   ├── Set isRemovable = false
   │   ├── Set maxDurability = artifact.durability
   │   └── Set durability = artifact.durability
   └── Apply via applyModifierContextObject()

4. ApplyModifierAction
   ├── Create modifier from context object
   ├── p_applyModifier() attaches to general
   └── Modifier now active on general

5. Durability Tracking
   ├── On any damage to general:
   │   ├── Modifier._onAction() detects DamageAction
   │   ├── applyDamage() reduces durability by 1
   │   └── If durability <= 0, modifier removed
   └── Show fxArtifactHit / fxArtifactBreak

6. Audio/Visual
   ├── Play sfx_artifact_equip sound
   └── Update artifact slots in UI
```
