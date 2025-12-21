#version 450

// SDF Shadow vertex shader
// Passes texture coordinates and sprite-local position for SDF raymarching

layout(location = 0) in vec2 a_position;    // NDC position
layout(location = 1) in vec2 a_texCoord;    // SDF atlas UV coordinates
layout(location = 2) in vec2 a_localPos;    // Local sprite position in pixels

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec2 v_localPos;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_localPos = a_localPos;
}
