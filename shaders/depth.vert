#version 450

// Depth buffer vertex shader
// Transforms vertices and passes depth value

layout(location = 0) in vec2 a_position;   // NDC position
layout(location = 1) in vec2 a_texCoord;   // Texture coordinates
layout(location = 2) in float a_depth;     // Depth value (0-1, 0=front, 1=back)

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out float v_depth;

void main() {
    gl_Position = vec4(a_position, a_depth, 1.0);
    v_texCoord = a_texCoord;
    v_depth = a_depth;
}
