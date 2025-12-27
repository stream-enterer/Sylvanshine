#pragma once
#include "types.hpp"

// Sun radius - far enough for parallel rays (Duelyst used TILESIZE * 1000)
constexpr float SUN_RADIUS = 95000.0f;

// Lighting preset: simplified to time-of-day
struct LightingPreset {
    const char* name;
    float time_of_day;        // 0-24 hours
    float shadow_intensity;   // Shadow darkness multiplier
};

extern const LightingPreset g_lighting_presets[];
constexpr int LIGHTING_PRESET_COUNT = 10;

// Global state for current lighting
extern int g_current_preset;

// Calculate sun screen position from time of day
Vec2 sun_position_from_time(float hour, int window_w, int window_h);

// Apply a lighting preset
void apply_lighting_preset(int preset_idx, const RenderConfig& config);
