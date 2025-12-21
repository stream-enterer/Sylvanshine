#version 450

// Lighting vertex shader
// Transforms occluder vertices with light data for per-pixel lighting calculation
// Based on Duelyst's LightingVertex.glsl

layout(location = 0) in vec2 a_position;      // NDC position
layout(location = 1) in vec2 a_texCoord;      // Texture coordinates
layout(location = 2) in vec3 a_lightPos;      // Light position (x, y, z)
layout(location = 3) in vec4 a_lightColor;    // Light color (r, g, b, intensity)
layout(location = 4) in float a_lightRadius;  // Light radius

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec3 v_lightPos;
layout(location = 2) out vec4 v_lightColor;
layout(location = 3) out float v_lightRadius;
layout(location = 4) out vec2 v_fragPos;      // Fragment position for light calc

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_lightPos = a_lightPos;
    v_lightColor = a_lightColor;
    v_lightRadius = a_lightRadius;
    // Pass fragment position in screen space for distance calculation
    v_fragPos = a_position * 0.5 + 0.5;  // Convert from NDC to 0-1
}
