#include "input.h"

#include <stdlib.h>
#include <string.h>

void InitInputState(InputState* state) {
    state->hover_pos = (BoardPos){-1, -1};
    state->hover_valid = false;
    state->selected_unit = NULL;
    memset(state->move_tiles, 0, sizeof(state->move_tiles));
}

void UpdateInputState(InputState* state, RenderState* render, Unit* units, int unit_count) {
    Vector2 mouse = GetMousePosition();
    Vector3 world_pos = ScreenPosToWorld(render, mouse.x, mouse.y);
    BoardPos board_pos = WorldToBoard(render, world_pos);

    state->hover_pos = board_pos;
    state->hover_valid = IsValidBoardPos(board_pos.x, board_pos.y);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state->hover_valid) {
        Unit* clicked_unit = NULL;
        for (int i = 0; i < unit_count; i++) {
            if (units[i].active &&
                units[i].board_pos.x == board_pos.x &&
                units[i].board_pos.y == board_pos.y) {
                clicked_unit = &units[i];
                break;
            }
        }

        if (clicked_unit && clicked_unit->state == UNIT_STATE_IDLE) {
            if (state->selected_unit) {
                state->selected_unit->selected = false;
            }
            state->selected_unit = clicked_unit;
            clicked_unit->selected = true;
            CalculateMoveTiles(state, clicked_unit);
        } else if (state->selected_unit && IsMoveValid(state, board_pos)) {
            StartUnitMove(state->selected_unit, render, board_pos);
            state->selected_unit->selected = false;
            state->selected_unit = NULL;
            memset(state->move_tiles, 0, sizeof(state->move_tiles));
        } else if (!clicked_unit) {
            if (state->selected_unit) {
                state->selected_unit->selected = false;
                state->selected_unit = NULL;
                memset(state->move_tiles, 0, sizeof(state->move_tiles));
            }
        }
    }
}

void CalculateMoveTiles(InputState* state, Unit* unit) {
    memset(state->move_tiles, 0, sizeof(state->move_tiles));

    int ux = unit->board_pos.x;
    int uy = unit->board_pos.y;

    for (int dx = -MOVEMENT_RANGE; dx <= MOVEMENT_RANGE; dx++) {
        for (int dy = -MOVEMENT_RANGE; dy <= MOVEMENT_RANGE; dy++) {
            int dist = abs(dx) + abs(dy);
            if (dist > 0 && dist <= MOVEMENT_RANGE) {
                int nx = ux + dx;
                int ny = uy + dy;
                if (IsValidBoardPos(nx, ny)) {
                    state->move_tiles[nx][ny] = true;
                }
            }
        }
    }
}

bool IsMoveValid(InputState* state, BoardPos pos) {
    if (!IsValidBoardPos(pos.x, pos.y)) return false;
    return state->move_tiles[pos.x][pos.y];
}
