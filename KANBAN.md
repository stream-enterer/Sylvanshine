# Sylvanshine Tasks (Annotated)

**Legend:**
- ⚡ = **Now trivial** — single script call answers this
- 🔓 = **Unblocked** — knowledge base provides missing info
- 📊 = **Bulk queryable** — can generate lists/reports programmatically
- *(unmarked = unchanged, still requires implementation work)*

---

## 🔴 BLOCKED

*(empty)*

---

## 🟡 NEEDS INFO

**Unit Progression Tree Structure** → Need unit list iteration session with LLM to map FFT-style paths onto Duelyst assets
> 📊 `instances/units.tsv` has all 664 units with faction, stats, visuals — can generate candidate lists for the LLM session automatically

---

## RENDERING PIPELINE

### Shadow System
- [ ] Penumbra blur shader (progressive blur from feet to head)
- [ ] Shadow distance fading (opacity/blur increases with distance from caster)
- [x] Dynamic shadow geometry (skew/stretch based on light position)
- [x] Light altitude calculation (sqrt(radius) * 6 * scale)
- [x] Per-map lighting presets (keyboard 0-9 to test)
- [x] Shadow intensity matched to Duelyst (0.75-1.1 per-map, not 0.15)
- [ ] Sync lighting presets with actual battle map backgrounds

### Backgrounds
- [ ] Per-level background system
- [ ] Level transition effect (arcade beat-em-up style screen transitions)
- [ ] Background asset organization

### Shader Catalogue
- [ ] Bloom activation — passes exist but not routed through render pipeline
- [ ] Document all Duelyst shaders and their usage contexts ⚡ `instances/shaders.tsv` (96 shaders already extracted)
- [ ] Identify shaders useful for our implementation 📊 can filter by type/usage

---

## ANIMATION SYSTEM

### Core Animations
- [x] Attack timing
- [x] Death fadeout + hold frame
- [x] Spawn fade-in (opacity 0→1)
- [ ] Hit reaction positioning fix (currently offset) 🔓 `get_unit_timing.py` gives exact attackDelay values

### Advanced Animations
- [ ] Ranged projectile system 🔓 `get_action_chain.py AttackAction` shows projectile flow
- [ ] Ranged attack animations (units with separate projectile sprites) 📊 can query units with ranged attacks
- [ ] Spawn FX animations (document when used vs walk-in) ⚡ `get_fx_composition.py Faction1.UnitSpawnFX`
- [ ] Attack path arc animation (ranged projectile trajectory)

### Timing
- [ ] FPS-independent timing verification (remove cap, disable vsync, confirm consistency)
- [ ] Animation speed multipliers (2x/3x player option)
- [ ] Speed multiplier without sound distortion (when sound implemented)
- [ ] Grid size change → timing verification

---

## LEVEL FLOW

### Player Entrance
- [ ] Walk from left offscreen
- [ ] Staggered entrance timing
- [ ] Per-row horizontal movement (start on destination row)
- [ ] End in leftmost column

### Enemy Spawns
- [ ] Walk-in from right (some levels)
- [ ] Pre-spawned on grid (some levels)
- [ ] Mixed spawn patterns per level

### Structures/Pickups
- [ ] Attackable/attacking buildings (leverage Duelyst building assets) 📊 can query building-type units
- [ ] Treasure chests → artifact drops
- [ ] Cages/spawners → unit recruitment
- [ ] Generic structure interaction system

### Level Progression
- [ ] Combat win condition: all enemies dead
- [ ] Node chooser after combat (Slay the Spire map style)
- [ ] Branching path selection
- [ ] ~12-15 levels per run (targeting 1hr sessions)
- [ ] 3 boss encounters per run
- [ ] Optional content nodes

### Event System
- [ ] Event node type (dialogue/choices)
- [ ] Automatic vs node-selected events (TBD)
- [ ] Class quirks → dialogue option unlocks

### Inter-Level Content
- [ ] Bonus content between levels (Golden Axe gnome style)

---

## UNIT IDENTITY

### Color System (from tactics-color-system.md)
- [ ] Grey: Tank (Armor + Sticky, charge when adjacent to enemy)
- [ ] Orange: Skirmisher (Diagonal + Ignore ZoC, charge when isolated)
- [ ] Yellow: Martial (Maneuvers, charge when adjacent to ally)
- [ ] Red: Support Caster (Mana, charge when adjacent to ally)
- [ ] Blue: Offense Caster (Mana, charge when 3+ from enemies)
- [ ] Green: Wildcard (unique per unit)

### Charge UI
- [ ] Condition dot (green=met, red=not met)
- [ ] Charge number display (hidden when zero)
- [ ] Unit opacity (full=can act, dimmed=turn over)
- [ ] Color pip on unit for quick identification

### Enemy Classification
- [ ] 4e/ICON internal typing (Brawler, Skirmisher, Artillery, Controller, Leader, Boss) 📊 `validate_card_balance.py` gives stat distributions to inform typing
- [ ] Minor visual telegraphing to player
- [ ] Balance framework based on typing

### Class Naming
- [ ] Short explanatory names (Brute, etc.)
- [ ] Name reflects color+role combination

---

## COMBAT MECHANICS

### Turn Structure
- [ ] WEGO-UGO system
- [ ] Player always goes first
- [ ] Click-ahead movement (queue moves, animations catch up)
- [ ] Legal move enforcement during click-ahead

### Undo System
- [ ] Per-move undo during player turn
- [ ] Undo window expires on turn end

### Movement Rules
- [ ] Pathfinding around units (units should route around other units, not through them)
- [ ] Ground units cannot move through other units (verify Duelyst behavior) ⚡ `get_modifier_chain.py ModifierFlying` shows movement rules
- [ ] Flying units move through other units ⚡ same query
- [ ] Sprite overlap visual handling

### Attack Types
- [ ] Melee attack flow 🔓 `get_action_chain.py AttackAction`
- [ ] Ranged attack flow (projectile travel) 🔓 action chain + FX composition
- [ ] Attack range display

### Preview System
- [ ] Enemy move preview (optional, needs testing)
- [ ] Enemy targeting priority display (optional, needs testing)

---

## ASSET CATALOGUING

### Unit Research
- [ ] Chaff enemy identification (minor units suitable for fodder) ⚡ `validate_card_balance.py --show-outliers`
- [ ] Faction groupings (Songhai, Lyonar, etc.) ⚡ `instances/factions.tsv` (7 factions, already done)
- [ ] Typal groupings (Arcanyst, Dervish, Mech, etc.) ⚡ `instances/races.tsv` (8 types, already done)
- [ ] Unit-specific FX catalogue 📊 `get_card_data.py` per unit, or bulk from TSV
- [ ] Unit-specific sounds catalogue 📊 same
- [ ] Unit-grouped sprites (summons, related units) 📊 queryable by relationship

### UI Trigger Catalogue
- [ ] Targeting cursors (3x3 selector, fence border, etc.) 🔓 `instances/fx.tsv` has these
- [ ] Battlecry-style ability UI patterns ⚡ `list_modifier_triggers.py ModifierOpeningGambit`
- [ ] Per-unit special UI elements 📊 queryable
- [ ] Generalized UI flows available for reuse 🔓 flows documented in `~/duelyst_analysis/flows/`

### Selection/Highlight Assets
- [ ] Document all selection sprite variants ⚡ `find_references.py SelectionSprite`
- [ ] Document usage contexts in original Duelyst 🔓 references show usage

### Color System Mapping
- [ ] Map Grey identity → Duelyst assets 📊 filter units by stats/abilities
- [ ] Map Orange identity → Duelyst assets 📊 same
- [ ] Map Yellow identity → Duelyst assets 📊 same
- [ ] Map Red identity → Duelyst assets 📊 same
- [ ] Map Blue identity → Duelyst assets 📊 same
- [ ] Map Green identity → Duelyst assets (unique units) 📊 same

### Visual Progression Research
- [ ] Identify visual upgrade paths (least → most powerful) 📊 664 units sortable by faction + stats
- [ ] Assess visual relatedness requirements 📊 same
- [ ] Document clear progression chains 📊 same

---

## UNIT PROGRESSION

### Job System Skeleton
- [ ] Define tier structure (Tier 1 → Tier 2 → Tier 3?)
- [ ] Define branch points per color
- [ ] Map Duelyst units to job tree nodes 📊 unit data ready for LLM iteration
- [ ] XP/battle count for level ups

### MVP Progression
- [ ] Basic linear upgrades (Unit → Better Unit)
- [ ] Single class change demonstration

### Full Tree (Future)
- [ ] Multiple paths per base class
- [ ] Branching decision points
- [ ] Visual cohesion within paths 📊 can query visual similarity
- [ ] Loreful weighted upgrades (e.g. Squire_t1 → {Knight_t2 = 0.7, Random_Grey_t2 = 0.2, Oathbreaker_t2  0.1})

---

## UI SYSTEMS

### CLI Options
- [ ] Scale flag (`-s 1/2/3/4` for 1x/2x/3x/4x)
- [ ] Grid and sprite scale linked

### HUD (DDSoM/DDToD Style)
- [ ] Top/bottom horizontal slices
- [ ] Unit cards (portrait, HP, name, XP, artifacts)
- [ ] Selected unit highlight

### Character Sheet
- [ ] Right-click unit card → detailed sheet
- [ ] OR group character sheet (item swaps)
- [ ] Artifact/ability display 🔓 artifact data in instances

### Controls
- [ ] Tab: cycle unit selection
- [ ] Number keys: ability/target selection
- [ ] Keyboard-only playable
- [ ] ESC: settings menu

### Settings Menu
- [ ] Animation speed (player units)
- [ ] Animation speed (enemy units)
- [ ] Resolution/scale options
- [ ] Vsync toggle
- [ ] FPS cap options

---

## META PROGRESSION

### Shops
- [ ] Shop node type
- [ ] Artifact purchasing
- [ ] Unit recruitment option

### Recruitment
- [ ] Event-based recruitment
- [ ] Shop-based recruitment
- [ ] Combat structure interaction (cage → freed ally)

### Artifacts
- [ ] Lootable from structures
- [ ] Purchasable from shops
- [ ] Equip system
- [ ] Icon assets for abilities 🔓 artifact assets catalogued

### Class Quirks
- [ ] Fixed quirks per class
- [ ] Dialogue option unlocks
- [ ] In-game mechanical effects

---

## ✅ DONE
- [x] Core movement + animations
- [x] Combat + FX + turns + AI
- [x] Attack damage timing
- [x] Grid perspective rendering (+16° rotation, direct primitives)
- [x] Analytical board centering
- [x] Inverse transform for mouse input
- [x] Scale architecture design
- [x] Board/sprite ratio (TILE_SIZE 95→48 for 2.1:1 ratio matching Duelyst)
- [x] Sprite foot positioning (SHADOW_OFFSET = 19.5)
- [x] Z-order sorting (screen Y, painter's algorithm)
- [x] Shadow system (blob shadow, perspective transform, shader pipeline)
- [x] Multi-pass rendering pipeline (FBO-based)
- [x] Bloom post-processing infrastructure (PassManager with highpass/blur/bloom passes)
- [x] Dynamic shadow geometry (skew/stretch from light position/altitude)
- [x] Per-map lighting preset system (10 presets from Duelyst battle maps)
- [x] Grid gap system (tile sprites with baked-in margins, disabled grid lines)
- [x] Duelyst color scheme (movement=#F0F0F0, attack=#FFD900 yellow)
- [x] Grid visuals: tile sprites, selection box, path arrows, glow tiles, hover states
- [x] Grid visuals: corner merging system with seam detection (`grid_renderer.cpp`)
- [x] Grid visuals: attack reticles, enemy indicators, enemy attack preview
- [x] Grid visuals: Z-order layer system (9 layers)

---

## OPEN DESIGN SPACE

*(unchanged — these are design decisions, not research)*

- Charge scaling formula (flat bonus, multiplier, unlock tiers)
- Maneuvers vs mana spells mechanical distinction
- Individual Green unit designs
- Event node content/writing
- Boss encounter design
- Run length tuning
- Difficulty curve

---

## ENEMY TURN VISUALIZATION (Sylvanshine Original)

> **Note:** This is a *new feature* not present in Duelyst. Duelyst's bot games have zero visual feedback during AI turns — actions just "happen" without preview. See `docs/enemy_turn_visualization_spec.md` for design.

### Core Concept
Show enemy "thinking" during enemy turn to help player understand AI decisions and add visual polish.

### Phase 1: Basic Turn Feedback
- [ ] "Enemy Turn" indicator (Duelyst has this: `notification_enemy_turn` sprite)
- [ ] Enemy unit highlight when AI is evaluating it
- [ ] Brief pause before each enemy action (configurable timing)

### Phase 2: Action Preview
- [ ] Show enemy unit's intended movement (red movement blob)
- [ ] Show enemy unit's attack target (red attack indicator)
- [ ] Movement path line to destination
- [ ] Attack path line to target

### Phase 3: Decision Visualization (Optional)
- [ ] Show all enemies being considered (dimmed highlight)
- [ ] Show selected enemy (bright highlight)
- [ ] Show evaluated targets (pulsing indicators)
- [ ] "Thinking" animation on active enemy

### Implementation Notes
- Timing: Insert artificial delays between AI evaluation → selection → move → attack
- Visualization: Reuse existing tile sprites with opponent colors (`#D22846`, `#82192D`)
- Toggle: Should be disable-able for speedrun/impatient players
- Animation speed multiplier should apply here too

### Reference Systems (Duelyst)
| System | In Duelyst | For Sylvanshine |
|--------|------------|-----------------|
| Ownership indicator (`tile_grid.png`) | Idle red tile under enemies | ✅ Done (`main.cpp:1205-1229`) |
| Enemy attack preview on hover | Red attack blob | ✅ Done (`main.cpp:1303-1314`) |
| Opponent network visualization | Selection box + path | **Adapt** for AI |
| Bot visual feedback | None | **New feature** |

---

## MISC / INVESTIGATE

- [ ] Card play tiles (`tile_card.png`, `tile_spawn.png`)

---

## Summary

| Symbol | Meaning | Count |
|--------|---------|-------|
| ⚡ | Now trivial (single query) | 11 |
| 🔓 | Unblocked (info available) | 12 |
| 📊 | Bulk queryable | 16 |
| | Unchanged | ~50 |

**~41 tasks** are now significantly easier or already answered by the knowledge base.
