#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include <SDL3/SDL.h>

enum class EntityState {
    Idle,
    Moving,
    Selected
};

struct Entity {
    BoardPos board_pos;
    Vec2 screen_pos;
    
    SDL_Texture* spritesheet;
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
    ~Entity();
    
    bool load(SDL_Renderer* renderer, const char* unit_name);
    void set_board_position(int window_w, int window_h, BoardPos pos);
    void play_animation(const char* name);
    void update(float dt, int window_w, int window_h);
    void render(SDL_Renderer* renderer, int window_w, int window_h) const;
    
    void start_move(int window_w, int window_h, BoardPos target);
    bool is_moving() const { return state == EntityState::Moving; }
};
