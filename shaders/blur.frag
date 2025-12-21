#version 450

// Gaussian blur fragment shader (separable)
// Used for bloom blur pass

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform BlurUniforms {
    vec2 u_direction;    // Blur direction: (1/width, 0) for horizontal, (0, 1/height) for vertical
    float u_radius;      // Blur radius in pixels
    float _pad;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

// 9-tap Gaussian weights (sigma ≈ 3)
const float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = u_direction;
    vec4 result = texture(u_texture, v_texCoord) * weights[0];

    for (int i = 1; i < 5; i++) {
        vec2 offset = texelSize * float(i) * u_radius;
        result += texture(u_texture, v_texCoord + offset) * weights[i];
        result += texture(u_texture, v_texCoord - offset) * weights[i];
    }

    fragColor = result;
}
