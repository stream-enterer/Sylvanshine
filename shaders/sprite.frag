#version 450

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform Uniforms {
    float u_opacity;
    float u_dissolve_time;
    float u_seed;
    float u_padding;
    // Tint color (RGBA) - multiplied with texture color
    float u_tint_r;
    float u_tint_g;
    float u_tint_b;
    float u_tint_a;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(u_texture, v_texCoord);
    // Apply tint color (multiply RGB, multiply alpha with both tint.a and opacity)
    vec4 tint = vec4(u_tint_r, u_tint_g, u_tint_b, u_tint_a);
    color.rgb *= tint.rgb;
    color.a *= u_opacity * tint.a;
    fragColor = color;
}
