#ifndef UNIT_H
#define UNIT_H

#include "animation.h"
#include "render.h"
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
    BoardPos prev_board_pos;
    Vector3 world_pos;
    Vector3 move_start;
    Vector3 move_target;
    float move_progress;
    float move_duration;
    int move_distance;

    float spawn_time;
    float shadow_alpha;

    UnitState state;
    bool selected;
    bool active;
    bool facing_left;
} Unit;

void LoadUnit(Unit* unit, const char* name);
void UnloadUnit(Unit* unit);
void UpdateUnit(Unit* unit, RenderState* render, float dt);
void DrawUnitShadow(Unit* unit, Texture2D shadow, RenderState* render);
void DrawUnit(Unit* unit, RenderState* render);
void SetUnitAnimation(Unit* unit, const char* anim_name, bool looping);
void StartUnitMove(Unit* unit, RenderState* render, BoardPos target);
void SpawnUnit(Unit* unit, RenderState* render, BoardPos pos);

#endif
