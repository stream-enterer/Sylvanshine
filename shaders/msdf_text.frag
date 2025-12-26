#version 450

layout(set = 2, binding = 0) uniform sampler2D u_msdfAtlas;

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec4 v_color;
layout(location = 0) out vec4 fragColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(u_msdfAtlas, v_texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDist = (0.5 - sd) * 4.0;  // pxrange=4
    float alpha = clamp(0.5 - screenPxDist, 0.0, 1.0);
    fragColor = vec4(v_color.rgb, v_color.a * alpha);
}
