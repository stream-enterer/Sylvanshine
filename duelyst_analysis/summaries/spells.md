# System: Spells

**Location:** app/sdk/spells/

## Purpose
The Spell system provides instant-cast cards that apply effects to targets, including damage, buffs, summons, and board manipulation without persistent board presence.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| Spell | spells/spell.coffee | Base spell class |
| SpellFactory | Various factory files | Spell definitions |
| SpellFilterType | spells/spellFilterType.coffee | Target filtering |
| SpellApply* | spells/spellApply*.coffee | Effect application |
| SpellDamage* | spells/spellDamage*.coffee | Damage spells |
| SpellSpawn* | spells/spellSpawn*.coffee | Summon spells |

## Data Flow
**Input:** Player cast, target selection, followup
**Processing:** Target validation → Effect resolution → State changes
**Output:** Damage, buffs, summons, board changes

## Dependencies
**Requires:** Card system, Action system, Modifier system
**Used by:** GameSession, Player, AI

## Spell Directory Structure
```
spells/
├── spell.coffee                    # Base class
├── spellFilterType.coffee          # Target filters
├── spellApply*.coffee              # Apply effects
├── spellDamage*.coffee             # Damage effects
├── spellSpawn*.coffee              # Summon effects
├── spellIntensify*.coffee          # Intensify mechanic
├── spellRemove*.coffee             # Removal effects
├── spellFollowup*.coffee           # Followup spells
└── [200+ spell implementations]
```

## Spell Categories
| Category | Pattern | Examples |
|----------|---------|----------|
| Damage | SpellDamage* | Direct damage, AoE |
| Buffs | SpellApplyModifiers | Stat buffs, keywords |
| Summons | SpellSpawn* | Summon units/tiles |
| Removal | SpellRemove*, SpellKill* | Destroy, dispel |
| Movement | SpellTeleport*, SpellSwap* | Repositioning |
| Draw | SpellDraw* | Card draw effects |
| Heal | SpellHeal* | Restore HP |
| Transform | SpellTransform* | Change units |
| Board | SpellTile* | Ground effects |

## Target Filtering
| Filter | Targets |
|--------|---------|
| NeutralDirect | All units |
| EnemyDirect | Enemy units only |
| AllyDirect | Friendly units only |
| Self | Caster only |
| SpawnSource | Spawn positions |
| None | No targeting |

## Spell Mechanics
| Mechanic | Description |
|----------|-------------|
| Followup | Additional targeting step |
| Intensify | Scales with cast count |
| Cantrip | Draw a card |
| Echo | Replays on same target |
| Bloodbound | General ability (BBS) |

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.FOLLOWUP_FX_TEMPLATE | Array | Followup visual |
| CONFIG.GENERAL_CAST_START_DELAY | 0.25 | Cast animation start |
| CONFIG.GENERAL_CAST_END_DELAY | 0.5 | Cast animation end |

## Resources
| Pattern | Purpose |
|---------|---------|
| fx_f{n}_* | Faction spell effects |
| sfx_spell_* | Spell sound effects |
| resources/fx/ | Effect sprites |

## Statistics
- **301 spells** in instances/spells.tsv
- Spell breakdown by faction:
  - Lyonar: ~45
  - Songhai: ~45
  - Vetruvian: ~45
  - Abyssian: ~45
  - Magmar: ~45
  - Vanar: ~45
  - Neutral: ~30
- Common spell types:
  - Damage spells: ~60
  - Buff spells: ~50
  - Summon spells: ~40
  - Removal spells: ~30
  - Draw spells: ~20
  - Other: ~100

## Bloodbound Spells (BBS)
Each general has a unique signature spell:
| General | BBS Name | Effect |
|---------|----------|--------|
| Argeon | Roar | +2 Attack to minion |
| Ziran | Afterglow | Heal 3 |
| Kaleos | Blink | Teleport minion |
| Reva | Heartseeker | Summon 1/1 Ranged |
| Zirix | Iron Dervish | Summon 2/2 Rush |
| Sajj | Siphon Energy | Dispel nearby |
| Lilithe | Wraithlings | Summon 1/1s |
| Cassyva | Abyssal Scar | Damage + Creep |
| Vaath | Overload | +1 Attack permanent |
| Starhorn | Thumping Wave | Both draw |
| Faie | Warbird | 2 damage to column |
| Kara | Kinetic Surge | +1/+1 to hand |
