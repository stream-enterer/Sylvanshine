# System: Audio

**Location:** app/audio/, app/resources/sfx/

## Purpose
The Audio system manages all sound effects, music, and voice playback through priority-based audio management and volume controls.

## Key Components
| Component | File | Purpose |
|-----------|------|---------|
| audio_manager | audio/*.coffee | Audio playback control |
| SoundManager | ui/managers/sound_manager.js | Volume/mute settings |
| resources/sfx/ | sfx/*.ogg | Sound effect files |

## Data Flow
**Input:** Game events, UI interactions, music cues
**Processing:** Priority checking → Volume scaling → Playback
**Output:** Audio playback through Web Audio API

## Dependencies
**Requires:** Web Audio API, Resources system
**Used by:** View system, UI system, Game events

## Configuration
| Constant | Value | Purpose |
|----------|-------|---------|
| CONFIG.DEFAULT_MASTER_VOLUME | 1.0 | Master volume |
| CONFIG.DEFAULT_MUSIC_VOLUME | 0.04 | Background music |
| CONFIG.DEFAULT_SFX_VOLUME | 0.3 | Sound effects |
| CONFIG.DEFAULT_VOICE_VOLUME | 0.3 | Voice/speech |
| CONFIG.DEFAULT_SFX_PRIORITY | 0 | Default priority |
| CONFIG.CLICK_SFX_PRIORITY | 1 | UI click sounds |
| CONFIG.CANCEL_SFX_PRIORITY | 3 | Cancel sounds |
| CONFIG.CONFIRM_SFX_PRIORITY | 4 | Confirm sounds |
| CONFIG.HIDE_SFX_PRIORITY | 6 | Hide animations |
| CONFIG.ERROR_SFX_PRIORITY | 7 | Error sounds |

## Sound Categories
| Category | Pattern | Examples |
|----------|---------|----------|
| Spell Effects | sfx_spell_* | sfx_spell_phoenixfire, sfx_spell_truestrike |
| Unit Sounds | sfx_f[1-6]_* | Attack, death, movement per unit |
| Neutral Units | sfx_neutral_* | sfx_neutral_golem_attack |
| UI Sounds | sfx_ui_* | Click, confirm, error |
| Music | music_* | Battle music, menu music |
| Emotes | sfx_emote_* | Emote voice lines |
| Victory/Defeat | sfx_victory, sfx_defeat | Game end sounds |

## Resource Directory
```
resources/sfx/
├── spell sounds        # Per-spell effects
├── unit sounds         # Per-unit attack/death
├── ui sounds           # Interface feedback
├── music               # Background tracks
├── emotes              # Voice lines
└── misc                # Other effects
Total: 26,891 bytes
```

## Sound Priority System
Higher priority sounds can interrupt lower priority:
| Priority | Category | Behavior |
|----------|----------|----------|
| 0 | Default SFX | Standard playback |
| 1 | Click | UI feedback |
| 3 | Cancel | Interrupt clicks |
| 4 | Confirm | Important actions |
| 6 | Hide | Animation sounds |
| 7 | Error | Always plays |

## Related FX Sounds
FX definitions in fx.tsv reference sounds:
| FX | Related Sound |
|----|---------------|
| fx_f1_divinebond | sfx_spell_divinebond |
| fx_f2_phoenixfire | sfx_spell_phoenixfire |
| fx_f1_martyrdom | sfx_spell_martyrdom |
| fx_f2_innerfocus | sfx_spell_innerfocus |

## Statistics
- 26,891 bytes of SFX resources
- 7+ priority levels
- Faction-specific sound sets
- Unit-specific attack/death sounds
- Web Audio API for playback
- Volume controls per category (Master, Music, SFX, Voice)
