#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "asset_manager.hpp"
#include "board_renderer.hpp"
#include "fx_composition_resolver.hpp"
#include "fx_system.hpp"
#include "sprite_batch.hpp"
#include "types.hpp"
#include "unit.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

enum class GamePhase {
    SPAWNING,
    IDLE,
    UNIT_SELECTED,
    MOVING,
    ATTACKING,
    GAME_OVER
};

class GameState {
public:
    GameState();

    bool Initialize(AssetManager* assets, FxCompositionResolver* resolver);
    void Reset();

    void Update(float dt);
    void Render(SpriteBatch& batch);

    void HandleClick(float screen_x, float screen_y);
    void HandleRightClick();

    GamePhase GetPhase() const { return phase_; }

private:
    AssetManager* assets_;
    FxCompositionResolver* resolver_;
    BoardRenderer board_renderer_;
    FxSystem fx_system_;

    std::vector<std::unique_ptr<Unit>> units_;
    Unit* selected_unit_;
    GamePhase phase_;

    float spawn_delay_timer_;
    int spawn_index_;

    Unit* GetUnitAt(BoardPosition pos);
    void SelectUnit(Unit* unit);
    void DeselectUnit();
    void MoveSelectedUnit(BoardPosition target);
    void AttackUnit(Unit* attacker, Unit* target);

    void UpdateSpawning(float dt);
    void UpdateMoving(float dt);
    void UpdateAttacking(float dt);

    void HighlightValidMoves(Unit* unit);

    void SpawnUnits();
    void RenderBackground(SpriteBatch& batch);
    void RenderUnits(SpriteBatch& batch);
    void RenderForeground(SpriteBatch& batch);

    unsigned int background_texture_;
    unsigned int middleground_texture_;
    unsigned int foreground1_texture_;
    unsigned int foreground2_texture_;

    Unit* attack_target_;
    float attack_timer_;
};

#endif
