# Script Coverage Analysis

## Current Scripts (20 total)

### Data Extraction Scripts
| Script | Purpose | Status |
|--------|---------|--------|
| `extract_agent_actions.js` | Extract AI agent action patterns | Complete |
| `extract_animations.js` | Extract animation frame data | Complete |
| `extract_challenge_scripts.js` | Extract challenge puzzle logic | Complete |
| `extract_localization.js` | Extract localization strings | Complete |
| `extract_particles.js` | Extract particle system configs | Complete |
| `extract_replay_format.js` | Extract replay data format | Complete |
| `extract_shaders.js` | Extract shader configurations | Complete |

### Query Scripts
| Script | Purpose | Status |
|--------|---------|--------|
| `calculate_move_duration.py` | Calculate unit movement timing | Complete |
| `find_references.py` | Find cross-file references | Complete |
| `get_action_chain.py` | Trace action execution chain | Complete |
| `get_ai_strategy.py` | Get AI strategy patterns | Complete |
| `get_animation_frames.py` | Get animation frame details | Complete |
| `get_card_data.py` | Look up card attributes | Complete |
| `get_fx_composition.py` | Get FX layer composition | Complete |
| `get_particle_config.py` | Get particle system config | Complete |
| `get_shader_info.py` | Get shader parameters | Complete |
| `get_unit_timing.py` | Get unit animation timing | Complete |
| `list_modifier_triggers.py` | List modifier event triggers | Complete |
| `lookup_localization.py` | Look up localized strings | Complete |
| `trace_imports.py` | Trace module dependencies | Complete |

## Common Query Patterns in Documentation

Based on analysis of flows and summaries:

| Pattern | Frequency | Current Coverage |
|---------|-----------|------------------|
| `getGameSession` | 36 uses | Covered by get_action_chain.py |
| `getCard` | 21 uses | Covered by get_card_data.py |
| `getTarget` | 13 uses | Partially covered |
| `getPosition` | 12 uses | Covered by calculate_move_duration.py |
| `getOwner` | 10 uses | Covered |
| `getModifiersContextObjects` | 6 uses | Covered by list_modifier_triggers.py |

## Suggested Additional Scripts

### 1. `get_modifier_chain.py` (High Priority)

**Purpose:** Trace modifier interactions and stacking for a given unit or card.

**Use Case:** When implementing new modifiers, understanding how existing modifiers stack and interact is critical.

```python
# Example usage
python get_modifier_chain.py --card "Cards.Faction4.GrandmasterVariax"
# Output: Shows all modifiers applied, their priorities, and interaction points
```

**Justification:**
- 717 modifiers with complex interactions
- Common debugging need for modifier conflicts
- Would help answer questions like "why does X override Y?"

### 2. `validate_card_balance.py` (Medium Priority)

**Purpose:** Check card stat distributions and flag potential balance outliers.

**Use Case:** Quick balance sanity check when adding new cards.

```python
# Example usage
python validate_card_balance.py --faction Faction2 --check-stats
# Output: Cards with unusual stat/cost ratios
```

**Justification:**
- 1,116 cards need consistent balancing
- Helps maintain faction identity
- Quick check during card development

### 3. `generate_flow_diagram.py` (Medium Priority)

**Purpose:** Generate Mermaid/ASCII diagrams from flow documentation.

**Use Case:** Visual documentation for complex flows.

```python
# Example usage
python generate_flow_diagram.py --flow unit_attack_flow.md --format mermaid
# Output: Mermaid diagram source
```

**Justification:**
- Flows are complex with many system interactions
- Visual representation aids understanding
- Would enhance documentation quality

## Script Quality Assessment

| Category | Count | Coverage |
|----------|-------|----------|
| Extraction scripts | 7 | All major data types covered |
| Query scripts | 13 | Core gameplay queries covered |
| Analysis scripts | 0 | **Gap: No analysis/validation scripts** |
| Visualization scripts | 0 | **Gap: No diagram generation** |

## Recommendations

1. **High Priority:** Add `get_modifier_chain.py` - Critical for implementing new cards
2. **Medium Priority:** Add `validate_card_balance.py` - Useful for balance work
3. **Nice to Have:** Add `generate_flow_diagram.py` - Improves documentation

## Scripts Not Needed

The following were considered but deemed unnecessary:

- FX debugging script - Covered by `get_fx_composition.py`
- Sound lookup script - Simple RSX key lookup
- Challenge parser - Covered by `extract_challenge_scripts.js`
