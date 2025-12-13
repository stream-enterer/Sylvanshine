#include "animation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int LoadAnimations(const char* filepath, Animation* animations, int max_count) {
    FILE* file = fopen(filepath, "r");
    if (!file) return 0;

    char line[4096];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < max_count) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;

        Animation* anim = &animations[count];
        memset(anim, 0, sizeof(Animation));

        char* name_end = strchr(line, '\t');
        if (!name_end) continue;

        size_t name_len = (size_t)(name_end - line);
        if (name_len >= sizeof(anim->name)) name_len = sizeof(anim->name) - 1;
        strncpy(anim->name, line, name_len);
        anim->name[name_len] = '\0';

        char* fps_start = name_end + 1;
        anim->fps = atoi(fps_start);
        if (anim->fps <= 0) anim->fps = 8;

        char* data_start = strchr(fps_start, '\t');
        if (!data_start) continue;
        data_start++;

        int comma_count = 0;
        for (char* p = data_start; *p; p++) {
            if (*p == ',') comma_count++;
        }
        anim->frame_count = (comma_count + 1) / 5;
        if (anim->frame_count > MAX_FRAMES) anim->frame_count = MAX_FRAMES;

        char* p = data_start;
        for (int i = 0; i < anim->frame_count; i++) {
            anim->frames[i].idx = (int)strtol(p, &p, 10);
            if (*p == ',') p++;
            anim->frames[i].x = (int)strtol(p, &p, 10);
            if (*p == ',') p++;
            anim->frames[i].y = (int)strtol(p, &p, 10);
            if (*p == ',') p++;
            anim->frames[i].w = (int)strtol(p, &p, 10);
            if (*p == ',') p++;
            anim->frames[i].h = (int)strtol(p, &p, 10);
            if (*p == ',') p++;
        }

        count++;
    }

    fclose(file);
    return count;
}

void InitAnimationPlayer(AnimationPlayer* player, Animation* anim, bool looping) {
    player->anim = anim;
    player->elapsed = 0.0f;
    player->current_frame = 0;
    player->looping = looping;
    player->finished = false;
}

void UpdateAnimationPlayer(AnimationPlayer* player, float dt) {
    if (!player->anim || player->finished) return;

    player->elapsed += dt;
    float frame_duration = 1.0f / player->anim->fps;
    int total_frames = player->anim->frame_count;

    int frame_index = (int)(player->elapsed / frame_duration);

    if (player->looping) {
        player->current_frame = frame_index % total_frames;
    } else {
        if (frame_index >= total_frames) {
            player->current_frame = total_frames - 1;
            player->finished = true;
        } else {
            player->current_frame = frame_index;
        }
    }
}

Rectangle GetCurrentFrameRect(AnimationPlayer* player) {
    Rectangle rect = {0, 0, 0, 0};
    if (!player->anim || player->anim->frame_count == 0) return rect;

    Frame* frame = &player->anim->frames[player->current_frame];
    rect.x = (float)frame->x;
    rect.y = (float)frame->y;
    rect.width = (float)frame->w;
    rect.height = (float)frame->h;
    return rect;
}

Animation* FindAnimation(Animation* animations, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(animations[i].name, name) == 0) {
            return &animations[i];
        }
    }
    return NULL;
}
