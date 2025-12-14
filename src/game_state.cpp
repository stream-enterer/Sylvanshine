#include "game_state.hpp"

#include <algorithm>
#include <iostream>

GameState::GameState()
    : assets_(nullptr),
      resolver_(nullptr),
      selected_unit_(nullptr),
      phase_(GamePhase::SPAWNING),
      spawn_delay_timer_(0.0f),
      spawn_index_(0),
      background_texture_(0),
      middleground_texture_(0),
      foreground1_texture_(0),
      foreground2_texture_(0),
      attack_target_(nullptr),
      attack_timer_(0.0f) {}

bool GameState::Initialize(AssetManager* assets, FxCompositionResolver* resolver) {
    assets_ = assets;
    resolver_ = resolver;

    if (!board_renderer_.Initialize(assets)) {
        return false;
    }

    fx_system_.Initialize(assets, resolver);

    std::string map_path = assets->GetDataRoot() + "/maps/";
    background_texture_ = assets->LoadTexture(map_path + "battlemap0_background.png");
    middleground_texture_ = assets->LoadTexture(map_path + "battlemap0_middleground.png");
    foreground1_texture_ = assets->LoadTexture(map_path + "battlemap0_foreground_001.png");
    foreground2_texture_ = assets->LoadTexture(map_path + "battlemap0_foreground_002.png");

    SpawnUnits();

    return true;
}

void GameState::Reset() {
    units_.clear();
    selected_unit_ = nullptr;
    attack_target_ = nullptr;
    phase_ = GamePhase::SPAWNING;
    spawn_delay_timer_ = 0.0f;
    spawn_index_ = 0;
    fx_system_.Clear();
    board_renderer_.ClearHighlights();
    board_renderer_.ClearSelection();

    SpawnUnits();
}

void GameState::SpawnUnits() {
    auto player_unit = std::make_unique<Unit>();
    if (player_unit->Initialize(assets_, "f1_azuritelion", 1, {1, 2})) {
        player_unit->StartSpawn();
        units_.push_back(std::move(player_unit));
    }

    auto enemy_unit = std::make_unique<Unit>();
    if (enemy_unit->Initialize(assets_, "critter_2", 0, {7, 2})) {
        enemy_unit->StartSpawn();
        units_.push_back(std::move(enemy_unit));
    }

    for (auto& unit : units_) {
        fx_system_.SpawnUnitFx(unit->GetUnitId(), unit->GetFactionId(),
                               "UnitSpawnFX", unit->GetScreenPosition());
    }
}

void GameState::Update(float dt) {
    fx_system_.Update(dt);

    for (auto& unit : units_) {
        unit->Update(dt);
    }

    switch (phase_) {
        case GamePhase::SPAWNING:
            UpdateSpawning(dt);
            break;
        case GamePhase::MOVING:
            UpdateMoving(dt);
            break;
        case GamePhase::ATTACKING:
            UpdateAttacking(dt);
            break;
        default:
            break;
    }

    units_.erase(
        std::remove_if(units_.begin(), units_.end(),
            [](const std::unique_ptr<Unit>& u) {
                return u->GetState() == UnitState::DEAD;
            }),
        units_.end());
}

void GameState::UpdateSpawning(float dt) {
    (void)dt;

    bool all_spawned = true;
    for (const auto& unit : units_) {
        if (unit->IsSpawning()) {
            all_spawned = false;
            break;
        }
    }

    if (all_spawned && !fx_system_.HasActiveFx()) {
        phase_ = GamePhase::IDLE;
    }
}

void GameState::UpdateMoving(float dt) {
    (void)dt;

    if (!selected_unit_ || !selected_unit_->IsMoving()) {
        DeselectUnit();
        phase_ = GamePhase::IDLE;
    }
}

void GameState::UpdateAttacking(float dt) {
    attack_timer_ += dt;

    if (selected_unit_ && selected_unit_->IsAttacking()) {
        float progress = selected_unit_->GetAnimationPlayer().GetProgress();

        if (progress >= 0.5f && attack_target_ && attack_target_->IsAlive() &&
            attack_target_->GetState() != UnitState::HIT &&
            attack_target_->GetState() != UnitState::DYING) {

            glm::vec2 target_pos = attack_target_->GetScreenPosition();
            fx_system_.SpawnUnitFx(selected_unit_->GetUnitId(),
                                   selected_unit_->GetFactionId(),
                                   "UnitDamagedFX", target_pos);

            attack_target_->StartDeath();
        }
    }

    if (selected_unit_ && selected_unit_->IsIdle()) {
        attack_target_ = nullptr;
        DeselectUnit();
        phase_ = GamePhase::IDLE;
    }
}

void GameState::Render(SpriteBatch& batch) {
    RenderBackground(batch);

    for (auto& unit : units_) {
        if (!unit->IsAlive() && unit->GetState() != UnitState::DYING) continue;

        glm::vec2 pos = unit->GetScreenPosition();

        if (unit->GetShadowOpacity() > 0.0f) {
            SpriteDrawCall shadow_call;
            shadow_call.texture_id = assets_->GetShadowTexture();
            shadow_call.position = pos;
            shadow_call.size = glm::vec2(96, 48);
            shadow_call.src_rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            shadow_call.color = glm::vec4(1.0f, 1.0f, 1.0f, unit->GetShadowOpacity());
            shadow_call.rotation = 0.0f;
            shadow_call.z_order = -9999.0f;
            shadow_call.flip_x = false;
            batch.Draw(shadow_call);
        }
    }

    RenderUnits(batch);

    fx_system_.Render(batch);

    board_renderer_.RenderTileHighlights(batch);

    RenderForeground(batch);
}

void GameState::RenderBackground(SpriteBatch& batch) {
    SpriteDrawCall bg_call;
    bg_call.position = glm::vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    bg_call.size = glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT);
    bg_call.src_rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    bg_call.color = glm::vec4(1.0f);
    bg_call.rotation = 0.0f;
    bg_call.z_order = -10000.0f;
    bg_call.flip_x = false;

    if (background_texture_ != 0) {
        bg_call.texture_id = background_texture_;
        batch.Draw(bg_call);
    }

    if (middleground_texture_ != 0) {
        bg_call.texture_id = middleground_texture_;
        bg_call.z_order = -9000.0f;
        batch.Draw(bg_call);
    }
}

void GameState::RenderUnits(SpriteBatch& batch) {
    for (auto& unit : units_) {
        if (!unit->IsAlive() && unit->GetState() != UnitState::DYING) continue;

        const AnimationFrame* frame = unit->GetAnimationPlayer().GetCurrentFrame();
        if (!frame) continue;

        UnitAssets* unit_assets = unit->GetAssets();
        if (!unit_assets) continue;

        glm::vec2 ground_pos = unit->GetScreenPosition();
        float sprite_y = ground_pos.y - unit->GetShadowOffset();

        SpriteDrawCall call;
        call.texture_id = unit_assets->spritesheet.GetTextureId();
        call.position = glm::vec2(ground_pos.x, sprite_y);
        call.size = glm::vec2(static_cast<float>(frame->width),
                              static_cast<float>(frame->height));

        float tex_width = static_cast<float>(unit_assets->spritesheet.GetWidth());
        float tex_height = static_cast<float>(unit_assets->spritesheet.GetHeight());

        call.src_rect = glm::vec4(
            static_cast<float>(frame->x) / tex_width,
            static_cast<float>(frame->y) / tex_height,
            static_cast<float>(frame->width) / tex_width,
            static_cast<float>(frame->height) / tex_height
        );

        call.color = glm::vec4(1.0f);
        call.rotation = 0.0f;
        call.z_order = ground_pos.y;
        call.flip_x = unit->flip_x;

        batch.Draw(call);
    }
}

void GameState::RenderForeground(SpriteBatch& batch) {
    SpriteDrawCall fg_call;
    fg_call.position = glm::vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    fg_call.size = glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT);
    fg_call.src_rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    fg_call.color = glm::vec4(1.0f);
    fg_call.rotation = 0.0f;
    fg_call.flip_x = false;

    if (foreground1_texture_ != 0) {
        fg_call.texture_id = foreground1_texture_;
        fg_call.z_order = 10000.0f;
        batch.Draw(fg_call);
    }

    if (foreground2_texture_ != 0) {
        fg_call.texture_id = foreground2_texture_;
        fg_call.z_order = 10001.0f;
        batch.Draw(fg_call);
    }
}

void GameState::HandleClick(float screen_x, float screen_y) {
    if (phase_ == GamePhase::SPAWNING || phase_ == GamePhase::MOVING ||
        phase_ == GamePhase::ATTACKING) {
        return;
    }

    BoardPosition clicked_pos = BoardRenderer::ScreenToBoard(screen_x, screen_y);

    if (!BoardRenderer::IsValidPosition(clicked_pos)) {
        return;
    }

    Unit* clicked_unit = GetUnitAt(clicked_pos);

    if (phase_ == GamePhase::IDLE) {
        if (clicked_unit && clicked_unit->GetFactionId() == 1) {
            SelectUnit(clicked_unit);
        }
    } else if (phase_ == GamePhase::UNIT_SELECTED && selected_unit_) {
        if (clicked_unit == selected_unit_) {
            DeselectUnit();
            return;
        }

        if (clicked_unit && clicked_unit->GetFactionId() != 1) {
            AttackUnit(selected_unit_, clicked_unit);
            return;
        }

        TileHighlight highlight = TileHighlight::NONE;
        if (BoardRenderer::IsValidPosition(clicked_pos)) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    if (dx != 0 && dy != 0) continue;

                    BoardPosition check = {
                        selected_unit_->GetBoardPosition().x + dx,
                        selected_unit_->GetBoardPosition().y + dy
                    };

                    if (check == clicked_pos && !GetUnitAt(check)) {
                        highlight = TileHighlight::MOVE;
                        break;
                    }
                }
                if (highlight != TileHighlight::NONE) break;
            }
        }

        if (highlight == TileHighlight::MOVE) {
            MoveSelectedUnit(clicked_pos);
        }
    }
}

void GameState::HandleRightClick() {
    if (phase_ == GamePhase::UNIT_SELECTED) {
        DeselectUnit();
    }
}

Unit* GameState::GetUnitAt(BoardPosition pos) {
    for (auto& unit : units_) {
        if (unit->IsAlive() && unit->GetBoardPosition() == pos) {
            return unit.get();
        }
    }
    return nullptr;
}

void GameState::SelectUnit(Unit* unit) {
    selected_unit_ = unit;
    phase_ = GamePhase::UNIT_SELECTED;

    board_renderer_.ClearHighlights();
    board_renderer_.SetSelectedTile(unit->GetBoardPosition());

    HighlightValidMoves(unit);
}

void GameState::DeselectUnit() {
    selected_unit_ = nullptr;
    phase_ = GamePhase::IDLE;
    board_renderer_.ClearHighlights();
    board_renderer_.ClearSelection();
}

void GameState::HighlightValidMoves(Unit* unit) {
    BoardPosition pos = unit->GetBoardPosition();

    BoardPosition directions[] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    for (const auto& dir : directions) {
        BoardPosition target = {pos.x + dir.x, pos.y + dir.y};

        if (!BoardRenderer::IsValidPosition(target)) continue;

        Unit* occupant = GetUnitAt(target);
        if (!occupant) {
            board_renderer_.SetHighlight(target, TileHighlight::MOVE);
        } else if (occupant->GetFactionId() != unit->GetFactionId()) {
            board_renderer_.SetHighlight(target, TileHighlight::ATTACK);
        }
    }
}

void GameState::MoveSelectedUnit(BoardPosition target) {
    if (!selected_unit_) return;

    selected_unit_->StartMove(target);
    phase_ = GamePhase::MOVING;
    board_renderer_.ClearHighlights();
}

void GameState::AttackUnit(Unit* attacker, Unit* target) {
    if (!attacker || !target) return;

    if (target->GetBoardPosition().x < attacker->GetBoardPosition().x) {
        attacker->flip_x = true;
    } else {
        attacker->flip_x = false;
    }

    attacker->StartAttack();
    attack_target_ = target;
    attack_timer_ = 0.0f;
    phase_ = GamePhase::ATTACKING;

    board_renderer_.ClearHighlights();

    fx_system_.SpawnUnitFx(attacker->GetUnitId(), attacker->GetFactionId(),
                           "UnitAttackedFX", attacker->GetScreenPosition());

    std::cout << "Would play sound: attack" << std::endl;
}
