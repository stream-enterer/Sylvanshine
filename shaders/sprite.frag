#version 450

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform Uniforms {
    float u_opacity;
    float u_dissolve_time;
    float u_seed;
    float u_padding;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(u_texture, v_texCoord);
    color.a *= u_opacity;
    fragColor = color;
}
