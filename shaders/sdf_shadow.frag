#version 450

// SDF Shadow fragment shader
// Uses signed distance field for soft shadow edges with penumbra gradient
// Supports synchronized dissolve effect with sprite

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
    float u_dissolveTime;         // Dissolve progress (0.0 = solid, 1.0 = fully dissolved)
    float u_dissolveSeed;         // Random seed for dissolve noise pattern
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_localPos;

layout(location = 0) out vec4 fragColor;

// Dissolve noise functions (from dissolve.frag)
const float DISSOLVE_FREQUENCY = 8.0;
const float DISSOLVE_AMPLITUDE = 1.0;
const float DISSOLVE_VIGNETTE_STRENGTH = 1.0;
const float DISSOLVE_EDGE_FALLOFF = 0.15;

float getHash(in vec2 coord) {
    return fract(sin(dot(coord, vec2(234.1235, 123.752))) * 34702.0);
}

float getNoise(in vec2 coord) {
    vec2 coordWhole = floor(coord);
    vec2 coordFraction = fract(coord);
    vec2 smoothFraction = coordFraction * coordFraction * (3.0 - 2.0 * coordFraction);
    float bl = getHash(coordWhole);
    float br = getHash(vec2(coordWhole.x + 1.0, coordWhole.y));
    float tl = getHash(vec2(coordWhole.x, coordWhole.y + 1.0));
    float tr = getHash(vec2(coordWhole.x + 1.0, coordWhole.y + 1.0));
    float b = mix(bl, br, smoothFraction.x);
    float t = mix(tl, tr, smoothFraction.x);
    return mix(b, t, smoothFraction.y);
}

float getNoiseFBM(in vec2 coord, in float time, in float frequency, in float amplitude) {
    float fractalSum = 0.0;
    time = time * 3.141592653589793 * 2.0 + 3.141592653589793;
    for (int i = 0; i < 2; i++) {
        vec2 offset = vec2(cos(time + frequency), sin(time + frequency)) * (frequency * 0.1);
        float noise = getNoise(coord * frequency + offset);
        fractalSum += noise * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return fractalSum;
}

// Compute dissolve alpha using normalized 0-1 UV space
// This ensures the dissolve pattern matches the sprite exactly
float getDissolveAlpha(vec2 normalizedUV, float dissolveTime, float seed) {
    if (dissolveTime <= 0.0) return 1.0;
    if (dissolveTime >= 1.0) return 0.0;

    // Vignette effect (dissolve from edges) - uses normalized 0-1 UV, centered at 0.5
    vec2 noiseCenterDiff = normalizedUV - vec2(0.5);
    float noiseCenterOffset = dissolveTime + 0.25;
    noiseCenterOffset = (noiseCenterOffset * noiseCenterOffset * noiseCenterOffset * noiseCenterOffset) + 0.25;
    float noiseCenterDist = length(noiseCenterDiff) * 5.0 * noiseCenterOffset + noiseCenterOffset;
    noiseCenterDist = noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist * noiseCenterDist;
    float noiseVignette = max(0.0, 1.0 - noiseCenterDist) * DISSOLVE_VIGNETTE_STRENGTH;

    // FBM noise - uses normalized UV for consistent pattern with sprite
    float noise = getNoiseFBM(vec2(seed) + normalizedUV, 0.0, DISSOLVE_FREQUENCY, max(0.0, DISSOLVE_AMPLITUDE + dissolveTime - noiseVignette));
    float noiseRangeMin = 0.99 - dissolveTime;
    float noiseRangeMax = 1.0 - dissolveTime;
    float noiseSmooth = 1.0 - smoothstep(noiseRangeMin, noiseRangeMax, noise);
    float noiseEdge = fract(1.0 - smoothstep(noiseRangeMin, noiseRangeMax + DISSOLVE_EDGE_FALLOFF, noise));

    return noiseSmooth + noiseEdge;
}

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

    // Apply dissolve effect - shadow dissolves in sync with sprite
    // Use normalized UV (0-1) computed from local position for perfect synchronization
    vec2 normalizedUV = v_localPos / u_spriteSize;
    float dissolveAlpha = getDissolveAlpha(normalizedUV, u_dissolveTime, u_dissolveSeed);

    // Combine all factors
    float finalAlpha = shadowAlpha * u_intensity * distanceFade * u_opacity * dissolveAlpha;

    fragColor = vec4(0.0, 0.0, 0.0, clamp(finalAlpha, 0.0, 1.0));
}
