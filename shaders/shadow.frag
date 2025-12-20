#version 450

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform ShadowUniforms {
    float u_opacity;
    float u_blur_amount;  // blur spread in texels
    vec2 u_tex_size;      // texture dimensions for texel calculation
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    // Calculate blur offsets based on texture size
    vec2 texel = vec2(1.0 / u_tex_size.x, 1.0 / u_tex_size.y) * u_blur_amount;

    // 7x7 box blur on alpha channel
    float alpha = 0.0;
    const float boxWeight = 1.0 / 49.0;

    for (int y = -3; y <= 3; y++) {
        for (int x = -3; x <= 3; x++) {
            vec2 offset = vec2(float(x), float(y)) * texel;
            alpha += texture(u_texture, v_texCoord + offset).a * boxWeight;
        }
    }

    // Output black with blurred alpha
    fragColor = vec4(0.0, 0.0, 0.0, alpha * u_opacity);
}
