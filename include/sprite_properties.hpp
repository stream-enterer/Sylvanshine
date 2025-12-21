#pragma once

// Sprite rendering properties matching Duelyst's BaseSprite
// These properties control how a sprite interacts with the rendering pipeline
struct SpriteProperties {
    // Depth properties
    float depth_offset = 0.0f;       // Artificial depth offset (CONFIG.DEPTH_OFFSET = 19.5)
    float depth_modifier = 0.0f;     // 0 = facing screen (upright), 1 = flat on ground

    // Shadow properties
    bool casts_shadows = true;       // Whether this sprite casts shadows
    float shadow_offset = 0.0f;      // Y offset for shadow anchor point
    float shadow_intensity = 0.15f;  // Shadow darkness (Duelyst default)

    // Occlusion properties
    bool occludes = true;            // Whether sprite blocks things behind it
    bool occlude_self = false;       // Whether to occlude own pixels (for special FX)

    // Lighting properties
    bool receives_lighting = true;   // Whether affected by dynamic lights
    bool is_lit = true;             // Whether to use the lit sprite shader
    float ambient_mult = 1.0f;       // Multiplier for ambient light

    // Bloom properties
    bool contributes_to_bloom = true;
    float bloom_threshold = 0.6f;    // Override for per-sprite bloom threshold

    // Z-order override
    float z_order_offset = 0.0f;     // Manual Z adjustment within layer

    // Animation flags
    bool needs_depth_draw = true;    // Whether to draw in depth pass
    bool needs_shadow_draw = true;   // Whether to draw shadow
    bool needs_light_draw = true;    // Whether to draw in light pass

    // Default constructor with Duelyst defaults
    SpriteProperties() = default;

    // Factory methods for common configurations
    static SpriteProperties unit() {
        SpriteProperties p;
        p.casts_shadows = true;
        p.occludes = true;
        p.receives_lighting = true;
        return p;
    }

    static SpriteProperties fx() {
        SpriteProperties p;
        p.casts_shadows = false;
        p.occludes = false;
        p.receives_lighting = false;
        p.contributes_to_bloom = true;
        return p;
    }

    static SpriteProperties ground_fx() {
        SpriteProperties p;
        p.casts_shadows = false;
        p.occludes = false;
        p.receives_lighting = true;
        p.depth_modifier = 1.0f;  // Flat on ground
        return p;
    }

    static SpriteProperties tile() {
        SpriteProperties p;
        p.casts_shadows = false;
        p.occludes = true;
        p.receives_lighting = true;
        p.depth_modifier = 1.0f;  // Flat on ground
        return p;
    }

    static SpriteProperties ui() {
        SpriteProperties p;
        p.casts_shadows = false;
        p.occludes = false;
        p.receives_lighting = false;
        p.contributes_to_bloom = false;
        p.needs_depth_draw = false;
        p.needs_shadow_draw = false;
        p.needs_light_draw = false;
        return p;
    }
};

// Composite rendering component that combines sprite with effects
// Based on Duelyst's CompositeComponent pattern
struct CompositeSprite {
    SpriteProperties properties;

    // Child sprite layers (for complex units with multiple parts)
    // Each child can have its own properties and offsets

    // Per-frame computed values
    float computed_depth = 0.0f;
    float computed_z_order = 0.0f;
    bool shadow_pass_dirty = true;
    bool light_pass_dirty = true;

    void update_computed_values(float screen_y, float board_y) {
        // Compute depth from screen position and modifiers
        computed_depth = screen_y + properties.depth_offset;

        // Apply depth modifier for flat sprites
        if (properties.depth_modifier > 0.0f) {
            // Lerp between upright depth and flat depth
            float flat_depth = board_y;
            computed_depth = computed_depth * (1.0f - properties.depth_modifier)
                           + flat_depth * properties.depth_modifier;
        }

        // Z-order is depth plus any manual offset
        computed_z_order = computed_depth + properties.z_order_offset;
    }

    void mark_dirty() {
        shadow_pass_dirty = true;
        light_pass_dirty = true;
    }
};
