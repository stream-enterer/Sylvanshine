#include "fx.h"

#include <stdio.h>
#include <string.h>

static const char* SPAWN_FX_SPRITES[] = {
    "fxTeleportRecallWhite",
    "fx_f1_holyimmolation",
    "fxSmokeGround"
};
static const int SPAWN_FX_COUNT = 3;

void InitFXSystem(FXSystem* system) {
    system->mapping_count = 0;
    memset(system->mappings, 0, sizeof(system->mappings));
}

void LoadRSXMappings(FXSystem* system, const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return;

    char line[512];
    int first = 1;

    while (fgets(line, sizeof(line), file) && system->mapping_count < MAX_RSX_MAPPINGS) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;

        if (first) {
            first = 0;
            continue;
        }

        RSXMapping* m = &system->mappings[system->mapping_count];

        char* tab1 = strchr(line, '\t');
        if (!tab1) continue;

        size_t name_len = (size_t)(tab1 - line);
        if (name_len >= sizeof(m->rsx_name)) name_len = sizeof(m->rsx_name) - 1;
        strncpy(m->rsx_name, line, name_len);
        m->rsx_name[name_len] = '\0';

        char* tab2 = strchr(tab1 + 1, '\t');
        if (!tab2) {
            size_t folder_len = strlen(tab1 + 1);
            if (folder_len >= sizeof(m->folder)) folder_len = sizeof(m->folder) - 1;
            strncpy(m->folder, tab1 + 1, folder_len);
            m->folder[folder_len] = '\0';
            m->prefix[0] = '\0';
        } else {
            size_t folder_len = (size_t)(tab2 - (tab1 + 1));
            if (folder_len >= sizeof(m->folder)) folder_len = sizeof(m->folder) - 1;
            strncpy(m->folder, tab1 + 1, folder_len);
            m->folder[folder_len] = '\0';

            size_t prefix_len = strlen(tab2 + 1);
            if (prefix_len >= sizeof(m->prefix)) prefix_len = sizeof(m->prefix) - 1;
            strncpy(m->prefix, tab2 + 1, prefix_len);
            m->prefix[prefix_len] = '\0';
        }

        system->mapping_count++;
    }

    fclose(file);
}

RSXMapping* FindRSXMapping(FXSystem* system, const char* rsx_name) {
    for (int i = 0; i < system->mapping_count; i++) {
        if (strcmp(system->mappings[i].rsx_name, rsx_name) == 0) {
            return &system->mappings[i];
        }
    }
    return NULL;
}

void CreateSpawnFX(FXSystem* system, FXInstance* fx, Vector2 position) {
    memset(fx, 0, sizeof(FXInstance));
    fx->position = position;
    fx->active = true;
    fx->sprite_count = 0;

    for (int i = 0; i < SPAWN_FX_COUNT && fx->sprite_count < MAX_FX_SPRITES; i++) {
        RSXMapping* mapping = FindRSXMapping(system, SPAWN_FX_SPRITES[i]);
        if (!mapping) continue;

        FXSprite* sprite = &fx->sprites[fx->sprite_count];
        char path[256];

        snprintf(path, sizeof(path), "data/fx/%s/spritesheet.png", mapping->folder);
        sprite->texture = LoadTexture(path);
        if (sprite->texture.id == 0) continue;

        snprintf(path, sizeof(path), "data/fx/%s/animations.txt", mapping->folder);
        Animation animations[16];
        int anim_count = LoadAnimations(path, animations, 16);

        Animation* found = NULL;
        for (int j = 0; j < anim_count; j++) {
            if (strcmp(animations[j].name, SPAWN_FX_SPRITES[i]) == 0) {
                found = &animations[j];
                break;
            }
        }

        if (found) {
            memcpy(&sprite->animation, found, sizeof(Animation));
            InitAnimationPlayer(&sprite->player, &sprite->animation, false);
            sprite->active = true;
            fx->sprite_count++;
        } else {
            UnloadTexture(sprite->texture);
        }
    }
}

void UpdateFXInstance(FXInstance* fx, float dt) {
    if (!fx->active) return;

    bool any_active = false;
    for (int i = 0; i < fx->sprite_count; i++) {
        if (fx->sprites[i].active) {
            UpdateAnimationPlayer(&fx->sprites[i].player, dt);
            if (fx->sprites[i].player.finished) {
                fx->sprites[i].active = false;
            } else {
                any_active = true;
            }
        }
    }

    if (!any_active) {
        fx->active = false;
    }
}

void DrawFXInstance(FXInstance* fx) {
    if (!fx->active) return;

    for (int i = 0; i < fx->sprite_count; i++) {
        if (!fx->sprites[i].active) continue;

        FXSprite* sprite = &fx->sprites[i];
        Rectangle src = GetCurrentFrameRect(&sprite->player);

        Rectangle dest = {
            fx->position.x - src.width * 0.5f,
            fx->position.y - src.height * 0.5f,
            src.width,
            src.height
        };

        DrawTexturePro(sprite->texture, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void UnloadFXInstance(FXInstance* fx) {
    for (int i = 0; i < fx->sprite_count; i++) {
        if (fx->sprites[i].texture.id != 0) {
            UnloadTexture(fx->sprites[i].texture);
        }
    }
    fx->sprite_count = 0;
    fx->active = false;
}

bool IsFXActive(FXInstance* fx) {
    return fx->active;
}
