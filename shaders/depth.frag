#version 450

// Depth buffer fragment shader
// Packs depth into RGBA for reading in other passes
// Uses Duelyst's depth packing formula

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform DepthUniforms {
    float u_depthOffset;     // Artificial depth offset (CONFIG.DEPTH_OFFSET = 19.5)
    float u_depthModifier;   // 0 = facing screen, 1 = flat on ground
    float _pad1, _pad2;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in float v_depth;

layout(location = 0) out vec4 fragColor;

// Pack depth (0-1) into RGBA
// Duelyst formula from DepthFragment.glsl
vec4 packDepth(float depth) {
    const vec4 bitShift = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);
    const vec4 bitMask = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
    vec4 res = fract(depth * bitShift);
    res -= res.xxyz * bitMask;
    return res;
}

void main() {
    // Sample texture to check alpha (discard fully transparent pixels)
    float alpha = texture(u_texture, v_texCoord).a;
    if (alpha < 0.01) {
        discard;
    }

    // Pack and output depth
    fragColor = packDepth(v_depth);
}
