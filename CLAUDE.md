# Coding Standards

**Flat structure is mandatory:**
- ALL headers go in `include/` — no subdirectories
- ALL source files go in `src/` — no subdirectories

## Naming Conventions

### Code Elements
- **Types (structs, enums, typedefs)**: PascalCase (`GameState`, `Unit`, `BoardPos`)
- **Functions**: PascalCase (`CalculateDamage`, `GetUnitAt`, `IsValidMove`)
- **Struct fields**: snake_case (`unit_count`, `max_health`, `board_x`)
- **Parameters**: snake_case (`unit_id`, `target_pos`, `damage_amount`)
- **Local variables**: snake_case (`result`, `hex_distance`, `is_valid`)
- **Constants (#define)**: SCREAMING_SNAKE_CASE (`MAX_UNITS`, `TILE_SIZE`)
- **Enum values**: SCREAMING_SNAKE_CASE (`TERRAIN_GRASS`, `UNIT_IDLE`)
- **Global variables**: g_PascalCase (`g_GameState`, `g_Assets`)

### Files
- **Headers**: snake_case.h (`game_state.h`, `unit.h`, `board.h`)
- **Source**: snake_case.c (`game_state.c`, `unit.c`, `board.c`)
- **Header guards**: SCREAMING_SNAKE_CASE (`GAME_STATE_H`, `UNIT_H`)

### Data Files
- **TSV/JSON keys**: snake_case (`unit_type`, `max_health`)
- **Asset folders**: snake_case (`fx_compositions`, `sfx_ui`)

## Include Organization

1. Corresponding header first (for .c files)
2. Project headers (double quotes), alphabetically sorted
3. Blank line
4. Library headers (angle brackets)
5. Standard library headers (angle brackets), alphabetically sorted

```c
#include "game_state.h"

#include "board.h"
#include "config.h"
#include "unit.h"

#include <raylib.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
```

## Header File Template

```c
#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <stdbool.h>

typedef struct {
    int x;
    int y;
} Example;

Example CreateExample(int x, int y);
void UpdateExample(Example* e);

#endif
```

## Compiler Warnings Policy

**Zero tolerance for warnings in project code.**

Build with: `-Wall -Wextra -Wpedantic -Werror`

### Common Fixes

#### Unused Parameters
```c
void MyFunction(int unused_param) {
    (void)unused_param;
}
```

#### Unused Variables
```c
int result = Calculate();
(void)result;
```

## Code Structure

**Maximum nesting depth: 3 levels.**

If code exceeds 3 levels of nesting, extract helper functions.

```c
// BAD: 4 levels deep
void Bad(void) {
    for (...) {
        if (...) {
            for (...) {
                if (...) {
                }
            }
        }
    }
}

// GOOD: extracted
static void ProcessItem(Item* item) {
    if (!item->is_valid) return;
    // handle item
}

void Good(void) {
    for (...) {
        if (...) {
            ProcessItem(&item);
        }
    }
}
```

## Memory Management

- Prefer stack allocation for small, fixed-size data
- Use arena/pool allocators for game objects where possible
- Document ownership in comments when heap allocating
- Free in reverse order of allocation

## No Comments

Do not write code comments. Names should be self-documenting.
