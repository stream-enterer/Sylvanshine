#version 450

// SDF Shadow fragment shader
// Uses signed distance field for soft shadow edges with penumbra gradient

layout(set = 2, binding = 0) uniform sampler2D u_sdfTexture;

layout(set = 3, binding = 0) uniform SDFShadowUniforms {
    float u_opacity;              // External opacity control
    float u_intensity;            // Shadow darkness (default: 0.15)
    float u_penumbraScale;        // Penumbra softness multiplier (default: 1.0)
    float u_sdfMaxDist;           // SDF max distance for decoding (default: 32.0)

    vec2 u_spriteSize;            // Sprite dimensions in pixels
    vec2 u_anchor;                // Anchor point in pixels (shadow pivot)

    vec2 u_lightDir;              // Normalized 2D direction toward light (unused now)
    float u_lightDistance;        // Distance to light (affects intensity falloff)
    float u_lightIntensity;       // Light contribution to shadow

    float u_maxRaymarch;          // Unused (kept for struct compatibility)
    float u_raymarchSteps;        // Unused (kept for struct compatibility)
    vec2 _pad;                    // Alignment padding
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_localPos;

layout(location = 0) out vec4 fragColor;

void main() {
    // Sample SDF at current position (grayscale texture, R channel)
    float encoded = texture(u_sdfTexture, v_texCoord).r;

    // Decode: encoded is 0-1, maps to -maxDist to +maxDist
    float sdf = encoded * 2.0 * u_sdfMaxDist - u_sdfMaxDist;

    // SDF < 0 = inside silhouette (shadow), SDF > 0 = outside
    // We want smooth falloff at the edge
    // Note: No early discard - let shadowAlpha smoothstep handle the falloff
    // Early discard was cutting off the penumbra region at sprite edges

    // Convert SDF to alpha:
    // Inside (sdf < 0): full shadow
    // Edge (sdf ~ 0): partial shadow
    // Outside (sdf > 0): fading shadow (penumbra)
    float penumbraWidth = u_penumbraScale * 8.0;  // Width of soft edge in SDF units

    // Smooth falloff from inside to outside
    // At sdf = -penumbraWidth: alpha = 1.0 (full shadow)
    // At sdf = 0: alpha = 0.5 (edge)
    // At sdf = +penumbraWidth: alpha = 0.0 (no shadow)
    float shadowAlpha = 1.0 - smoothstep(-penumbraWidth * 0.5, penumbraWidth, sdf);

    // Distance-based fade from anchor (Duelyst-style)
    // Shadow is darker/sharper near feet (anchor), lighter/softer toward head
    // In texture space: feet at Y=spriteSize.y, head at Y=0
    // So distance from feet = anchor.y - localPos.y
    float distFromFeet = max(u_anchor.y - v_localPos.y, 0.0);
    float fadeRange = u_spriteSize.y * 0.8;
    float distanceFade = 1.0 - smoothstep(0.0, fadeRange, distFromFeet);

    // Side fade (less shadow at horizontal edges)
    float sideDistance = abs(v_localPos.x - u_anchor.x) / (u_spriteSize.x * 0.5);
    float sideFade = 1.0 - smoothstep(0.5, 1.2, sideDistance);

    // Combine
    float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity;

    fragColor = vec4(0.0, 0.0, 0.0, clamp(finalAlpha, 0.0, 1.0));
}
