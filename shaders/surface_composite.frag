#version 450

// Surface composite fragment shader
// Composites the rendered surface with bloom

layout(set = 2, binding = 0) uniform sampler2D u_surface;
layout(set = 2, binding = 1) uniform sampler2D u_bloom;

layout(set = 3, binding = 0) uniform CompositeUniforms {
    float u_bloomIntensity;    // Bloom brightness multiplier (default: 2.5)
    float u_bloomScale;        // Bloom texture scale (default: 0.5)
    float u_exposure;          // Exposure adjustment
    float u_gamma;             // Gamma correction (default: 2.2)
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 surface = texture(u_surface, v_texCoord);
    vec4 bloom = texture(u_bloom, v_texCoord);  // Bloom is already at correct UV

    // Additive bloom
    vec3 color = surface.rgb + bloom.rgb * u_bloomIntensity;

    // Simple tone mapping (Reinhard)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / u_gamma));

    fragColor = vec4(color, surface.a);
}
