#include "entity.hpp"
#include "asset_manager.hpp"
#include <SDL3_image/SDL_image.h>
#include <string>
#include <cmath>
#include <cstdlib>

GPUTextureHandle Entity::shadow_texture;
bool Entity::shadow_loaded = false;

constexpr float SHADOW_OPACITY = 200.0f / 255.0f;
constexpr float SHADOW_W = 96.0f;
constexpr float SHADOW_H = 48.0f;

Entity::Entity()
    : board_pos{0, 0}
    , screen_pos{0, 0}
    , current_anim(nullptr)
    , anim_time(0.0f)
    , flip_x(false)
    , original_flip_x(false)
    , state(EntityState::Spawning)
    , type(UnitType::Player)
    , attack_range(1)
    , hp(10)
    , max_hp(10)
    , attack_power(2)
    , move_target{0, 0}
    , move_start_pos{0, 0}
    , move_elapsed(0.0f)
    , move_duration(0.0f)
    , target_entity_idx(-1)
    , attack_damage_delay(0.5f)
    , attack_elapsed(0.0f)
    , attack_duration(0.0f)
    , attack_damage_dealt(false)
    , death_elapsed(0.0f)
    , death_duration(0.0f)
    , death_complete(false)
    , spawn_elapsed(0.0f)
    , spawn_duration(FADE_MEDIUM)
    , dissolve_elapsed(0.0f)
    , dissolve_duration(FADE_SLOW)
    , dissolve_seed(0.0f)
    , opacity(0.0f) {
    dissolve_seed = static_cast<float>(std::rand()) / RAND_MAX * 100.0f;
}

bool Entity::load_shadow() {
    if (shadow_loaded) return true;

    std::string shadow_path = AssetManager::instance().get_shadow_texture_path();
    shadow_texture = g_gpu.load_texture(shadow_path.c_str());
    if (!shadow_texture) {
        SDL_Log("Failed to load shadow texture: %s", shadow_path.c_str());
        return false;
    }

    shadow_loaded = true;
    SDL_Log("Shadow texture loaded from: %s", shadow_path.c_str());
    return true;
}

bool Entity::load(const char* unit_name) {
    // Get unit asset from AssetManager (pre-parsed from assets.json)
    const auto* asset = AssetManager::instance().get_unit(unit_name);
    if (!asset) {
        SDL_Log("Unit not found in assets: %s", unit_name);
        return false;
    }

    // Load spritesheet from dist directory
    std::string spritesheet_path = AssetManager::instance().get_unit_spritesheet_path(unit_name);
    spritesheet = g_gpu.load_texture(spritesheet_path.c_str());

    if (!spritesheet) {
        SDL_Log("Failed to load spritesheet: %s", spritesheet_path.c_str());
        return false;
    }

    // Copy pre-parsed animations from AssetManager
    animations = asset->animations;

    if (animations.animations.empty()) {
        SDL_Log("No animations found for unit: %s", unit_name);
        return false;
    }

    // Load timing data
    UnitTiming timing = AssetManager::instance().get_timing(unit_name);
    attack_damage_delay = timing.attack_damage_delay;

    play_animation("idle");

    state = EntityState::Spawning;
    spawn_elapsed = 0.0f;
    opacity = 0.0f;

    SDL_Log("Loaded unit '%s' from dist/", unit_name);
    return true;
}

void Entity::set_board_position(const RenderConfig& config, BoardPos pos) {
    board_pos = pos;
    screen_pos = board_to_screen_perspective(config, pos);
}

void Entity::set_stats(int health, int atk) {
    hp = health;
    max_hp = health;
    attack_power = atk;
}

void Entity::set_timing(float damage_delay) {
    attack_damage_delay = damage_delay;
}

void Entity::play_animation(const char* name) {
    const Animation* anim = animations.find(name);
    if (anim) {
        current_anim = anim;
        anim_time = 0.0f;
    }
}

bool Entity::should_deal_damage() const {
    if (state != EntityState::Attacking) return false;
    if (attack_damage_dealt) return false;
    return attack_elapsed >= attack_damage_delay;
}

void Entity::mark_damage_dealt() {
    attack_damage_dealt = true;
}

void Entity::update(float dt, const RenderConfig& config) {
    if (!current_anim) return;
    
    if (state == EntityState::Spawning) {
        spawn_elapsed += dt;
        float t = spawn_elapsed / spawn_duration;
        opacity = t < 1.0f ? t : 1.0f;
        
        if (spawn_elapsed >= spawn_duration) {
            state = EntityState::Idle;
            opacity = 1.0f;
        }
    }
    
    if (state == EntityState::Moving) {
        move_elapsed += dt;
        
        if (move_elapsed >= move_duration) {
            board_pos = move_target;
            screen_pos = board_to_screen_perspective(config, board_pos);
            state = EntityState::Idle;
            play_animation("idle");
        } else {
            float t = move_elapsed / move_duration;
            Vec2 target_pos = board_to_screen_perspective(config, move_target);
            screen_pos.x = move_start_pos.x + (target_pos.x - move_start_pos.x) * t;
            screen_pos.y = move_start_pos.y + (target_pos.y - move_start_pos.y) * t;
        }
    }
    
    if (state == EntityState::Attacking) {
        attack_elapsed += dt;
        
        if (attack_elapsed >= attack_duration) {
            state = EntityState::Idle;
            play_animation("idle");
            target_entity_idx = -1;
            attack_damage_dealt = false;
        }
    }
    
    if (state == EntityState::Dying) {
        death_elapsed += dt;
        if (death_elapsed >= death_duration) {
            start_dissolve();
        }
    }
    
    if (state == EntityState::Dissolving) {
        dissolve_elapsed += dt;
        
        float t = dissolve_elapsed / dissolve_duration;
        opacity = 1.0f - (t < 1.0f ? t : 1.0f);
        
        if (dissolve_elapsed >= dissolve_duration) {
            death_complete = true;
            opacity = 0.0f;
        }
    }
    
    anim_time += dt;
    float frame_duration = 1.0f / current_anim->fps;
    float anim_total_duration = frame_duration * current_anim->frames.size();
    
    if (state == EntityState::Dying || state == EntityState::Dissolving) {
        if (anim_time >= anim_total_duration) {
            anim_time = anim_total_duration - 0.001f;
        }
    } else {
        while (anim_time >= anim_total_duration) {
            anim_time -= anim_total_duration;
        }
    }
}

void Entity::render_shadow(const RenderConfig& config) const {
    if (is_dead()) return;
    if (!spritesheet.ptr || !current_anim || current_anim->frames.empty()) return;

    // Silhouette shadow: unit sprite projected onto ground plane with blur
    // Duelyst's UnitSprite has castsShadows: true, shadowOffset: CONFIG.DEPTH_OFFSET
    // Uses ShadowHighQuality/LowQuality shaders with 7x7/3x3 blur
    int frame_idx = static_cast<int>(anim_time * current_anim->fps);
    if (frame_idx >= static_cast<int>(current_anim->frames.size())) {
        frame_idx = current_anim->frames.size() - 1;
    }
    const SDL_Rect& src_rect = current_anim->frames[frame_idx].rect;

    SDL_FRect src = {
        static_cast<float>(src_rect.x),
        static_cast<float>(src_rect.y),
        static_cast<float>(src_rect.w),
        static_cast<float>(src_rect.h)
    };

    float shadow_alpha = SHADOW_OPACITY * opacity;
    g_gpu.draw_sprite_shadow(spritesheet, src, screen_pos, config.scale, flip_x, shadow_alpha);
}

void Entity::render(const RenderConfig& config) const {
    if (!spritesheet.ptr) {
        SDL_Log("Entity::render: no spritesheet");
        return;
    }
    if (!current_anim) {
        SDL_Log("Entity::render: no current_anim");
        return;
    }
    if (current_anim->frames.empty()) {
        SDL_Log("Entity::render: no frames in anim '%s'", current_anim->name);
        return;
    }
    if (is_dead()) return;

    int frame_idx = static_cast<int>(anim_time * current_anim->fps);
    if (frame_idx >= static_cast<int>(current_anim->frames.size())) {
        frame_idx = current_anim->frames.size() - 1;
    }
    const SDL_Rect& src_rect = current_anim->frames[frame_idx].rect;

    SDL_FRect src = {
        static_cast<float>(src_rect.x),
        static_cast<float>(src_rect.y),
        static_cast<float>(src_rect.w),
        static_cast<float>(src_rect.h)
    };

    float sprite_top_y = screen_pos.y - (src.h - SHADOW_OFFSET) * config.scale;

    SDL_FRect dst;
    dst.x = screen_pos.x - src.w * 0.5f * config.scale;
    dst.y = sprite_top_y;
    dst.w = src.w * config.scale;
    dst.h = src.h * config.scale;

    if (state == EntityState::Dissolving) {
        float dissolve_time = get_dissolve_time();
        g_gpu.draw_sprite_dissolve(spritesheet, src, dst, flip_x, opacity, dissolve_time, dissolve_seed);
    } else {
        g_gpu.draw_sprite(spritesheet, src, dst, flip_x, opacity);
    }
}

void Entity::render_hp_bar(const RenderConfig& config) const {
    if (is_dead()) return;
    if (state == EntityState::Spawning && spawn_elapsed < spawn_duration * 0.5f) return;
    if (state == EntityState::Dissolving) return;

    float hp_percent = static_cast<float>(hp) / max_hp;

    float bar_width = 60.0f * config.scale;
    float bar_height = 6.0f * config.scale;
    float bar_x = screen_pos.x - bar_width * 0.5f;
    float bar_y = screen_pos.y - 55.0f * config.scale;

    float alpha = opacity;

    SDL_FRect bg = {bar_x - 1, bar_y - 1, bar_width + 2, bar_height + 2};
    g_gpu.draw_quad_colored(bg, {0.0f, 0.0f, 0.0f, alpha});

    SDL_FRect bg_inner = {bar_x, bar_y, bar_width, bar_height};
    g_gpu.draw_quad_colored(bg_inner, {40.0f/255.0f, 40.0f/255.0f, 40.0f/255.0f, alpha});

    if (hp > 0) {
        SDL_FRect fg = {bar_x, bar_y, bar_width * hp_percent, bar_height};
        if (hp_percent > 0.66f) {
            g_gpu.draw_quad_colored(fg, {100.0f/255.0f, 1.0f, 100.0f/255.0f, alpha});
        } else if (hp_percent > 0.33f) {
            g_gpu.draw_quad_colored(fg, {1.0f, 1.0f, 100.0f/255.0f, alpha});
        } else {
            g_gpu.draw_quad_colored(fg, {1.0f, 100.0f/255.0f, 100.0f/255.0f, alpha});
        }
    }
}

void Entity::start_move(const RenderConfig& config, BoardPos target) {
    if (target == board_pos || !target.is_valid()) return;
    
    int dx = std::abs(target.x - board_pos.x);
    int dy = std::abs(target.y - board_pos.y);
    int tile_count = dx + dy;
    
    const Animation* run_anim = animations.find("run");
    if (!run_anim) run_anim = animations.find("walk");
    if (!run_anim) return;
    
    move_target = target;
    move_start_pos = screen_pos;
    move_elapsed = 0.0f;
    move_duration = calculate_move_duration(run_anim->duration(), tile_count);
    
    face_position(target);
    
    state = EntityState::Moving;
    play_animation("run");
}

void Entity::start_attack(int target_idx) {
    if (!can_act()) return;
    
    const Animation* attack_anim = animations.find("attack");
    if (!attack_anim) return;
    
    target_entity_idx = target_idx;
    attack_elapsed = 0.0f;
    attack_duration = attack_anim->duration();
    attack_damage_dealt = false;
    
    state = EntityState::Attacking;
    play_animation("attack");
    
    SDL_Log("Attack started (duration: %.2fs, damage at: %.2fs)", 
            attack_duration, attack_damage_delay);
}

void Entity::take_damage(int damage) {
    hp -= damage;
    if (hp < 0) hp = 0;
    
    SDL_Log("Unit took %d damage, HP now %d/%d", damage, hp, max_hp);
    
    if (hp == 0) {
        start_death();
    }
}

void Entity::start_death() {
    state = EntityState::Dying;
    death_elapsed = 0.0f;
    
    const Animation* death_anim = animations.find("death");
    if (death_anim) {
        death_duration = death_anim->duration();
        play_animation("death");
    } else {
        death_duration = 0.0f;
        start_dissolve();
    }
}

void Entity::start_dissolve() {
    state = EntityState::Dissolving;
    dissolve_elapsed = 0.0f;
    dissolve_duration = FADE_SLOW;
    
    SDL_Log("Dissolve started (duration: %.2fs)", dissolve_duration);
}

void Entity::face_position(BoardPos target) {
    if (target.x < board_pos.x) {
        flip_x = true;
    } else if (target.x > board_pos.x) {
        flip_x = false;
    }
}

void Entity::store_facing() {
    original_flip_x = flip_x;
}

void Entity::restore_facing() {
    flip_x = original_flip_x;
}
