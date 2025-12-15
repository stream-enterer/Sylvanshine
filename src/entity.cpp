#include "entity.hpp"
#include <SDL3_image/SDL_image.h>
#include <string>
#include <cmath>

Entity::Entity() 
    : board_pos{0, 0}
    , screen_pos{0, 0}
    , spritesheet(nullptr)
    , current_anim(nullptr)
    , anim_time(0.0f)
    , flip_x(false)
    , state(EntityState::Idle)
    , move_target{0, 0}
    , move_start_pos{0, 0}
    , move_elapsed(0.0f)
    , move_duration(0.0f) {
}

Entity::~Entity() {
    if (spritesheet) {
        SDL_DestroyTexture(spritesheet);
    }
}

Entity::Entity(Entity&& other) noexcept
    : board_pos(other.board_pos)
    , screen_pos(other.screen_pos)
    , spritesheet(other.spritesheet)
    , animations(std::move(other.animations))
    , current_anim(other.current_anim)
    , anim_time(other.anim_time)
    , flip_x(other.flip_x)
    , state(other.state)
    , move_target(other.move_target)
    , move_start_pos(other.move_start_pos)
    , move_elapsed(other.move_elapsed)
    , move_duration(other.move_duration) {
    other.spritesheet = nullptr;
    other.current_anim = nullptr;
}

Entity& Entity::operator=(Entity&& other) noexcept {
    if (this != &other) {
        if (spritesheet) {
            SDL_DestroyTexture(spritesheet);
        }
        
        board_pos = other.board_pos;
        screen_pos = other.screen_pos;
        spritesheet = other.spritesheet;
        animations = std::move(other.animations);
        current_anim = other.current_anim;
        anim_time = other.anim_time;
        flip_x = other.flip_x;
        state = other.state;
        move_target = other.move_target;
        move_start_pos = other.move_start_pos;
        move_elapsed = other.move_elapsed;
        move_duration = other.move_duration;
        
        other.spritesheet = nullptr;
        other.current_anim = nullptr;
    }
    return *this;
}

bool Entity::load(SDL_Renderer* renderer, const char* unit_name) {
    std::string base_path = "data/units/";
    base_path += unit_name;
    
    std::string spritesheet_path = base_path + "/spritesheet.png";
    SDL_Surface* surface = IMG_Load(spritesheet_path.c_str());
    if (!surface) {
        SDL_Log("Failed to load spritesheet: %s - %s", spritesheet_path.c_str(), SDL_GetError());
        return false;
    }
    
    spritesheet = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    
    if (!spritesheet) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return false;
    }
    
    SDL_SetTextureScaleMode(spritesheet, SDL_SCALEMODE_NEAREST);
    
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
    screen_pos = board_to_screen(config, pos);
}

void Entity::play_animation(const char* name) {
    const Animation* anim = animations.find(name);
    if (anim) {
        current_anim = anim;
        anim_time = 0.0f;
    }
}

void Entity::update(float dt, const RenderConfig& config) {
    if (!current_anim) return;
    
    if (state == EntityState::Moving) {
        move_elapsed += dt;
        
        if (move_elapsed >= move_duration) {
            board_pos = move_target;
            screen_pos = board_to_screen(config, board_pos);
            state = EntityState::Idle;
            play_animation("idle");
        } else {
            float t = move_elapsed / move_duration;
            Vec2 target_pos = board_to_screen(config, move_target);
            screen_pos.x = move_start_pos.x + (target_pos.x - move_start_pos.x) * t;
            screen_pos.y = move_start_pos.y + (target_pos.y - move_start_pos.y) * t;
        }
    }
    
    anim_time += dt;
    float frame_duration = 1.0f / current_anim->fps;
    while (anim_time >= frame_duration * current_anim->frames.size()) {
        anim_time -= frame_duration * current_anim->frames.size();
    }
}

void Entity::render(SDL_Renderer* renderer, const RenderConfig& config) const {
    if (!spritesheet || !current_anim || current_anim->frames.empty()) return;
    
    int frame_idx = static_cast<int>(anim_time * current_anim->fps) % current_anim->frames.size();
    const SDL_Rect& src_rect = current_anim->frames[frame_idx].rect;
    
    SDL_FRect src = {
        static_cast<float>(src_rect.x),
        static_cast<float>(src_rect.y),
        static_cast<float>(src_rect.w),
        static_cast<float>(src_rect.h)
    };
    
    SDL_FRect dst;
    if (flip_x) {
        dst.x = screen_pos.x + src.w * 0.5f * config.scale;
        dst.y = screen_pos.y - src.h * 0.5f * config.scale;
        dst.w = -src.w * config.scale;
        dst.h = src.h * config.scale;
    } else {
        dst.x = screen_pos.x - src.w * 0.5f * config.scale;
        dst.y = screen_pos.y - src.h * 0.5f * config.scale;
        dst.w = src.w * config.scale;
        dst.h = src.h * config.scale;
    }
    
    SDL_RenderTexture(renderer, spritesheet, &src, &dst);
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
    
    flip_x = (target.x < board_pos.x);
    
    state = EntityState::Moving;
    play_animation("run");
}
