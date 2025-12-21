#version 450

// Bloom accumulation fragment shader with temporal smoothing
// Blends current bloom with previous frame for smooth transitions

layout(set = 2, binding = 0) uniform sampler2D u_currentBloom;
layout(set = 2, binding = 1) uniform sampler2D u_previousBloom;

layout(set = 3, binding = 0) uniform BloomUniforms {
    float u_transition;   // Blend factor (default: 0.15 - favor current)
    float u_intensity;    // Bloom intensity multiplier (default: 2.5)
    float _pad1, _pad2;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 current = texture(u_currentBloom, v_texCoord);
    vec4 previous = texture(u_previousBloom, v_texCoord);

    // Temporal blend for smooth bloom transitions
    vec4 blended = mix(previous, current, u_transition);

    // Apply intensity
    fragColor = blended * u_intensity;
}
