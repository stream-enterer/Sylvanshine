#include "unit.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void LoadUnit(Unit* unit, const char* name) {
    char path[256];

    snprintf(path, sizeof(path), "data/units/%s/spritesheet.png", name);
    unit->spritesheet = LoadTexture(path);

    snprintf(path, sizeof(path), "data/units/%s/animations.txt", name);
    unit->animation_count = LoadAnimations(path, unit->animations, MAX_ANIMATIONS);

    unit->state = UNIT_STATE_IDLE;
    unit->selected = false;
    unit->active = true;
    unit->facing_left = false;
    unit->board_pos = (BoardPos){0, 0};
    unit->prev_board_pos = (BoardPos){0, 0};
    unit->world_pos = (Vector3){0, 0, 0};
    unit->move_progress = 0.0f;
    unit->move_duration = 0.0f;
    unit->move_distance = 0;
    unit->spawn_time = 0.0f;
    unit->shadow_alpha = SHADOW_OPACITY;

    Animation* breathing = FindAnimation(unit->animations, unit->animation_count, "breathing");
    if (breathing) {
        InitAnimationPlayer(&unit->player, breathing, true);
    }
}

void UnloadUnit(Unit* unit) {
    UnloadTexture(unit->spritesheet);
}

static float Smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void UpdateUnit(Unit* unit, RenderState* render, float dt) {
    if (!unit->active) return;

    UpdateAnimationPlayer(&unit->player, dt);

    if (unit->state == UNIT_STATE_MOVING) {
        unit->move_progress += dt / unit->move_duration;
        if (unit->move_progress >= 1.0f) {
            unit->move_progress = 1.0f;
            unit->world_pos = unit->move_target;
            unit->state = UNIT_STATE_IDLE;
            SetUnitAnimation(unit, "breathing", true);
        } else {
            float t = Smoothstep(unit->move_progress);
            unit->world_pos.x = unit->move_start.x + (unit->move_target.x - unit->move_start.x) * t;
            unit->world_pos.y = unit->move_start.y + (unit->move_target.y - unit->move_start.y) * t;
            unit->world_pos.z = unit->move_start.z + (unit->move_target.z - unit->move_start.z) * t;
        }
    } else if (unit->state == UNIT_STATE_SPAWNING) {
        unit->spawn_time += dt;

        float fade_progress = unit->spawn_time / FADE_MEDIUM_DURATION;
        if (fade_progress > 1.0f) fade_progress = 1.0f;
        unit->shadow_alpha = fade_progress * SHADOW_OPACITY;

        if (unit->player.finished) {
            unit->state = UNIT_STATE_IDLE;
            unit->shadow_alpha = SHADOW_OPACITY;
            SetUnitAnimation(unit, "breathing", true);
        }
    } else {
        unit->world_pos = BoardToWorld(render, unit->board_pos.x, unit->board_pos.y);
    }
}

void DrawUnitShadow(Unit* unit, Texture2D shadow, RenderState* render) {
    if (!unit->active) return;

    (void)render;

    Vector3 shadow_pos = unit->world_pos;
    shadow_pos.y = 0.001f;

    float shadow_width = shadow.width * WORLD_SCALE;
    float shadow_height = shadow.height * WORLD_SCALE;

    Rectangle src = {0, 0, (float)shadow.width, (float)shadow.height};
    Color tint = {255, 255, 255, (unsigned char)unit->shadow_alpha};

    DrawTexturedQuad(shadow, src, shadow_pos, shadow_width, shadow_height, BOARD_XYZ_ROTATION, tint, false);
}

void DrawUnit(Unit* unit, RenderState* render) {
    if (!unit->active) return;

    (void)render;

    Rectangle src = GetCurrentFrameRect(&unit->player);
    if (src.width == 0 || src.height == 0) return;

    float sprite_width = src.width * WORLD_SCALE;
    float sprite_height = src.height * WORLD_SCALE;

    Vector3 sprite_pos = unit->world_pos;
    sprite_pos.y = sprite_height * 0.5f;

    Color tint = WHITE;
    if (unit->selected) {
        tint = (Color){200, 255, 200, 255};
    }

    DrawTexturedQuad(unit->spritesheet, src, sprite_pos, sprite_width, sprite_height, ENTITY_XYZ_ROTATION, tint, unit->facing_left);
}

void SetUnitAnimation(Unit* unit, const char* anim_name, bool looping) {
    Animation* anim = FindAnimation(unit->animations, unit->animation_count, anim_name);
    if (anim) {
        InitAnimationPlayer(&unit->player, anim, looping);
    }
}

void StartUnitMove(Unit* unit, RenderState* render, BoardPos target) {
    unit->prev_board_pos = unit->board_pos;
    unit->move_start = unit->world_pos;
    unit->move_target = BoardToWorld(render, target.x, target.y);

    int dx = abs(target.x - unit->board_pos.x);
    int dy = abs(target.y - unit->board_pos.y);
    unit->move_distance = dx + dy;
    unit->move_duration = unit->move_distance * MOVE_DURATION_PER_TILE;

    if (target.x < unit->board_pos.x) {
        unit->facing_left = true;
    } else if (target.x > unit->board_pos.x) {
        unit->facing_left = false;
    }

    unit->board_pos = target;
    unit->move_progress = 0.0f;
    unit->state = UNIT_STATE_MOVING;
    SetUnitAnimation(unit, "run", true);
}

void SpawnUnit(Unit* unit, RenderState* render, BoardPos pos) {
    unit->board_pos = pos;
    unit->prev_board_pos = pos;
    unit->world_pos = BoardToWorld(render, pos.x, pos.y);
    unit->state = UNIT_STATE_SPAWNING;
    unit->spawn_time = 0.0f;
    unit->shadow_alpha = 0.0f;

    Animation* apply = FindAnimation(unit->animations, unit->animation_count, "apply");
    if (apply) {
        InitAnimationPlayer(&unit->player, apply, false);
    } else {
        unit->state = UNIT_STATE_IDLE;
        unit->shadow_alpha = SHADOW_OPACITY;
        SetUnitAnimation(unit, "breathing", true);
    }
}
