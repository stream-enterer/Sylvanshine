#version 450

// Shadow vertex shader - passes local sprite position for progressive blur calculation
// Matches Duelyst's ShadowVertex.glsl coordinate passing (not the 45° cast, just the varying)

layout(location = 0) in vec2 a_position;   // NDC position
layout(location = 1) in vec2 a_texCoord;   // Texture coordinates
layout(location = 2) in vec2 a_localPos;   // Local sprite position in PIXELS (not 0-1)

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec2 v_position;  // Local position for blur calculation (matches Duelyst)

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_position = a_localPos;
}
