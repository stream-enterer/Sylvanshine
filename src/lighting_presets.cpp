#include "lighting_presets.hpp"
#include "gpu_renderer.hpp"
#include <cmath>

const LightingPreset g_lighting_presets[] = {
    {"Dawn",         6.5f,  0.50f},
    {"Morning",      9.0f,  0.75f},
    {"Noon",        12.0f,  1.00f},
    {"Afternoon",   15.0f,  0.85f},
    {"Golden Hour", 17.0f,  0.70f},
    {"Dusk",        18.5f,  0.55f},
    {"Evening",     20.0f,  0.40f},
    {"Night",       23.0f,  0.30f},
    {"Pre-Dawn",     5.0f,  0.35f},
    {"Zenith",      12.0f,  1.10f},
};

int g_current_preset = 0;

Vec2 sun_position_from_time(float hour, int window_w, int window_h) {
    // Normalize hour to 0-24 range
    while (hour < 0.0f) hour += 24.0f;
    while (hour >= 24.0f) hour -= 24.0f;

    // Convert time to angle: 6am = right (0), 12pm = top (PI/2), 6pm = left (PI)
    float angle = ((hour - 6.0f) / 12.0f) * static_cast<float>(M_PI);

    // Radius of sun orbit (far from screen center for parallel rays)
    float orbit_radius = static_cast<float>(window_h) * 3.5f;

    // Sun position relative to screen center
    float sun_x = static_cast<float>(window_w) * 0.5f + std::cos(angle) * orbit_radius;
    float sun_y = static_cast<float>(window_h) * 0.5f - std::sin(angle) * orbit_radius;

    return {sun_x, sun_y};
}

void apply_lighting_preset(int preset_idx, const RenderConfig& config) {
    if (preset_idx < 0 || preset_idx >= LIGHTING_PRESET_COUNT) {
        return;
    }

    const auto& preset = g_lighting_presets[preset_idx];
    g_current_preset = preset_idx;

    Vec2 sun_pos = sun_position_from_time(preset.time_of_day, config.window_w, config.window_h);

    PointLight scene_light;
    scene_light.x = sun_pos.x;
    scene_light.y = sun_pos.y;
    scene_light.radius = SUN_RADIUS;
    scene_light.intensity = 1.0f;
    scene_light.r = 1.0f;
    scene_light.g = 1.0f;
    scene_light.b = 1.0f;
    scene_light.a = 1.0f;
    scene_light.casts_shadows = true;
    g_gpu.set_scene_light(scene_light);

    g_gpu.fx_config.shadow_intensity = preset.shadow_intensity;

    SDL_Log("=== Lighting Preset %d: %s (%.1fh) ===", preset_idx, preset.name, preset.time_of_day);
    SDL_Log("  Sun pos: (%.0f, %.0f)", sun_pos.x, sun_pos.y);
    SDL_Log("  Shadow intensity: %.2f", preset.shadow_intensity);
}
