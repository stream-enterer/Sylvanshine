#ifndef UNIT_HPP
#define UNIT_HPP

#include "animation_player.hpp"
#include "asset_manager.hpp"
#include "types.hpp"

#include <glm/glm.hpp>

#include <string>

class Unit {
public:
    Unit();

    bool Initialize(AssetManager* assets, const std::string& unit_id,
                    int faction_id, BoardPosition position);

    void Update(float dt);

    void StartSpawn();
    void StartIdle();
    void StartMove(BoardPosition target);
    void StartAttack();
    void StartHit();
    void StartDeath();

    void SetScreenPosition(const glm::vec2& pos) { screen_position_ = pos; }
    glm::vec2 GetScreenPosition() const { return screen_position_; }

    BoardPosition GetBoardPosition() const { return board_position_; }
    void SetBoardPosition(BoardPosition pos) { board_position_ = pos; }

    UnitState GetState() const { return state_; }
    bool IsAlive() const { return state_ != UnitState::DEAD; }
    bool IsSpawning() const { return state_ == UnitState::SPAWNING; }
    bool IsIdle() const { return state_ == UnitState::IDLE; }
    bool IsMoving() const { return state_ == UnitState::MOVING; }
    bool IsAttacking() const { return state_ == UnitState::ATTACKING; }

    float GetShadowOpacity() const { return shadow_opacity_; }
    int GetShadowOffset() const;
    int GetFactionId() const { return faction_id_; }
    const std::string& GetUnitId() const { return unit_id_; }

    UnitAssets* GetAssets() { return assets_; }
    AnimationPlayer& GetAnimationPlayer() { return animation_; }

    bool flip_x;

private:
    std::string unit_id_;
    int faction_id_;
    BoardPosition board_position_;
    BoardPosition target_position_;
    glm::vec2 screen_position_;
    glm::vec2 move_start_position_;
    glm::vec2 move_target_position_;

    UnitState state_;
    AnimationPlayer animation_;
    UnitAssets* assets_;

    float spawn_timer_;
    float shadow_opacity_;
    float move_progress_;
    float move_duration_;

    void UpdateSpawning(float dt);
    void UpdateMovement(float dt);
};

#endif
