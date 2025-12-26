# Menu Code Guide (LLM Quick Reference)

This document explains Sylvanshine's menu rendering architecture to prevent common misunderstandings.

## Core File

`src/main.cpp` → `render_settings_menu(const RenderConfig& config)`

---

## Critical Concept: Decoupled Components

The menu has **three independent components** that are deliberately NOT derived from each other:

```
┌─────────────────────────────────────┐  ← Title bar (own width, own position)
├─────────────────────────────────────┤  ← 1px gap
│                                     │
│         Dialog body                 │  ← Dialog (own width, own height)
│                                     │
│                                     │
└─────────────────────────────────────┘

Text positions are ALSO decoupled from their containers.
```

**Why this matters:** If asked to "widen the dialog by 10px", do NOT assume the title bar should also widen. Each component has independent dimensions.

### Decoupled Variables

| Component | Width Variable | NOT derived from |
|-----------|---------------|------------------|
| Dialog | `menu_width` | anything |
| Title bar | `title_w` | menu_width |
| Title overhang left | `title_overhang_left` | menu_width |
| Title overhang right | `title_overhang_right` | menu_width |
| Title text X | hardcoded offset from `title_x` | title_w |

---

## Footgun #1: Magic Numbers Are Screen Percentages

Every "magic number" is a **proportion of screen dimensions**, not pixels.

```cpp
// WRONG interpretation: "menu is 603.4 pixels wide"
// RIGHT interpretation: "menu is 31.4% of screen width"
float menu_width = config.window_w * 0.314271f;  // 603.4px at 1080p
```

The comment `// 603.4px at 1080p` is documentation only. The actual value scales with resolution.

### Converting Pixels to Percentages

When user says "move X pixels at 1080p":
- Horizontal: `pixels / 1920.0f` → multiply by `config.window_w`
- Vertical: `pixels / 1080.0f` → multiply by `config.window_h`

```cpp
// "shift right 37 pixels at 1080p"
float shift = config.window_w * 0.019271f;  // 37/1920
```

---

## Footgun #2: "Extend" vs "Crop" Semantics

- **Extend right** = increase width (add to `.w`)
- **Extend left** = decrease X position AND increase width
- **Extend down** = increase height (add to `.h`)
- **Crop right** = decrease width
- **Crop left** = increase X position AND decrease width

When the user says "extend left by 10px", you need TWO changes:
```cpp
menu_x -= config.window_w * (10.0f/1920.0f);  // move left
menu_width += config.window_w * (10.0f/1920.0f);  // grow width
```

---

## Footgun #3: Y-Axis Direction

Screen Y increases downward. Named variables hide this:

```cpp
float offset_up = config.window_h * 0.042593f;  // positive value
menu_y = ... - offset_up;  // subtraction moves UP
```

- "Move up" → subtract from Y
- "Move down" → add to Y
- "Extend down" → add to height (no Y change needed)
- "Extend up" → subtract from Y AND add to height

---

## Footgun #4: Title Bar Position

The title bar sits **ABOVE** the dialog body with a gap. Its Y is calculated by going negative from `menu_y`:

```cpp
float title_bar_y = menu_y - title_height - gap;
//                  ↑ dialog top
//                         ↑ go up by title height
//                                      ↑ go up by gap
```

If you modify `menu_y`, the title bar moves with it (good). If you modify `title_height`, the title bar's top edge moves but `menu_y` stays fixed (title grows upward).

---

## Footgun #5: Bottom Extension Pattern

The dialog's visual height differs from `menu_height` used in calculations:

```cpp
float menu_height = config.window_h * 0.75f;  // used for positioning calculations
float bottom_ext = config.window_h * 0.085185f;  // visual-only extension

// menu_height used for item positioning
float item_y = menu_y + menu_height * 0.06f;

// But drawn taller
SDL_FRect dialog_body = {menu_x, menu_y, menu_width, menu_height + bottom_ext};
```

**Do NOT** add `bottom_ext` to `menu_height` itself—it would break item positioning.

---

## Footgun #6: Three-Color Gradient

A 3-color vertical gradient requires TWO quad draws:

```cpp
// Top→Mid gradient (upper half)
g_gpu.draw_quad_gradient(title_upper, top_color, top_color, mid_color, mid_color);
//                                    ↑TL       ↑TR       ↑BR        ↑BL

// Mid→Bot gradient (lower half)
g_gpu.draw_quad_gradient(title_lower, mid_color, mid_color, bot_color, bot_color);
```

The vertex order for `draw_quad_gradient` is: **TL, TR, BR, BL** (clockwise from top-left).

---

## Footgun #7: Text Positioning Independence

Text positions use **absolute offsets from container origin**, not centering or relative positioning:

```cpp
// Title text: offset from title_x and title_bar_y
float title_text_x = title_x + config.window_w * 0.014167f;  // fixed offset
float title_text_y = title_bar_y + config.window_h * 0.002037f;
```

If the container moves, text moves with it. If the container resizes, text does NOT reposition (it's not centered).

---

## Common Request Translations

| User says | Actually means |
|-----------|---------------|
| "Widen by 10px" | Increase width percentage by 10/1920 |
| "Extend left" | Decrease X AND increase width |
| "Crop right" | Decrease width only |
| "Move up" | Subtract from Y |
| "Make taller" | Increase height (grows downward by default) |
| "Extend bottom" | Increase height OR add to `bottom_ext` |
| "Title bar too short" | Increase `title_height` (grows upward) |

---

## Quick Reference: Key Variables

```cpp
// Dialog body
menu_width      // dialog width (screen %)
menu_height     // dialog height for calculations (screen %)
menu_x, menu_y  // dialog top-left position
bottom_ext      // visual-only height extension

// Title bar (DECOUPLED from dialog)
title_w                 // title bar width (independent)
title_height            // title bar height
title_overhang_left     // how far left of dialog (independent)
title_overhang_right    // how far right of dialog (independent)
title_x                 // = menu_x - title_overhang_left
title_bar_y             // = menu_y - title_height - gap

// Composition offsets
offset_up       // shift entire menu up from center
offset_right    // shift entire menu right from center
left_extend     // additional left shift for dialog

// Text (DECOUPLED from containers)
title_text_x, title_text_y  // absolute offsets from title bar origin
item_x, item_y              // menu item positions
```

---

## Testing Changes

After any layout change, verify at multiple resolutions. The percentage-based system should maintain proportions, but edge cases can break:

1. Check 1920x1080 (baseline)
2. Check 2560x1440 (common high-res)
3. Check 1280x720 (smaller screens)
