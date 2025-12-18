# Sylvanshine Tasks

## 🔴 BLOCKED

## 🟡 NEEDS INFO

**Unit Progression Tree Structure** → Need unit list iteration session with LLM to map FFT-style paths onto Duelyst assets

---

## RENDERING PIPELINE

### Shadow System
- [x] Static blob shadow (unit_shadow.png, 200/255 opacity, ground position)
- [ ] Shadow shader research (Duelyst FXShadowBlobSprite)
- [ ] Implement shadow shader (blur, perspective stretch)

### Grid Visuals
- [ ] Document Duelyst selection sprites (all variants in original repo)
- [ ] Tile sprites with spacing (Duelyst grid has gaps between tiles)
- [ ] Movement highlight sprites
- [ ] Attack highlight sprites
- [ ] Selection cursor sprites

### Backgrounds
- [ ] Per-level background system
- [ ] Level transition effect (arcade beat-em-up style screen transitions)
- [ ] Background asset organization

### Shader Catalogue
- [ ] Document all Duelyst shaders and their usage contexts
- [ ] Identify shaders useful for our implementation

---

## ANIMATION SYSTEM

### Core Animations
- [x] Attack timing
- [x] Death fadeout + hold frame
- [x] Spawn fade-in (opacity 0→1)
- [ ] Hit reaction positioning fix (currently offset)

### Advanced Animations
- [ ] Ranged projectile system
- [ ] Ranged attack animations (units with separate projectile sprites)
- [ ] Spawn FX animations (document when used vs walk-in)

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
- [ ] Attackable/attacking buildings (leverage Duelyst building assets)
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
- [ ] 4e/ICON internal typing (Brawler, Skirmisher, Artillery, Controller, Leader, Boss)
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
- [ ] Ground units cannot move through other units (verify Duelyst behavior)
- [ ] Flying units move through other units
- [ ] Sprite overlap visual handling

### Attack Types
- [ ] Melee attack flow
- [ ] Ranged attack flow (projectile travel)
- [ ] Attack range display

### Preview System
- [ ] Enemy move preview (optional, needs testing)
- [ ] Enemy targeting priority display (optional, needs testing)

---

## ASSET CATALOGUING

### Unit Research
- [ ] Chaff enemy identification (minor units suitable for fodder)
- [ ] Faction groupings (Songhai, Lyonar, etc.)
- [ ] Typal groupings (Arcanyst, Dervish, Mech, etc.)
- [ ] Unit-specific FX catalogue
- [ ] Unit-specific sounds catalogue
- [ ] Unit-grouped sprites (summons, related units)

### UI Trigger Catalogue
- [ ] Targeting cursors (3x3 selector, fence border, etc.)
- [ ] Battlecry-style ability UI patterns
- [ ] Per-unit special UI elements
- [ ] Generalized UI flows available for reuse

### Selection/Highlight Assets
- [ ] Document all selection sprite variants
- [ ] Document usage contexts in original Duelyst

### Color System Mapping
- [ ] Map Grey identity → Duelyst assets
- [ ] Map Orange identity → Duelyst assets
- [ ] Map Yellow identity → Duelyst assets
- [ ] Map Red identity → Duelyst assets
- [ ] Map Blue identity → Duelyst assets
- [ ] Map Green identity → Duelyst assets (unique units)

### Visual Progression Research
- [ ] Identify visual upgrade paths (least → most powerful)
- [ ] Assess visual relatedness requirements
- [ ] Document clear progression chains

---

## UNIT PROGRESSION

### Job System Skeleton
- [ ] Define tier structure (Tier 1 → Tier 2 → Tier 3?)
- [ ] Define branch points per color
- [ ] Map Duelyst units to job tree nodes
- [ ] XP/battle count for level ups

### MVP Progression
- [ ] Basic linear upgrades (Unit → Better Unit)
- [ ] Single class change demonstration

### Full Tree (Future)
- [ ] Multiple paths per base class
- [ ] Branching decision points
- [ ] Visual cohesion within paths

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
- [ ] Artifact/ability display

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
- [ ] Icon assets for abilities

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
- [x] Shadow system (unit_shadow.png, 200/255 opacity, ground position)

---

## OPEN DESIGN SPACE

- Charge scaling formula (flat bonus, multiplier, unlock tiers)
- Maneuvers vs mana spells mechanical distinction
- Individual Green unit designs
- Event node content/writing
- Boss encounter design
- Run length tuning
- Difficulty curve
