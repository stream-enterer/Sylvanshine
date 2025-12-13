#ifndef UNIT_H
#define UNIT_H

#include "animation.h"
#include "board.h"
#include "types.h"

#include <raylib.h>

typedef enum {
    UNIT_STATE_IDLE,
    UNIT_STATE_MOVING,
    UNIT_STATE_SPAWNING
} UnitState;

typedef struct {
    Texture2D spritesheet;
    Animation animations[MAX_ANIMATIONS];
    int animation_count;
    AnimationPlayer player;

    BoardPos board_pos;
    Vector2 screen_pos;
    Vector2 move_start;
    Vector2 move_target;
    float move_progress;

    UnitState state;
    bool selected;
    bool active;
} Unit;

void LoadUnit(Unit* unit, const char* name);
void UnloadUnit(Unit* unit);
void UpdateUnit(Unit* unit, Board* board, float dt);
void DrawUnit(Unit* unit, Texture2D shadow);
void SetUnitAnimation(Unit* unit, const char* anim_name, bool looping);
void StartUnitMove(Unit* unit, Board* board, BoardPos target);
void SpawnUnit(Unit* unit, Board* board, BoardPos pos);

#endif
