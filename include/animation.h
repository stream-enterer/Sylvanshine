#ifndef ANIMATION_H
#define ANIMATION_H

#include "types.h"

#include <raylib.h>

typedef struct {
    Animation* anim;
    float elapsed;
    int current_frame;
    bool looping;
    bool finished;
} AnimationPlayer;

int LoadAnimations(const char* filepath, Animation* animations, int max_count);
void InitAnimationPlayer(AnimationPlayer* player, Animation* anim, bool looping);
void UpdateAnimationPlayer(AnimationPlayer* player, float dt);
Rectangle GetCurrentFrameRect(AnimationPlayer* player);
Animation* FindAnimation(Animation* animations, int count, const char* name);

#endif
