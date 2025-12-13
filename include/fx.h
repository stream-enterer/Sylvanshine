#ifndef FX_H
#define FX_H

#include "animation.h"
#include "render.h"
#include "types.h"

#include <raylib.h>

#define MAX_RSX_MAPPINGS 256

typedef struct {
    char rsx_name[64];
    char folder[64];
    char prefix[64];
} RSXMapping;

typedef struct {
    Texture2D texture;
    Animation animation;
    AnimationPlayer player;
    bool active;
} FXSprite;

typedef struct {
    FXSprite sprites[MAX_FX_SPRITES];
    int sprite_count;
    Vector3 position;
    bool active;
} FXInstance;

typedef struct {
    RSXMapping mappings[MAX_RSX_MAPPINGS];
    int mapping_count;
} FXSystem;

void InitFXSystem(FXSystem* system);
void LoadRSXMappings(FXSystem* system, const char* filepath);
RSXMapping* FindRSXMapping(FXSystem* system, const char* rsx_name);
void CreateSpawnFX(FXSystem* system, FXInstance* fx, Vector3 position);
void UpdateFXInstance(FXInstance* fx, float dt);
void DrawFXInstance(FXInstance* fx, RenderState* render);
void UnloadFXInstance(FXInstance* fx);
bool IsFXActive(FXInstance* fx);

#endif
