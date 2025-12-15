#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "sdl_handles.hpp"
#include <SDL3/SDL.h>

enum class EntityState {
    Idle,
    Moving,
    Selected
};

struct Entity {
    BoardPos board_pos;
    Vec2 screen_pos;

    TextureHandle spritesheet;
    AnimationSet animations;
    const Animation* current_anim;
    float anim_time;
    bool flip_x;

    EntityState state;
    BoardPos move_target;
    Vec2 move_start_pos;
    float move_elapsed;
    float move_duration;

    Entity();
    // Compiler-generated destructor, move constructor, and move assignment are correct
    // Copy operations remain deleted
    
    bool load(SDL_Renderer* renderer, const char* unit_name);
    void set_board_position(const RenderConfig& config, BoardPos pos);
    void play_animation(const char* name);
    void update(float dt, const RenderConfig& config);
    void render(SDL_Renderer* renderer, const RenderConfig& config) const;
    
    void start_move(const RenderConfig& config, BoardPos target);
    bool is_moving() const { return state == EntityState::Moving; }
};
