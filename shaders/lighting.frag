#version 450

// Lighting fragment shader
// Calculates per-pixel light contribution for a single light
// Based on Duelyst's LightingFragment.glsl

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform LightingUniforms {
    vec2 u_screenSize;        // Screen dimensions for position calculation
    float u_falloffModifier;  // Light spread within radius (default: 1.0)
    float u_intensityModifier;// Light brightness multiplier (default: 1.0)
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec3 v_lightPos;
layout(location = 2) in vec4 v_lightColor;
layout(location = 3) in float v_lightRadius;
layout(location = 4) in vec2 v_fragPos;

layout(location = 0) out vec4 fragColor;

void main() {
    // Sample sprite alpha for masking
    float alpha = texture(u_texture, v_texCoord).a;
    if (alpha < 0.01) {
        discard;
    }

    // Calculate fragment position in screen space
    vec2 fragScreenPos = v_fragPos * u_screenSize;

    // Light distance calculation (Duelyst formula)
    vec2 lightDiff = v_lightPos.xy - fragScreenPos;
    float lightDist = length(lightDiff);
    float lightDistPct = pow(lightDist / v_lightRadius, 2.0);
    float lightDistPctInv = max(0.0, 1.0 - lightDistPct);

    // Apply falloff modifier
    lightDistPctInv = pow(lightDistPctInv, u_falloffModifier);

    // Calculate light contribution
    vec3 lightContrib = v_lightColor.rgb * v_lightColor.a * lightDistPctInv * u_intensityModifier;

    // Output light accumulation (will be added with other lights)
    fragColor = vec4(lightContrib * alpha, alpha);
}
