#include "unit.hpp"

#include "board_renderer.hpp"

#include <cmath>

Unit::Unit()
    : faction_id_(0),
      state_(UnitState::SPAWNING),
      assets_(nullptr),
      spawn_timer_(0.0f),
      shadow_opacity_(0.0f),
      move_progress_(0.0f),
      move_duration_(0.0f),
      flip_x(false) {
    board_position_ = {0, 0};
    target_position_ = {0, 0};
}

bool Unit::Initialize(AssetManager* assets, const std::string& unit_id,
                       int faction_id, BoardPosition position) {
    unit_id_ = unit_id;
    faction_id_ = faction_id;
    board_position_ = position;
    target_position_ = position;

    assets_ = assets->LoadUnit(unit_id);
    if (!assets_) {
        return false;
    }

    animation_.SetAtlas(&assets_->spritesheet);
    screen_position_ = BoardRenderer::BoardToScreen(position);

    if (position.x > BOARD_CENTER_X) {
        flip_x = true;
    }

    return true;
}

void Unit::Update(float dt) {
    switch (state_) {
        case UnitState::SPAWNING:
            UpdateSpawning(dt);
            break;
        case UnitState::MOVING:
            UpdateMovement(dt);
            break;
        default:
            animation_.Update(dt);
            break;
    }

    if (state_ == UnitState::ATTACKING && animation_.IsFinished()) {
        StartIdle();
    }

    if (state_ == UnitState::HIT && animation_.IsFinished()) {
        StartIdle();
    }

    if (state_ == UnitState::DYING && animation_.IsFinished()) {
        state_ = UnitState::DEAD;
    }
}

void Unit::StartSpawn() {
    state_ = UnitState::SPAWNING;
    spawn_timer_ = 0.0f;
    shadow_opacity_ = 0.0f;
    animation_.Play("breathing", true);
}

void Unit::StartIdle() {
    state_ = UnitState::IDLE;
    animation_.Play("breathing", true);
}

void Unit::StartMove(BoardPosition target) {
    if (!BoardRenderer::IsValidPosition(target)) return;

    state_ = UnitState::MOVING;
    target_position_ = target;
    move_start_position_ = screen_position_;
    move_target_position_ = BoardRenderer::BoardToScreen(target);
    move_progress_ = 0.0f;

    float distance = glm::length(move_target_position_ - move_start_position_);
    move_duration_ = distance / 400.0f;

    if (target.x < board_position_.x) {
        flip_x = true;
    } else if (target.x > board_position_.x) {
        flip_x = false;
    }

    animation_.Play("run", true);
}

void Unit::StartAttack() {
    state_ = UnitState::ATTACKING;
    animation_.Play("attack", false);
}

void Unit::StartHit() {
    state_ = UnitState::HIT;
    animation_.Play("hit", false);
}

void Unit::StartDeath() {
    state_ = UnitState::DYING;
    animation_.Play("death", false);
}

int Unit::GetShadowOffset() const {
    if (assets_) {
        return assets_->shadow_offset;
    }
    return DEFAULT_SHADOW_OFFSET;
}

void Unit::UpdateSpawning(float dt) {
    spawn_timer_ += dt;

    float fade_progress = spawn_timer_ / SHADOW_FADE_DURATION;
    shadow_opacity_ = std::min(1.0f, fade_progress) * (SHADOW_OPACITY / 255.0f);

    animation_.Update(dt);

    if (spawn_timer_ >= SHADOW_FADE_DURATION) {
        state_ = UnitState::IDLE;
        shadow_opacity_ = SHADOW_OPACITY / 255.0f;
    }
}

void Unit::UpdateMovement(float dt) {
    move_progress_ += dt / move_duration_;

    float t = move_progress_;
    t = t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.0f) / 2;
    t = std::min(1.0f, std::max(0.0f, t));

    screen_position_ = glm::mix(move_start_position_, move_target_position_, t);

    animation_.Update(dt);

    if (move_progress_ >= 1.0f) {
        screen_position_ = move_target_position_;
        board_position_ = target_position_;
        StartIdle();
    }
}
