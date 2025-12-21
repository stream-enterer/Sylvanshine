#version 450

// Per-sprite shadow fragment shader with progressive blur
// Uses UV 0-1 for the entire sprite (not atlas coordinates)
// This fixes the blur dilution issue from atlas sampling

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform ShadowUniforms {
    float u_opacity;              // External opacity control
    float u_intensity;            // Shadow darkness (Duelyst default: 0.15)
    float u_blurShiftModifier;    // Blur gradient steepness (default: 1.0)
    float u_blurIntensityModifier;// Blur amount (default: 3.0)
    vec2 u_size;                  // Sprite content size in pixels
    vec2 u_anchor;                // (anchorX * width, shadowOffset) in pixels
    float u_lightDistPctInv;      // Light distance attenuation
    float u_renderScale;          // Render scale for scale-independent blur
    float _pad1, _pad2;           // Alignment padding
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_position;  // Local sprite position in pixels

layout(location = 0) out vec4 fragColor;

// Sample with edge clamping - since we have the full sprite texture,
// clamping to 0-1 UV range gives correct edge sampling
float sampleAlpha(vec2 uv) {
    vec2 clampedUV = clamp(uv, vec2(0.0), vec2(1.0));
    return texture(u_texture, clampedUV).a;
}

void main() {
    // Distance from anchor (feet) - matches Duelyst exactly
    vec2 anchorDiff = v_position - u_anchor;
    float sizeRadius = length(u_size) * 0.5;

    // Blur by distance from anchor point
    float occluderDistPctX = min(abs(anchorDiff.x) / sizeRadius, 1.0);
    float occluderDistPctY = min(max(anchorDiff.y, 0.0) / u_size.x, 1.0);

    // Blur modifier: 0 at feet (sharp), increases toward head
    float occluderDistPctBlurModifier = pow(occluderDistPctY, u_blurShiftModifier);

    // Blur in texture coordinate space (UV 0-1 for per-sprite texture)
    // Divide by renderScale to get consistent screen-space blur at any scale:
    // - At scale 2: blur is half as many texels, but each texel is 2 screen pixels = same blur
    // - At scale 0.5: blur is twice as many texels, but each texel is 0.5 screen pixels = same blur
    float scaleCompensation = max(u_renderScale, 0.25);  // Clamp to avoid division issues at tiny scales
    float blurX = (u_blurIntensityModifier / u_size.x / scaleCompensation) * occluderDistPctBlurModifier;
    float blurY = (u_blurIntensityModifier / u_size.y / scaleCompensation) * occluderDistPctBlurModifier;

    // Intensity falloff - exact Duelyst exponents
    float intensityFadeX = pow(1.0 - occluderDistPctX, 1.25);
    float intensityFadeY = pow(1.0 - occluderDistPctY, 1.5);
    float intensity = intensityFadeX * intensityFadeY * u_intensity;

    // 7x7 box blur
    float alpha = 0.0;
    const float boxWeight = 1.0 / 49.0;

    for (int y = -3; y <= 3; y++) {
        for (int x = -3; x <= 3; x++) {
            vec2 offset = vec2(float(x) * blurX, float(y) * blurY);
            alpha += sampleAlpha(v_texCoord + offset) * boxWeight;
        }
    }

    // Final color with light attenuation
    fragColor = vec4(0.0, 0.0, 0.0, min(1.0, u_lightDistPctInv * alpha * intensity * u_opacity));
}
