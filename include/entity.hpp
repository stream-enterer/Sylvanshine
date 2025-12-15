#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "sdl_handles.hpp"
#include <SDL3/SDL.h>

enum class EntityState {
    Idle,
    Moving,
    Attacking,
    TakingDamage,
    Dying
};

enum class UnitType {
    Player,
    Enemy
};

struct Entity {
    BoardPos board_pos;
    Vec2 screen_pos;

    TextureHandle spritesheet;
    AnimationSet animations;
    const Animation* current_anim;
    float anim_time;
    bool flip_x;
    bool original_flip_x;

    EntityState state;
    UnitType type;
    int attack_range;
    
    BoardPos move_target;
    Vec2 move_start_pos;
    float move_elapsed;
    float move_duration;
    
    int target_entity_idx;
    float attack_elapsed;
    float attack_duration;

    Entity();
    
    bool load(SDL_Renderer* renderer, const char* unit_name);
    void set_board_position(const RenderConfig& config, BoardPos pos);
    void play_animation(const char* name);
    void update(float dt, const RenderConfig& config);
    void render(SDL_Renderer* renderer, const RenderConfig& config) const;
    
    void start_move(const RenderConfig& config, BoardPos target);
    void start_attack(int target_idx);
    void face_position(BoardPos target);
    void store_facing();
    void restore_facing();
    bool is_moving() const { return state == EntityState::Moving; }
    bool is_attacking() const { return state == EntityState::Attacking; }
    bool can_act() const { return state == EntityState::Idle; }
};
