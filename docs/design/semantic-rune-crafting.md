# Semantic Rune Crafting System

## Golden Rules

**1. The player must be visibly rewarded for making a mental connection between a rune and a cube's description.**

If a player reads "burn scars" and thinks "fire would work here" — that intuition must pay off. The composition theater should validate their reasoning with resonance animations and stronger results. The system exists to reward semantic thinking, not to trick or randomize it away.

**2. It's more fun to be punished in an entertaining way than to be bored by a mediocre or generic result.**

We want the player to fail spectacularly rather than always have superpowered results or mediocre results. A combination that backfires hilariously, summons something absurd, or explodes in their face is better than a safe, forgettable outcome. The floor isn't "average" — it's "interesting."

---

## Core Concept

Player combines a **cube** (semantic description) with a **rune** (symbolic meaning). An LLM evaluates the semantic relationship and produces a result. The evaluation process is visualized as an opaque but entertaining "composition theater" — like Peggle, you choose your input then watch the cascade.

**Key feeling:** You're not optimizing a formula. You're reading flavor text, making an intuitive connection, and watching something interesting happen.

---

## The Loop

```
INPUT
  Cube: Short semantic description ("Ancient cube with burn scars")
  Rune: Symbolic item (Fire, Ice, Horse, Bell, etc.)
           ↓
COMPOSITION THEATER (visual, opaque)
  Player sees: Particles representing semantic elements
               colliding, merging, orbiting, transforming
  Player doesn't see: LLM reasoning
           ↓
OUTPUT
  Always: A viable result (unit summoned, item created, etc.)
  Sometimes: Bonus traits, rare outcomes, special effects
```

---

## Design Pillars

| Principle | Implementation |
|-----------|----------------|
| No useless combinations | LLM always finds *some* connection, even if weak |
| No grinding | One rune + one cube = one result, no repetition loops |
| Peggle feel | Choose input, watch cascade, enjoy the process |
| Floor not ceiling | Results are always at least average; bonuses are upside |
| Opaque but legible | Player sees *something* happening without reverse-engineering it |

---

## Cube Descriptions

Short, evocative, semantically rich.

```
"Ancient cube with burn scars"
"Humming box covered in frost"
"Cracked vessel leaking light"
"Silent cube wrapped in chains"
"Warm stone that smells of horses"
"Mirror-surfaced cube, cold to touch"
"Bone cube with carved eyes"
```

Each description implies semantic hooks the LLM can latch onto.

---

## Rune Types

### Tier 1: Elemental (obvious)
Fire, Ice, Lightning, Earth, Water, Void

### Tier 2: Functional (clear purpose)
Damage, Heal, Shield, Power, Speed

### Tier 3: Abstract (interpretable)
Life, Death, Time, Balance, Chaos

### Tier 4: Weird (semantically unstable)
Horse, Bell, Eye, Spiral, Crown, Mask, Bone, Door, Key, Feather

Weird runes have no "correct" interpretation — the LLM improvises based on context.

---

## LLM Output Schema

```python
@dataclass
class CompositionEvent:
    element_a: str        # semantic element from cube ("burn scars")
    element_b: str        # the rune ("fire")
    relationship: str     # "resonance" | "tension" | "neutral" | "transformation"
    intensity: float      # 0.0 - 1.0
    visual_hint: str      # "merge" | "repel" | "orbit" | "consume" | "split" | "glow"

@dataclass
class CompositionResult:
    events: list[CompositionEvent]
    final_strength: float           # 0.3 - 1.0 (floor guarantees average result)
    output_tags: list[str]          # semantic tags for result selection
    bonus_effects: list[str]        # additional traits/effects
    flavor_text: str                # short description of what happened
```

---

## Visual Composition Theater

The `events` list drives a particle animation system:

| Relationship | Visual Behavior |
|--------------|-----------------|
| resonance | Particles attract and merge, bright flash |
| tension | Particles orbit each other, sparking |
| neutral | Particles drift past, minimal interaction |
| transformation | Particles collide and change shape/color |

| Visual Hint | Animation |
|-------------|-----------|
| merge | Elements combine into one |
| repel | Elements push apart |
| orbit | Elements circle each other |
| consume | One element absorbs the other |
| split | Elements break into new pieces |
| glow | Element brightens and shifts |

Intensity (0-1) controls:
- Particle speed
- Effect brightness
- Sound intensity
- Screen shake (at high values)

---

## Example Compositions

### Fire Rune + "Ancient cube with burn scars"

**LLM reasoning (hidden):**
Strong semantic match. "burn" and "fire" resonate. "ancient" adds depth — this is reuniting, not just matching.

**Output:**
```
events: [
  {a: "burn scars", b: "fire", relationship: "resonance", intensity: 0.9, visual: "merge"},
  {a: "ancient", b: "fire", relationship: "transformation", intensity: 0.5, visual: "glow"}
]
final_strength: 0.85
output_tags: ["fire", "survivor", "ancient"]
bonus_effects: ["ember_touched"]
flavor_text: "Old wounds remember their origin."
```

**Player sees:**
1. "burn scars" and "fire" particles rush together
2. Bright merge flash
3. "ancient" particle glows, shifts to warm color
4. Strong crystal forms

---

### Ice Rune + "Ancient cube with burn scars"

**LLM reasoning (hidden):**
Semantic tension. Ice vs burn = opposition. But also: healing burns with cold? Interesting conflict.

**Output:**
```
events: [
  {a: "burn scars", b: "ice", relationship: "tension", intensity: 0.7, visual: "orbit"},
  {a: "ancient", b: "ice", relationship: "neutral", intensity: 0.3, visual: "drift"}
]
final_strength: 0.5
output_tags: ["ice", "conflicted", "healing"]
bonus_effects: ["soothed_wounds"]
flavor_text: "Cold meets old heat. Something settles."
```

**Player sees:**
1. "burn scars" and "ice" circle each other, sparking
2. They stabilize into orbit, don't merge
3. "ancient" drifts by, uninvolved
4. Unusual crystal forms

---

### Horse Rune + "Ancient cube with burn scars"

**LLM reasoning (hidden):**
No obvious connection. Horse = journey, freedom, cavalry. Burn scars = trauma, fire history. Maybe: escaping fire? Branded horse? War mount from long ago?

**Output:**
```
events: [
  {a: "burn scars", b: "horse", relationship: "transformation", intensity: 0.6, visual: "split"},
  {a: "ancient", b: "horse", relationship: "resonance", intensity: 0.4, visual: "merge"}
]
final_strength: 0.55
output_tags: ["mounted", "survivor", "war", "old"]
bonus_effects: ["branded"]
flavor_text: "Old cavalry. The brand remains."
```

**Player sees:**
1. "burn scars" and "horse" collide, split into new shapes
2. "ancient" and "horse" quietly merge
3. Transformed crystal forms
4. Result has unexpected "branded" trait

---

## Result Selection

The `output_tags` bias selection from a database of possible results (units, items, effects).

```python
def select_result(tags: list[str], strength: float, database: list[Result]) -> Result:
    scored = []
    for entry in database:
        score = tag_overlap(tags, entry.tags)
        scored.append((entry, score))
    
    scored.sort(key=lambda x: -x[1])
    
    if strength > 0.8:
        return scored[0][0]  # top match
    elif strength > 0.5:
        return weighted_random(scored[:5])
    else:
        return weighted_random(scored[:10])
```

Lower strength = more variance in selection = more surprises.

---

## Prompt Template

```
You are interpreting a symbolic crafting combination.

CUBE: "{cube_description}"
RUNE: {rune_name} (associations: {rune_associations})

Analyze the semantic relationship between the cube's description and the rune.
Break down the cube description into semantic elements.
For each element, determine its relationship to the rune.

Output a CompositionResult with:
- events: list of semantic interactions
- final_strength: 0.3-1.0 (how strong/coherent the combination is)
- output_tags: semantic tags for result selection
- bonus_effects: any special traits that emerge
- flavor_text: one sentence describing what happened

Find interesting connections. There are no "wrong" combinations — 
even weak connections produce something. Weird runes should produce 
unexpected but narratively coherent interpretations.
```

---

## Open Questions

- Multiple runes at once? (increases event complexity)
- Cube "memory" of past crafts? (adds state, enables evolution)
- Rare "critical" compositions? (jackpot moments)
- Player can see event list after composition? (post-hoc legibility)
- Sound design for different relationships?

---

## Reference: Peggle Parallel

| Peggle | This System |
|--------|-------------|
| Aim ball | Choose rune |
| Ball physics | LLM interpretation |
| Pegs lighting up | Composition events animating |
| Score multipliers | Intensity values |
| Bucket bonus | High final_strength bonus |
| Watch and enjoy | Watch and enjoy |

The skill is in the setup. The joy is in the cascade.
