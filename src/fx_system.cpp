#include "fx_system.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

FxSystem::FxSystem() : assets_(nullptr), resolver_(nullptr) {}

void FxSystem::Initialize(AssetManager* assets, FxCompositionResolver* resolver) {
    assets_ = assets;
    resolver_ = resolver;
}

void FxSystem::SpawnUnitFx(const std::string& unit_id, int faction_id,
                            const std::string& fx_type, const glm::vec2& position) {
    if (!assets_ || !resolver_) return;

    FxComposition* comp = resolver_->ResolveUnitFx(unit_id, faction_id, fx_type);
    if (!comp) {
        std::cerr << "No FX composition found for: " << fx_type << std::endl;
        return;
    }

    float z_offset = 0.0f;
    for (const auto& sprite_name : comp->sprites) {
        FxSprite* fx = CreateFxSprite(sprite_name, position);
        if (fx) {
            fx->z_order = position.y + z_offset;
            z_offset += 0.1f;
        }
    }
}

void FxSystem::SpawnProjectile(const std::string& rsx_name, const glm::vec2& start,
                                const glm::vec2& target, float speed) {
    FxSprite* fx = CreateFxSprite(rsx_name, start);
    if (!fx) return;

    fx->source_to_target = true;
    fx->start_position = start;
    fx->target_position = target;

    float distance = glm::length(target - start);
    fx->travel_duration = distance / speed;
}

FxSprite* FxSystem::CreateFxSprite(const std::string& rsx_name, const glm::vec2& position) {
    FxAssets* fx_assets = assets_->LoadFx(rsx_name);
    if (!fx_assets) {
        std::cerr << "Failed to load FX assets: " << rsx_name << std::endl;
        return nullptr;
    }

    auto fx = std::make_unique<FxSprite>();
    fx->assets = fx_assets;
    fx->position = position;
    fx->start_position = position;
    fx->target_position = position;
    fx->elapsed_time = 0.0f;
    fx->travel_duration = 0.0f;
    fx->source_to_target = false;
    fx->impact_at_end = false;
    fx->finished = false;
    fx->z_order = position.y;
    fx->animation_name = rsx_name;

    fx->animation.SetAtlas(&fx_assets->spritesheet);

    auto anim_names = fx_assets->spritesheet.GetAnimationNames();
    if (!anim_names.empty()) {
        fx->animation.Play(anim_names[0], false);

        float frame_delay = resolver_->GetFrameDelay(rsx_name);
        fx->animation.SetFrameDelay(frame_delay);
    }

    FxSprite* ptr = fx.get();
    active_fx_.push_back(std::move(fx));
    return ptr;
}

void FxSystem::Update(float dt) {
    for (auto& fx : active_fx_) {
        if (fx->finished) continue;

        fx->elapsed_time += dt;
        fx->animation.Update(dt);

        if (fx->source_to_target && fx->travel_duration > 0.0f) {
            float t = std::min(1.0f, fx->elapsed_time / fx->travel_duration);
            fx->position = glm::mix(fx->start_position, fx->target_position, t);

            if (t >= 1.0f) {
                fx->finished = true;
            }
        } else if (fx->animation.IsFinished()) {
            fx->finished = true;
        }
    }

    active_fx_.erase(
        std::remove_if(active_fx_.begin(), active_fx_.end(),
            [](const std::unique_ptr<FxSprite>& fx) {
                return fx->finished;
            }),
        active_fx_.end());
}

void FxSystem::Render(SpriteBatch& batch) {
    for (const auto& fx : active_fx_) {
        if (fx->finished) continue;

        const AnimationFrame* frame = fx->animation.GetCurrentFrame();
        if (!frame) continue;

        SpriteDrawCall call;
        call.texture_id = fx->assets->spritesheet.GetTextureId();
        call.position = fx->position;
        call.size = glm::vec2(static_cast<float>(frame->width),
                              static_cast<float>(frame->height));

        float tex_width = static_cast<float>(fx->assets->spritesheet.GetWidth());
        float tex_height = static_cast<float>(fx->assets->spritesheet.GetHeight());

        call.src_rect = glm::vec4(
            static_cast<float>(frame->x) / tex_width,
            static_cast<float>(frame->y) / tex_height,
            static_cast<float>(frame->width) / tex_width,
            static_cast<float>(frame->height) / tex_height
        );

        call.color = glm::vec4(1.0f);
        call.rotation = 0.0f;
        call.z_order = fx->z_order;
        call.flip_x = false;

        batch.Draw(call);
    }
}

float FxSystem::GetMaxFxDuration() const {
    float max_duration = 0.0f;

    for (const auto& fx : active_fx_) {
        float duration = fx->travel_duration;
        const Animation* anim = fx->assets->spritesheet.GetAnimation(fx->animation_name);
        if (anim && !anim->frames.empty()) {
            float frame_delay = resolver_->GetFrameDelay(fx->animation_name);
            duration += frame_delay * static_cast<float>(anim->frames.size());
        }
        max_duration = std::max(max_duration, duration);
    }

    return max_duration;
}

void FxSystem::Clear() {
    active_fx_.clear();
}
