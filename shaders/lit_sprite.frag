#version 450

// Lit sprite fragment shader
// Composites sprite diffuse with accumulated light map
// Based on Duelyst's MultipliedLightingFragment.glsl

layout(set = 2, binding = 0) uniform sampler2D u_sprite;    // Sprite diffuse texture
layout(set = 2, binding = 1) uniform sampler2D u_lightMap;  // Accumulated light texture

layout(set = 3, binding = 0) uniform LitSpriteUniforms {
    float u_opacity;          // Overall opacity
    vec3 u_ambientColor;      // Ambient light color (added to light map)
    float u_lightMapScale;    // Light map UV scale (default: 1.0)
    float _pad1, _pad2, _pad3;
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 diffuse = texture(u_sprite, v_texCoord);

    // Sample light map (may be at different resolution)
    vec2 lightUV = v_texCoord * u_lightMapScale;
    vec4 light = texture(u_lightMap, lightUV);

    // Add ambient and multiply with diffuse
    vec3 totalLight = light.rgb + u_ambientColor;
    vec3 litColor = diffuse.rgb * totalLight;

    fragColor = vec4(litColor, diffuse.a * u_opacity);
}
