#include "entity.hpp"
#include <SDL3_image/SDL_image.h>
#include <string>
#include <cmath>

Entity::Entity()
    : board_pos{0, 0}
    , screen_pos{0, 0}
    , current_anim(nullptr)
    , anim_time(0.0f)
    , flip_x(false)
    , original_flip_x(false)
    , state(EntityState::Idle)
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
    , death_complete(false) {
}

bool Entity::load(SDL_Renderer* renderer, const char* unit_name) {
    std::string base_path = "data/units/";
    base_path += unit_name;

    std::string spritesheet_path = base_path + "/spritesheet.png";
    SurfaceHandle surface(IMG_Load(spritesheet_path.c_str()));
    if (!surface) {
        SDL_Log("Failed to load spritesheet: %s - %s", spritesheet_path.c_str(), SDL_GetError());
        return false;
    }

    spritesheet = TextureHandle(SDL_CreateTextureFromSurface(renderer, surface.get()));

    if (!spritesheet) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(spritesheet.get(), SDL_SCALEMODE_NEAREST);

    std::string anim_path = base_path + "/animations.txt";
    animations = load_animations(anim_path.c_str());

    if (animations.animations.empty()) {
        return false;
    }

    play_animation("idle");
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
            death_complete = true;
        }
    }
    
    anim_time += dt;
    float frame_duration = 1.0f / current_anim->fps;
    float anim_total_duration = frame_duration * current_anim->frames.size();
    
    if (state == EntityState::Dying) {
        if (anim_time >= anim_total_duration) {
            anim_time = anim_total_duration - 0.001f;
        }
    } else {
        while (anim_time >= anim_total_duration) {
            anim_time -= anim_total_duration;
        }
    }
}

void Entity::render(SDL_Renderer* renderer, const RenderConfig& config) const {
    if (!spritesheet || !current_anim || current_anim->frames.empty()) return;

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

    // Position sprite so feet (SHADOW_OFFSET from bottom) align with tile center
    // SDL Y-down: dst.y is top of sprite, so sprite top = ground - (height - shadowOffset)
    float sprite_top_y = screen_pos.y - (src.h - SHADOW_OFFSET) * config.scale;

    SDL_FRect dst;
    if (flip_x) {
        dst.x = screen_pos.x + src.w * 0.5f * config.scale;
        dst.y = sprite_top_y;
        dst.w = -src.w * config.scale;
        dst.h = src.h * config.scale;
    } else {
        dst.x = screen_pos.x - src.w * 0.5f * config.scale;
        dst.y = sprite_top_y;
        dst.w = src.w * config.scale;
        dst.h = src.h * config.scale;
    }

    SDL_RenderTexture(renderer, spritesheet.get(), &src, &dst);
}

void Entity::render_hp_bar(SDL_Renderer* renderer, const RenderConfig& config) const {
    if (is_dead()) return;
    
    float hp_percent = static_cast<float>(hp) / max_hp;
    
    float bar_width = 60.0f * config.scale;
    float bar_height = 6.0f * config.scale;
    float bar_x = screen_pos.x - bar_width * 0.5f;
    float bar_y = screen_pos.y - 55.0f * config.scale;
    
    SDL_FRect bg = {bar_x - 1, bar_y - 1, bar_width + 2, bar_height + 2};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bg);
    
    SDL_FRect bg_inner = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bg_inner);
    
    if (hp > 0) {
        SDL_FRect fg = {bar_x, bar_y, bar_width * hp_percent, bar_height};
        if (hp_percent > 0.66f) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        } else if (hp_percent > 0.33f) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &fg);
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
    death_complete = false;
    
    const Animation* death_anim = animations.find("death");
    if (death_anim) {
        death_duration = death_anim->duration();
        play_animation("death");
    } else {
        death_duration = 0.5f;
    }
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
