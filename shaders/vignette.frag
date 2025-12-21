#version 450

// Vignette fragment shader
// Darkens corners of the screen
// Based on Duelyst's vignette effect

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform VignetteUniforms {
    float u_intensity;   // Vignette darkness (0 = none, 1 = full black)
    float u_radius;      // Vignette start radius (0.5 = half screen)
    float u_softness;    // Edge softness (higher = softer)
    float u_aspectRatio; // Screen aspect ratio for circular vignette
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(u_texture, v_texCoord);

    // Center UV coordinates
    vec2 uv = v_texCoord - 0.5;

    // Correct for aspect ratio
    uv.x *= u_aspectRatio;

    // Calculate distance from center
    float dist = length(uv);

    // Smooth vignette falloff
    float vignette = 1.0 - smoothstep(u_radius, u_radius + u_softness, dist);

    // Apply vignette
    vec3 result = color.rgb * mix(1.0, vignette, u_intensity);

    fragColor = vec4(result, color.a);
}
