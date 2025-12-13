#ifndef INPUT_H
#define INPUT_H

#include "board.h"
#include "types.h"
#include "unit.h"

#include <raylib.h>

typedef struct {
    BoardPos hover_pos;
    bool hover_valid;
    Unit* selected_unit;
    bool move_tiles[BOARD_COLS][BOARD_ROWS];
} InputState;

void InitInputState(InputState* state);
void UpdateInputState(InputState* state, Board* board, Unit* units, int unit_count);
void CalculateMoveTiles(InputState* state, Unit* unit);
bool IsMoveValid(InputState* state, BoardPos pos);

#endif
