#pragma once
#include "types.hpp"
#include "render_pass.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>

// Light types matching Duelyst's lighting system
enum class LightType {
    Point,      // Radial falloff
    Directional // Parallel rays (sun-like)
};

// Light source with full Duelyst parameters
struct Light {
    LightType type = LightType::Point;

    // Position (screen space for point, direction for directional)
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;  // Height above ground (affects shadow length)

    // Color and intensity
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float intensity = 1.0f;

    // Attenuation
    float radius = 285.0f;           // CONFIG.TILESIZE * 3
    float falloff_exponent = 2.0f;   // Quadratic falloff

    // Shadow casting
    bool casts_shadows = true;
    float shadow_intensity = 0.15f;

    // Animation
    float flicker_amount = 0.0f;     // 0 = no flicker
    float flicker_speed = 1.0f;
    float flicker_phase = 0.0f;

    // Computed values (updated per frame)
    float effective_intensity = 1.0f;

    Light() = default;

    // Calculate light contribution at a point
    float get_attenuation(float px, float py) const {
        if (type == LightType::Directional) {
            return intensity;
        }

        float dx = px - x;
        float dy = py - y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float dist_pct = std::pow(dist / radius, falloff_exponent);
        return std::max(0.0f, 1.0f - dist_pct) * effective_intensity;
    }

    // Update flicker animation
    void update(float dt) {
        if (flicker_amount > 0.0f) {
            flicker_phase += dt * flicker_speed;
            float flicker = std::sin(flicker_phase * 6.28318f) * 0.5f + 0.5f;
            effective_intensity = intensity * (1.0f - flicker_amount * flicker);
        } else {
            effective_intensity = intensity;
        }
    }
};

// Batch of lights for efficient GPU upload
struct LightBatch {
    static constexpr size_t MAX_LIGHTS = 32;

    struct GPULightData {
        float pos_x, pos_y, pos_z, radius;
        float r, g, b, intensity;
    };

    std::vector<GPULightData> data;

    void clear() { data.clear(); }

    void add(const Light& light) {
        if (data.size() >= MAX_LIGHTS) return;

        GPULightData ld;
        ld.pos_x = light.x;
        ld.pos_y = light.y;
        ld.pos_z = light.z;
        ld.radius = light.radius;
        ld.r = light.r;
        ld.g = light.g;
        ld.b = light.b;
        ld.intensity = light.effective_intensity;
        data.push_back(ld);
    }

    size_t count() const { return data.size(); }
    const GPULightData* gpu_data() const { return data.data(); }
    size_t gpu_data_size() const { return data.size() * sizeof(GPULightData); }
};

// Lighting manager - handles all scene lights
struct LightingManager {
    std::vector<Light> lights;
    Light ambient_light;  // Global ambient
    LightBatch batch;

    // Ambient color (Duelyst default: rgb(89, 89, 89))
    float ambient_r = 89.0f / 255.0f;
    float ambient_g = 89.0f / 255.0f;
    float ambient_b = 89.0f / 255.0f;

    LightingManager() {
        ambient_light.type = LightType::Directional;
        ambient_light.intensity = 1.0f;
    }

    // Add a light to the scene
    size_t add_light(const Light& light) {
        lights.push_back(light);
        return lights.size() - 1;
    }

    // Remove a light by index
    void remove_light(size_t idx) {
        if (idx < lights.size()) {
            lights.erase(lights.begin() + idx);
        }
    }

    // Clear all dynamic lights
    void clear_lights() {
        lights.clear();
    }

    // Update all lights (animation, etc.)
    void update(float dt) {
        for (auto& light : lights) {
            light.update(dt);
        }
    }

    // Prepare batch for rendering
    void prepare_batch() {
        batch.clear();
        for (const auto& light : lights) {
            batch.add(light);
        }
    }

    // Get lights affecting a region
    std::vector<const Light*> get_lights_in_region(float x, float y, float w, float h) const {
        std::vector<const Light*> result;

        for (const auto& light : lights) {
            // Check if light radius intersects region
            float closest_x = std::max(x, std::min(light.x, x + w));
            float closest_y = std::max(y, std::min(light.y, y + h));
            float dx = light.x - closest_x;
            float dy = light.y - closest_y;
            float dist_sq = dx * dx + dy * dy;

            if (dist_sq < light.radius * light.radius) {
                result.push_back(&light);
            }
        }

        return result;
    }

    // Get total light contribution at a point
    void get_light_at(float px, float py, float& out_r, float& out_g, float& out_b) const {
        out_r = ambient_r;
        out_g = ambient_g;
        out_b = ambient_b;

        for (const auto& light : lights) {
            float atten = light.get_attenuation(px, py);
            out_r += light.r * atten;
            out_g += light.g * atten;
            out_b += light.b * atten;
        }

        // Clamp to 0-1 range (HDR would skip this)
        out_r = std::min(out_r, 1.0f);
        out_g = std::min(out_g, 1.0f);
        out_b = std::min(out_b, 1.0f);
    }
};

// Per-sprite lighting accumulation
// Each sprite gets its own light FBO to accumulate light contributions
struct SpriteLightAccumulator {
    RenderPass* light_pass = nullptr;  // Borrowed from PassManager pool
    float sprite_x, sprite_y;          // Sprite screen position
    float sprite_w, sprite_h;          // Sprite dimensions

    void reset() {
        light_pass = nullptr;
    }

    bool is_valid() const {
        return light_pass != nullptr && light_pass->is_valid();
    }
};
