#version 450

// Radial blur fragment shader
// Creates motion/zoom blur from center point
// Based on Duelyst's RadialBlurFragment.glsl

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform RadialBlurUniforms {
    vec2 u_center;       // Blur center in UV space (default: 0.5, 0.5)
    float u_strength;    // Blur strength (default: 0.0, max ~0.1)
    float u_samples;     // Number of samples (default: 8)
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec2 dir = v_texCoord - u_center;
    float dist = length(dir);

    // No blur at center, increasing toward edges
    float blurAmount = u_strength * dist;

    vec4 color = vec4(0.0);
    float totalWeight = 0.0;

    int numSamples = int(u_samples);
    for (int i = 0; i < numSamples; i++) {
        float t = float(i) / float(numSamples - 1) - 0.5;
        vec2 offset = dir * t * blurAmount;
        float weight = 1.0 - abs(t) * 0.5;  // Center samples weighted more

        color += texture(u_texture, v_texCoord + offset) * weight;
        totalWeight += weight;
    }

    fragColor = color / totalWeight;
}
