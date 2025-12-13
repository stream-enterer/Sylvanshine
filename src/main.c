#include "board.h"
#include "fx.h"
#include "input.h"
#include "render.h"
#include "types.h"
#include "unit.h"

#include <raylib.h>

#include <stdio.h>

#define MAX_FX_INSTANCES 16

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Duelyst Visual Prototype");
    SetTargetFPS(60);

    RenderState render;
    InitRenderState(&render);

    Board board;
    InitBoard(&board);

    printf("\n=== DEBUG INFO ===\n");
    printf("1. Background texture: %d x %d pixels\n", board.background.width, board.background.height);
    printf("2. Camera settings:\n");
    printf("   Position: (%.2f, %.2f, %.2f)\n", render.camera.position.x, render.camera.position.y, render.camera.position.z);
    printf("   Target:   (%.2f, %.2f, %.2f)\n", render.camera.target.x, render.camera.target.y, render.camera.target.z);
    printf("   FOV Y:    %.2f world units visible vertically\n", render.camera.fovy);
    printf("   Projection: %s\n", render.camera.projection == CAMERA_ORTHOGRAPHIC ? "ORTHOGRAPHIC" : "PERSPECTIVE");
    printf("3. Board world-space size:\n");
    printf("   1 tile = 1 world unit\n");
    printf("   Board width:  %.1f world units (%d cols)\n", render.board_width, BOARD_COLS);
    printf("   Board height: %.1f world units (%d rows)\n", render.board_height, BOARD_ROWS);
    printf("4. Window size: %d x %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("==================\n\n");

    Texture2D shadow = LoadTexture("data/units/unit_shadow.png");

    FXSystem fx_system;
    InitFXSystem(&fx_system);
    LoadRSXMappings(&fx_system, "data/fx/rsx_mapping.tsv");

    FXInstance fx_instances[MAX_FX_INSTANCES];
    int fx_count = 0;

    Unit units[MAX_UNITS];
    int unit_count = 0;

    LoadUnit(&units[unit_count], "f1_general");
    SpawnUnit(&units[unit_count], &render, (BoardPos){4, 2});

    if (fx_count < MAX_FX_INSTANCES) {
        Vector3 spawn_pos = BoardToWorld(&render, 4, 2);
        CreateSpawnFX(&fx_system, &fx_instances[fx_count], spawn_pos);
        fx_count++;
    }

    unit_count++;

    InputState input;
    InitInputState(&input);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateInputState(&input, &render, units, unit_count);

        for (int i = 0; i < unit_count; i++) {
            UpdateUnit(&units[i], &render, dt);
        }

        for (int i = 0; i < fx_count; i++) {
            UpdateFXInstance(&fx_instances[i], dt);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawBackground(&board);

        BeginMode3D(render.camera);

        for (int col = 0; col < BOARD_COLS; col++) {
            for (int row = 0; row < BOARD_ROWS; row++) {
                if (input.move_tiles[col][row]) {
                    Color tile_color = {MOVE_COLOR_R, MOVE_COLOR_G, MOVE_COLOR_B, TILE_SELECT_OPACITY};
                    DrawTileHighlight(&board, &render, col, row, tile_color);
                }
            }
        }

        if (input.hover_valid && input.selected_unit) {
            Color hover_color;
            if (IsMoveValid(&input, input.hover_pos)) {
                hover_color = (Color){MOVE_HOVER_COLOR_R, MOVE_HOVER_COLOR_G, MOVE_HOVER_COLOR_B, TILE_HOVER_OPACITY};
            } else {
                hover_color = (Color){255, 255, 255, TILE_DIM_OPACITY};
            }
            DrawTileHighlight(&board, &render, input.hover_pos.x, input.hover_pos.y, hover_color);
        }

        for (int i = 0; i < unit_count; i++) {
            DrawUnitShadow(&units[i], shadow, &render);
        }

        for (int i = 0; i < fx_count; i++) {
            DrawFXInstance(&fx_instances[i], &render);
        }

        for (int i = 0; i < unit_count; i++) {
            DrawUnit(&units[i], &render);
        }

        EndMode3D();

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
