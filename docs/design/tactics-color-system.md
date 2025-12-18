# Tactics Unit Color System

## Overview

A 6-color identity system for turn-based tactics units. Each color defines a hardcoded trait and a charge condition that creates distinct positioning incentives.

**Design Principle:** Visually see color → Know where to move the unit on turn one without checking abilities.

---

## Color Identities

| Color | Role | Hardcoded Trait | Charge Condition | Position Fantasy |
|-------|------|-----------------|------------------|------------------|
| Grey | Tank | Armor + Sticky (enemies pay +1 to leave adjacency) | Adjacent to enemy | Frontline anchor |
| Orange | Skirmisher | Diagonal movement + Ignore ZoC | Not adjacent to enemy AND not adjacent to ally | Flanker, hit-and-run |
| Yellow | Martial | Maneuvers (non-mana abilities) | Adjacent to ally | Battle line, formation |
| Red | Support Caster | Mana caster | Adjacent to ally | Behind frontline, with allies |
| Blue | Offense Caster | Mana caster | No enemies within 2 | Backline, safe distance |
| Green | Wildcard | Unique per unit | Unique per unit | Varies |

---

## Positioning Spectrum

```
        BLUE -------- RED -------- YELLOW -------- GREY
       (backline)   (support)    (battle line)    (front)
                                      |
                                   ORANGE
                                  (flanking/alone)
```

---

## Charge System

### Two Concepts

| Concept | What It Means |
|---------|---------------|
| **Condition** | Position check. Met = can use abilities. Not met = can't use abilities. |
| **Charges** | Banked resource. Scales ability effect when spent. |

### Charge Flow

- Charges are a **live board state**—continuously updated based on current positions
- At **end of unit's turn**: If you didn't use an ability AND condition is met → +1 charge banked
- To **spend charges**: Condition must be met at moment of use
- **No cap** on banked charges

### Turn Examples

| Turn | Start State | Action | End of Turn Result |
|------|-------------|--------|--------------------|
| 1 | 0 charges, condition met | Use ability (base power) | No gain (used ability) |
| 2 | 0 charges, condition met | Use ability (base power) | No gain (used ability) |
| 3 | 0 charges, condition met | Basic attack / do nothing | +1 charge banked |
| 4 | 1 charge, condition met | Use ability + spend charge (scaled) | No gain (used ability) |

### Spend-or-Save Tension

Every turn in position creates a decision: spend now for effect, or bank for bigger turn later.

---

## UI Display

### Elements

| Element | Shows |
|---------|-------|
| Unit opacity | Full = can act, Dimmed = turn over |
| Dot color | Green = condition met, Red = condition not met |
| Number | Banked charges (hidden if zero) |

### Display States

| Display | Meaning |
|---------|---------|
| `🟢` | Can act, base power, no banked charges |
| `3🟢` | Can act, 3 charges available to scale |
| `🔴` | Can't use abilities (out of position), no banked charges |
| `3🔴` | Can't use abilities, but 3 charges banked for later |

**Zero is hidden:** Dot alone means "can act at base power."

### After Turn Ends

- Unit dims
- Dot updates to current condition
- Number updates (incremented if banked)

---

## Color Details

### Grey (Tank)

**Trait:** Armor + Sticky
- Has Armor stat (damage reduction, value varies per unit)
- Enemies pay +1 movement to exit adjacency

**Charge Condition:** Adjacent to enemy

**Position:** Move next to enemy, hold ground

---

### Orange (Skirmisher)

**Trait:** Skirmisher
- Can move diagonally
- Ignores Zone of Control

**Charge Condition:** Not adjacent to enemy AND not adjacent to ally

**Position:** Flank, strike, escape to isolation

---

### Yellow (Martial)

**Trait:** Maneuvers
- Has special abilities that don't use mana

**Charge Condition:** Adjacent to ally

**Position:** Battle line, shoulder-to-shoulder with allies

---

### Red (Support Caster)

**Trait:** Mana caster
- Has mana pool, casts spells

**Charge Condition:** Adjacent to ally

**Position:** Stay near allies, behind frontline

**Differentiation from Yellow:** Red is a caster (ranged/magic), Yellow is melee martial. Same condition, different combat role.

---

### Blue (Offense Caster)

**Trait:** Mana caster
- Has mana pool, casts spells

**Charge Condition:** No enemies within 2 (3+ distance from all enemies)

**Position:** Backline, maintain safe distance

**Note:** Strict condition creates kiting gameplay. Escape tools live in individual unit kits, not base identity.

---

### Green (Wildcard)

**Trait:** Unique per unit

**Charge Condition:** Unique per unit

**Position:** Varies

**Design Space:** No constraints. Each Green unit is its own thing.

---

## Interaction Examples

### Red Supporting Orange

| Step | Board State | Red Charge | Orange Charge |
|------|-------------|------------|---------------|
| Turn starts | Red and Orange apart | ✗ | ✓ |
| Move Red adjacent to Orange | Red next to Orange | ✓ | ✗ |
| Red uses ability on Orange | Still adjacent | spent | ✗ |
| Orange moves to isolated position | Now apart | ✗ | ✓ |
| Orange uses ability | Still isolated | ✗ | spent |

Both got value. Sequencing mattered.

### Formation Play

| Unit | Position | Gets Charge? |
|------|----------|--------------|
| Grey | Adjacent to enemy | ✓ |
| Yellow | Adjacent to Grey | ✓ |
| Red | Adjacent to Yellow | ✓ |
| Blue | 3+ spaces from enemies | ✓ |
| Orange | Alone on flank | ✓ |

Proper formation = everyone earning.

---

## Open Design Space

- How charges scale abilities (flat bonus, multiplier, unlock tiers)
- What "Maneuvers" means mechanically vs mana spells
- Individual unit kits within each color
- Green unit designs
