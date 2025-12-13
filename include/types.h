#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BOARD_COLS 9
#define BOARD_ROWS 5
#define TILE_SIZE 95
#define TILE_OFFSET_X 0.0f
#define TILE_OFFSET_Y 10.0f
#define TILE_SELECT_OPACITY 200
#define TILE_HOVER_OPACITY 200
#define TILE_DIM_OPACITY 127
#define SHADOW_OPACITY 200
#define MAX_ANIMATIONS 16
#define MAX_FRAMES 64
#define MAX_UNITS 32
#define MAX_FX_SPRITES 8
#define MOVEMENT_RANGE 2
#define MOVE_DURATION 0.5f

typedef struct {
    int x;
    int y;
} BoardPos;

typedef struct {
    int idx;
    int x;
    int y;
    int w;
    int h;
} Frame;

typedef struct {
    char name[64];
    int fps;
    Frame frames[MAX_FRAMES];
    int frame_count;
} Animation;

#endif
