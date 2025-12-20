# Artifact

## Source Evidence
- Primary class: `Artifact` (app/sdk/artifacts/artifact.coffee)
- Related classes: Card, Modifier
- Data shape: Equipment cards attached to Generals

## Fields
| Field | Type | Source | Required? |
|-------|------|--------|-----------|
| durability | number | classes.tsv | yes |
| maxDurability | number | config (default: 3) | yes |
| targetModifiersContextObjects | array | artifact config | no |
| modifiersContextObjects | array | artifact config | no |

## Lifecycle Events
- created: PlayCardAction (equip to general)
- destroyed: durability reaches 0, RemoveArtifactsAction
- modified: attack events (durability decreases)

## Dependencies
- Requires: Card, Player/General, Modifier
- Used by: GameSession, ModifierArtifact

## Constants (from config.js)
| Constant | Value | Description |
|----------|-------|-------------|
| MAX_ARTIFACT_DURABILITY | 3 | Default durability |
| MAX_ARTIFACTS | 3 | Max equipped at once |

## Artifact Types by Faction
| Faction | Artifacts |
|---------|-----------|
| Lyonar | Skywind Glaives, Arclyte Regalia, Sunstone Bracers |
| Songhai | Cyclone Mask, Bloodrage Mask, Mask of Shadows |
| Vetruvian | Staff of Y'Kir, Wildfire Ankh, Hexblade |
| Abyssian | Spectral Blade, Soul Grimoire, Horn of the Forsaken |
| Magmar | Adamantite Claws, Iridium Scale, Morin-Khur |
| Vanar | Winterblade, Snowpiercer, White Asp |
| Neutral | Mindlathe, Astral Crusader weapons |

## Key Methods
- `onEquip` - Called when artifact is equipped
- `onUnequip` - Called when artifact is removed
- `getDurability` - Get remaining durability
- `reduceDurability` - Decrease durability after attack

## Description
Artifacts are equipment cards that attach to the player's General. They provide passive effects and typically lose durability when the General attacks. Each player can have up to 3 artifacts equipped simultaneously. Artifacts are removed when their durability reaches 0 or via specific removal effects.
