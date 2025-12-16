# Sound System Implementation Guide

## Quick Start

Load sounds for a unit:
```cpp
// In entity loading
std::string sound_path = "data/timing/sound_events.tsv";
// Lookup: unit_folder + event_type → sound_file
// File location: data/sfx/{sound_file}.wav
```

Play sounds at these moments:
| Event | When to Trigger |
|-------|-----------------|
| `apply` | Unit spawns on board |
| `attack` | Attack animation starts |
| `attackDamage` | Damage is dealt (at `attackReleaseDelay` point) |
| `death` | Unit dies, before death animation |
| `receiveDamage` | Unit takes damage (play hit animation) |
| `walk` | Each movement step or movement start |

---

## File Structure

```
data/
├── sfx/                              # 522 WAV files (16-bit PCM)
│   ├── sfx_f1_general_attack_swing.wav
│   ├── sfx_neutral_maw_death.wav
│   └── ...
│
└── timing/
    ├── sound_events.tsv              # PRIMARY: unit → event → sound
    ├── sound_inheritance.tsv         # Which units share sounds
    ├── sound_fixes.tsv               # Audit log (not needed at runtime)
    ├── sound_unfixable.tsv           # Empty if all resolved
    └── SOUND_VERIFICATION.md         # Human-readable report
```

---

## Data Format

### sound_events.tsv
```
unit_folder     event_type      sound_file
f1_general      apply           sfx_unit_deploy
f1_general      attack          sfx_f1_general_attack_swing
f1_general      attackDamage    sfx_f6_draugarlord_attack_impact
f1_general      death           sfx_f1general_death
f1_general      receiveDamage   sfx_f1_general_hit
f1_general      walk            sfx_neutral_ladylocke_attack_impact
```

- Tab-separated, header row
- `sound_file` is the filename stem (no path, no `.wav`)
- Full path: `data/sfx/{sound_file}.wav`

### sound_inheritance.tsv
```
unit_folder                     inherits_from
f1_general_skinroguelegacy      f1_general
f1_elyxstormblademk2            f1_elyxstormblade
neutral_healingmystictwitch     neutral_healingmystic
```

Units listed here have no `sounds.txt` of their own — their sounds come from another unit. The main `sound_events.tsv` already includes these inherited mappings, so **you don't need to read this file at runtime**. It exists for debugging.

---

## Loading Strategy

### Option A: Single HashMap (Recommended)
```cpp
// Key: "unit_folder:event_type"
// Value: sound file path
std::unordered_map<std::string, std::string> sound_map;

void load_sound_events(const char* tsv_path) {
    // Skip header line
    // For each line: unit, event, sound_file
    std::string key = unit + ":" + event;
    sound_map[key] = "data/sfx/" + sound_file + ".wav";
}

std::optional<std::string> get_sound(const std::string& unit, const std::string& event) {
    auto it = sound_map.find(unit + ":" + event);
    if (it != sound_map.end()) return it->second;
    return std::nullopt;
}
```

### Option B: Per-Unit Sound Struct
```cpp
struct UnitSounds {
    std::string apply;
    std::string attack;
    std::string attack_damage;
    std::string death;
    std::string receive_damage;
    std::string walk;
};

std::unordered_map<std::string, UnitSounds> unit_sounds;
```

---

## Integration with Attack Timeline

From `unit_timing.tsv`, each unit has:
- `attack_delay`: Pause BEFORE attack animation starts
- `attack_release_delay`: Point WITHIN animation where damage triggers

```
Timeline:
    ├─ User clicks attack
    ├─ Wait attack_delay (0.0-1.7s)
    ├─ Start attack animation
    │   └─ Play "attack" sound HERE
    ├─ At attack_release_delay (usually 0.0):
    │   ├─ Deal damage to target
    │   ├─ Play "attackDamage" sound on attacker
    │   └─ Play "receiveDamage" sound on target
    └─ Animation completes → return to idle
```

Most units have `attack_release_delay = 0.0`, meaning damage happens at animation start, not middle.

---

## SDL3 Audio Implementation

```cpp
#include <SDL3/SDL_audio.h>

struct SoundCache {
    std::unordered_map<std::string, Mix_Chunk*> chunks;
    
    Mix_Chunk* get(const std::string& path) {
        auto it = chunks.find(path);
        if (it != chunks.end()) return it->second;
        
        Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
        if (chunk) chunks[path] = chunk;
        return chunk;
    }
    
    void play(const std::string& path) {
        Mix_Chunk* chunk = get(path);
        if (chunk) Mix_PlayChannel(-1, chunk, 0);
    }
};
```

Or with SDL3 directly (no SDL_mixer):
```cpp
SDL_AudioSpec spec;
Uint8* audio_buf;
Uint32 audio_len;

SDL_LoadWAV(path, &spec, &audio_buf, &audio_len);
SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &spec);
SDL_PutAudioStreamData(stream, audio_buf, audio_len);
```

---

## Edge Cases

### Missing Sounds
Only `prop_cranewisp` has no sounds (it's a decorative prop). All 692 other units have valid mappings.

If a lookup fails, skip silently:
```cpp
void play_unit_sound(const Entity& entity, const std::string& event) {
    auto sound = get_sound(entity.unit_folder, event);
    if (sound) {
        sound_cache.play(*sound);
    }
    // No sound = no-op, not an error
}
```

### Shared Sounds
Many units share the same sound files. For example, `sfx_singe2` is used by 100+ units for walk sounds. The sound cache handles deduplication automatically.

### Sound Naming Patterns
```
sfx_{faction}_{unit}_{event}.wav    # Unit-specific
sfx_spell_{name}.wav                 # Spell effects
sfx_unit_{type}.wav                  # Generic fallbacks
sfx_neutral_{unit}_{event}.wav       # Neutral faction
```

---

## File Statistics

| Metric | Value |
|--------|-------|
| Total WAV files | 522 |
| Units with sounds | 692 |
| Units without sounds | 1 (prop_cranewisp) |
| Total sound mappings | ~4150 |
| Sound events per unit | 6 (apply, attack, attackDamage, death, receiveDamage, walk) |

---

## Regenerating Sound Data

If you modify `sounds.txt` files or add units:

```bash
python scripts/validate_sounds.py ~/.local/git/Sylvanshine
```

This reads all per-unit `sounds.txt` files, validates against actual WAV files, resolves inheritance, and regenerates the TSV files.
