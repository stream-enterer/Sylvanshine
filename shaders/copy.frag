#version 450

// Simple copy/blit fragment shader
// Copies texture to output (used for final composite to screen)

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform CopyUniforms {
    float u_opacity;
    float _pad1, _pad2, _pad3;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(u_texture, v_texCoord);
    color.a *= u_opacity;
    fragColor = color;
}
