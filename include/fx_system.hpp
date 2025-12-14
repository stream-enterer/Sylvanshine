#ifndef FX_SYSTEM_HPP
#define FX_SYSTEM_HPP

#include "animation_player.hpp"
#include "asset_manager.hpp"
#include "fx_composition_resolver.hpp"
#include "sprite_batch.hpp"
#include "types.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

struct FxSprite {
    AnimationPlayer animation;
    FxAssets* assets;
    glm::vec2 position;
    glm::vec2 start_position;
    glm::vec2 target_position;
    float elapsed_time;
    float travel_duration;
    bool source_to_target;
    bool impact_at_end;
    bool finished;
    float z_order;
    std::string animation_name;
};

class FxSystem {
public:
    FxSystem();

    void Initialize(AssetManager* assets, FxCompositionResolver* resolver);

    void SpawnUnitFx(const std::string& unit_id, int faction_id,
                     const std::string& fx_type, const glm::vec2& position);

    void SpawnProjectile(const std::string& rsx_name, const glm::vec2& start,
                        const glm::vec2& target, float speed);

    void Update(float dt);
    void Render(SpriteBatch& batch);

    bool HasActiveFx() const { return !active_fx_.empty(); }
    float GetMaxFxDuration() const;
    void Clear();

private:
    AssetManager* assets_;
    FxCompositionResolver* resolver_;
    std::vector<std::unique_ptr<FxSprite>> active_fx_;

    FxSprite* CreateFxSprite(const std::string& rsx_name, const glm::vec2& position);
};

#endif
