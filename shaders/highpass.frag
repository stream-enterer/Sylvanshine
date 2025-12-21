#version 450

// Highpass fragment shader for bloom extraction
// Extracts pixels above brightness threshold

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform HighpassUniforms {
    float u_threshold;   // Brightness threshold (default: 0.6)
    float u_softness;    // Soft threshold transition (default: 0.1)
    float _pad1, _pad2;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(u_texture, v_texCoord);

    // Calculate luminance
    float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    // Soft threshold
    float brightness = smoothstep(u_threshold - u_softness, u_threshold + u_softness, luminance);

    // Output bright pixels only
    fragColor = vec4(color.rgb * brightness, color.a * brightness);
}
