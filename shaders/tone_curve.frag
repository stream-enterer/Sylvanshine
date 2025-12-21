#version 450

// Tone curve fragment shader
// Applies color grading via RGB curves
// Based on Duelyst's ToneCurveFragment.glsl

layout(set = 2, binding = 0) uniform sampler2D u_texture;
layout(set = 2, binding = 1) uniform sampler2D u_toneCurveLUT;  // 256x1 lookup texture

layout(set = 3, binding = 0) uniform ToneCurveUniforms {
    float u_intensity;   // Blend factor (0 = original, 1 = full effect)
    float u_contrast;    // Contrast adjustment
    float u_brightness;  // Brightness adjustment
    float u_saturation;  // Saturation adjustment
};

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;

// Convert RGB to HSL
vec3 rgb2hsl(vec3 c) {
    float maxC = max(max(c.r, c.g), c.b);
    float minC = min(min(c.r, c.g), c.b);
    float l = (maxC + minC) / 2.0;

    if (maxC == minC) {
        return vec3(0.0, 0.0, l);
    }

    float d = maxC - minC;
    float s = l > 0.5 ? d / (2.0 - maxC - minC) : d / (maxC + minC);

    float h;
    if (maxC == c.r) {
        h = (c.g - c.b) / d + (c.g < c.b ? 6.0 : 0.0);
    } else if (maxC == c.g) {
        h = (c.b - c.r) / d + 2.0;
    } else {
        h = (c.r - c.g) / d + 4.0;
    }
    h /= 6.0;

    return vec3(h, s, l);
}

// Convert HSL to RGB
float hue2rgb(float p, float q, float t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
    return p;
}

vec3 hsl2rgb(vec3 c) {
    if (c.y == 0.0) {
        return vec3(c.z);
    }

    float q = c.z < 0.5 ? c.z * (1.0 + c.y) : c.z + c.y - c.z * c.y;
    float p = 2.0 * c.z - q;

    return vec3(
        hue2rgb(p, q, c.x + 1.0/3.0),
        hue2rgb(p, q, c.x),
        hue2rgb(p, q, c.x - 1.0/3.0)
    );
}

void main() {
    vec4 color = texture(u_texture, v_texCoord);

    // Apply brightness
    vec3 adjusted = color.rgb + vec3(u_brightness);

    // Apply contrast
    adjusted = (adjusted - 0.5) * (1.0 + u_contrast) + 0.5;

    // Apply saturation
    vec3 hsl = rgb2hsl(adjusted);
    hsl.y *= u_saturation;
    adjusted = hsl2rgb(hsl);

    // Apply tone curve LUT if available (optional)
    // adjusted.r = texture(u_toneCurveLUT, vec2(adjusted.r, 0.5)).r;
    // adjusted.g = texture(u_toneCurveLUT, vec2(adjusted.g, 0.5)).g;
    // adjusted.b = texture(u_toneCurveLUT, vec2(adjusted.b, 0.5)).b;

    // Blend with original
    vec3 result = mix(color.rgb, adjusted, u_intensity);

    fragColor = vec4(clamp(result, 0.0, 1.0), color.a);
}
