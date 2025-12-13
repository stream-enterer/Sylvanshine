#include "unit.h"

#include <stdio.h>
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
    unit->board_pos = (BoardPos){0, 0};
    unit->screen_pos = (Vector2){0, 0};
    unit->move_progress = 0.0f;

    Animation* breathing = FindAnimation(unit->animations, unit->animation_count, "breathing");
    if (breathing) {
        InitAnimationPlayer(&unit->player, breathing, true);
    }
}

void UnloadUnit(Unit* unit) {
    UnloadTexture(unit->spritesheet);
}

void UpdateUnit(Unit* unit, Board* board, float dt) {
    if (!unit->active) return;

    UpdateAnimationPlayer(&unit->player, dt);

    if (unit->state == UNIT_STATE_MOVING) {
        unit->move_progress += dt / MOVE_DURATION;
        if (unit->move_progress >= 1.0f) {
            unit->move_progress = 1.0f;
            unit->screen_pos = unit->move_target;
            unit->state = UNIT_STATE_IDLE;
            SetUnitAnimation(unit, "breathing", true);
        } else {
            float t = unit->move_progress;
            t = t * t * (3.0f - 2.0f * t);
            unit->screen_pos.x = unit->move_start.x + (unit->move_target.x - unit->move_start.x) * t;
            unit->screen_pos.y = unit->move_start.y + (unit->move_target.y - unit->move_start.y) * t;
        }
    } else if (unit->state == UNIT_STATE_SPAWNING) {
        if (unit->player.finished) {
            unit->state = UNIT_STATE_IDLE;
            SetUnitAnimation(unit, "breathing", true);
        }
    } else {
        unit->screen_pos = BoardToScreen(board, unit->board_pos.x, unit->board_pos.y);
    }
}

void DrawUnit(Unit* unit, Texture2D shadow) {
    if (!unit->active) return;

    Rectangle shadow_src = {0, 0, (float)shadow.width, (float)shadow.height};
    Rectangle shadow_dest = {
        unit->screen_pos.x - shadow.width * 0.5f,
        unit->screen_pos.y - shadow.height * 0.5f + 20,
        (float)shadow.width,
        (float)shadow.height
    };
    Color shadow_color = {255, 255, 255, SHADOW_OPACITY};
    DrawTexturePro(shadow, shadow_src, shadow_dest, (Vector2){0, 0}, 0.0f, shadow_color);

    Rectangle src = GetCurrentFrameRect(&unit->player);
    Rectangle dest = {
        unit->screen_pos.x - src.width * 0.5f,
        unit->screen_pos.y - src.height * 0.5f - 20,
        src.width,
        src.height
    };

    Color tint = WHITE;
    if (unit->selected) {
        tint = (Color){200, 255, 200, 255};
    }

    DrawTexturePro(unit->spritesheet, src, dest, (Vector2){0, 0}, 0.0f, tint);
}

void SetUnitAnimation(Unit* unit, const char* anim_name, bool looping) {
    Animation* anim = FindAnimation(unit->animations, unit->animation_count, anim_name);
    if (anim) {
        InitAnimationPlayer(&unit->player, anim, looping);
    }
}

void StartUnitMove(Unit* unit, Board* board, BoardPos target) {
    unit->move_start = unit->screen_pos;
    unit->move_target = BoardToScreen(board, target.x, target.y);
    unit->board_pos = target;
    unit->move_progress = 0.0f;
    unit->state = UNIT_STATE_MOVING;
    SetUnitAnimation(unit, "run", true);
}

void SpawnUnit(Unit* unit, Board* board, BoardPos pos) {
    unit->board_pos = pos;
    unit->screen_pos = BoardToScreen(board, pos.x, pos.y);
    unit->state = UNIT_STATE_SPAWNING;

    Animation* apply = FindAnimation(unit->animations, unit->animation_count, "apply");
    if (apply) {
        InitAnimationPlayer(&unit->player, apply, false);
    } else {
        unit->state = UNIT_STATE_IDLE;
        SetUnitAnimation(unit, "breathing", true);
    }
}
