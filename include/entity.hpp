#pragma once
#include "types.hpp"
#include "animation_loader.hpp"
#include "sdl_handles.hpp"
#include <SDL3/SDL.h>

constexpr float FADE_FAST = 0.2f;
constexpr float FADE_MEDIUM = 0.35f;
constexpr float FADE_SLOW = 1.0f;

enum class EntityState {
    Spawning,
    Idle,
    Moving,
    Attacking,
    TakingDamage,
    Dying,
    Dissolving
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
    
    static TextureHandle shadow_texture;
    static bool shadow_loaded;
    
    const Animation* current_anim;
    float anim_time;
    bool flip_x;
    bool original_flip_x;

    EntityState state;
    UnitType type;
    int attack_range;
    
    int hp;
    int max_hp;
    int attack_power;
    
    BoardPos move_target;
    Vec2 move_start_pos;
    float move_elapsed;
    float move_duration;
    
    int target_entity_idx;
    float attack_damage_delay;
    float attack_elapsed;
    float attack_duration;
    bool attack_damage_dealt;
    
    float death_elapsed;
    float death_duration;
    bool death_complete;

    float spawn_elapsed;
    float spawn_duration;
    
    float dissolve_elapsed;
    float dissolve_duration;
    float dissolve_seed;
    
    float opacity;

    Entity();
    
    bool load(SDL_Renderer* renderer, const char* unit_name);
    void set_board_position(const RenderConfig& config, BoardPos pos);
    void set_stats(int health, int atk);
    void set_timing(float damage_delay);
    void play_animation(const char* name);
    void update(float dt, const RenderConfig& config);
    void render(SDL_Renderer* renderer, const RenderConfig& config) const;
    void render_shadow(SDL_Renderer* renderer, const RenderConfig& config) const;
    void render_hp_bar(SDL_Renderer* renderer, const RenderConfig& config) const;
    
    static bool load_shadow(SDL_Renderer* renderer);
    
    void start_move(const RenderConfig& config, BoardPos target);
    void start_attack(int target_idx);
    void mark_damage_dealt();
    void take_damage(int damage);
    void start_death();
    void start_dissolve();
    void face_position(BoardPos target);
    void store_facing();
    void restore_facing();
    
    bool is_spawning() const { return state == EntityState::Spawning; }
    bool is_moving() const { return state == EntityState::Moving; }
    bool is_attacking() const { return state == EntityState::Attacking; }
    bool is_dying() const { return state == EntityState::Dying; }
    bool is_dissolving() const { return state == EntityState::Dissolving; }
    bool is_dead() const { return death_complete; }
    bool can_act() const { return state == EntityState::Idle; }
    int get_target_idx() const { return target_entity_idx; }
    bool should_deal_damage() const;
    
    float get_opacity() const { return opacity; }
    float get_dissolve_time() const { return dissolve_elapsed / dissolve_duration; }
    float get_dissolve_seed() const { return dissolve_seed; }
};
