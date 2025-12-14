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
    SetTargetFPS(0);

    RenderState render;
    InitRenderState(&render);

    Board board;
    InitBoard(&board);

    printf("\n=== DEBUG INFO ===\n");
    printf("1. Background texture: %d x %d pixels\n", board.background.width, board.background.height);
    printf("2. Board origin: (%.2f, %.2f)\n", render.board_origin_x, render.board_origin_y);
    printf("3. Board center Y: %.2f\n", render.board_center_y);
    printf("4. Board size: %d cols x %d rows\n", BOARD_COLS, BOARD_ROWS);
    printf("5. Tile size: %d pixels\n", TILE_SIZE);
    printf("6. Window size: %d x %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT);
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
        Vector2 spawn_screen = TileCenterScreen(&render, 4, 2);
        Vector2 spawn_tilted = ApplyTilt(&render, spawn_screen);
        CreateSpawnFX(&fx_system, &fx_instances[fx_count], spawn_tilted);
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
        DrawBoardFloor(&render);
        DrawBoardGrid(&board, &render);

        bool hover_tiles[BOARD_COLS][BOARD_ROWS] = {0};
        if (input.hover_valid && input.selected_unit && IsMoveValid(&input, input.hover_pos)) {
            hover_tiles[input.hover_pos.x][input.hover_pos.y] = true;
        }

        Color move_color = {MOVE_COLOR_R, MOVE_COLOR_G, MOVE_COLOR_B, TILE_SELECT_OPACITY};
        Color hover_color = {MOVE_HOVER_COLOR_R, MOVE_HOVER_COLOR_G, MOVE_HOVER_COLOR_B, TILE_HOVER_OPACITY};

        DrawMergedMoveTiles(&board, &render, input.move_tiles, hover_tiles, move_color, hover_color);

        for (int i = 0; i < unit_count; i++) {
            DrawUnitShadow(&units[i], shadow, &render);
        }

        for (int i = 0; i < fx_count; i++) {
            DrawFXInstance(&fx_instances[i], &render);
        }

        for (int i = 0; i < unit_count; i++) {
            DrawUnit(&units[i], &render);
        }

        DrawFPS(10, 10);

        Vector2 mouse = GetMousePosition();
        BoardPos board_pos = ScreenToBoard(&render, mouse);
        DrawText(TextFormat("Screen: %.0f, %.0f", mouse.x, mouse.y), 10, 30, 20, WHITE);
        DrawText(TextFormat("Board: %d, %d", board_pos.x, board_pos.y), 10, 50, 20, WHITE);

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
