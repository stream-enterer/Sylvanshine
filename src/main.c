#include "board.h"
#include "fx.h"
#include "input.h"
#include "types.h"
#include "unit.h"

#include <raylib.h>

#define MAX_FX_INSTANCES 16

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Duelyst Visual Prototype");
    SetTargetFPS(60);

    Board board;
    InitBoard(&board);

    Texture2D shadow = LoadTexture("data/units/unit_shadow.png");

    FXSystem fx_system;
    InitFXSystem(&fx_system);
    LoadRSXMappings(&fx_system, "data/fx/rsx_mapping.tsv");

    FXInstance fx_instances[MAX_FX_INSTANCES];
    int fx_count = 0;

    Unit units[MAX_UNITS];
    int unit_count = 0;

    LoadUnit(&units[unit_count], "f1_general");
    Vector2 spawn_pos = BoardToScreen(&board, 4, 2);
    SpawnUnit(&units[unit_count], &board, (BoardPos){4, 2});

    if (fx_count < MAX_FX_INSTANCES) {
        CreateSpawnFX(&fx_system, &fx_instances[fx_count], spawn_pos);
        fx_count++;
    }

    unit_count++;

    InputState input;
    InitInputState(&input);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateInputState(&input, &board, units, unit_count);

        for (int i = 0; i < unit_count; i++) {
            UpdateUnit(&units[i], &board, dt);
        }

        for (int i = 0; i < fx_count; i++) {
            UpdateFXInstance(&fx_instances[i], dt);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawBoard(&board);

        for (int col = 0; col < BOARD_COLS; col++) {
            for (int row = 0; row < BOARD_ROWS; row++) {
                if (input.move_tiles[col][row]) {
                    Color tile_color = {255, 255, 255, TILE_SELECT_OPACITY};
                    DrawTileHighlight(&board, col, row, tile_color);
                }
            }
        }

        if (input.hover_valid && input.selected_unit) {
            Color hover_color = {255, 255, 255, TILE_HOVER_OPACITY};
            if (IsMoveValid(&input, input.hover_pos)) {
                hover_color = (Color){100, 255, 100, TILE_HOVER_OPACITY};
            }
            DrawTileHighlight(&board, input.hover_pos.x, input.hover_pos.y, hover_color);
        }

        for (int i = 0; i < unit_count; i++) {
            DrawUnit(&units[i], shadow);
        }

        for (int i = 0; i < fx_count; i++) {
            DrawFXInstance(&fx_instances[i]);
        }

        DrawFPS(10, 10);

        EndDrawing();
    }

    for (int i = 0; i < fx_count; i++) {
        UnloadFXInstance(&fx_instances[i]);
    }
    for (int i = 0; i < unit_count; i++) {
        UnloadUnit(&units[i]);
    }
    UnloadTexture(shadow);
    UnloadBoard(&board);

    CloseWindow();
    return 0;
}
