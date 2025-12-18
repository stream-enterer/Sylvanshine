# Sylvanshine Grid Analysis

## Context
Duelyst-inspired SDL3/C++20 tactics game. Reverse-engineering Duelyst rendering to match visual style.

**Repos:**
- `~/.local/git/Sylvanshine` (our project)
- `~/.local/git/duelyst` (reference)

## Workflow
1. I describe what needs investigation in Duelyst
2. You give me fish scripts using rg/fd to extract relevant code
3. I run scripts, paste output back
4. You analyze findings and produce concrete values/formulas
5. You generate code changes for Sylvanshine

**Script rules:** Use rg/fd one-liners, pipe to wl-copy, keep output <500 lines, combine into single runnable script.

## Completed
- TILE_SIZE: 95→48 (matches Duelyst 2.1:1 sprite:tile ratio)
- SHADOW_OFFSET: 19.5 (sprite feet align with tile center)
- Sprite positioning: `dst.y = screen_pos.y - (src.h - SHADOW_OFFSET) * scale`
- Z-order: sort by screen_pos.y ascending (painter's algorithm, no depth buffer)
- Shadow system: unit_shadow.png (96×48), 200/255 opacity, centered at ground pos

## Next Step: Phase 3 Polish - Shadow Shaders
Duelyst uses FXShadowBlobSprite with a custom shader for unit shadows.

**Investigate in Duelyst:**
1. What does the ShadowBlob shader do? (blur? gradient? noise?)
2. Shader uniforms/parameters
3. How it differs from a simple textured quad

**Output:** Updated code files ready to copy into Sylvanshine.
