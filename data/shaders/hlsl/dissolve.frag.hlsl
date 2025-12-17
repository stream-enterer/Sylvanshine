Texture2D<float4> SpriteTexture : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

cbuffer Uniforms : register(b0, space3) {
    float u_opacity;
    float u_time;
    float u_seed;
    float u_padding;
};

struct PSInput {
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

static const float u_frequency = 8.0;
static const float u_amplitude = 1.0;
static const float u_vignetteStrength = 1.0;
static const float u_edgeFalloff = 0.15;

float getHash(float2 coord) {
    return frac(sin(dot(coord, float2(234.1235, 123.752))) * 34702.0);
}

float getNoise(float2 coord) {
    float2 coordWhole = floor(coord);
    float2 coordFraction = frac(coord);
    float2 smoothFraction = coordFraction * coordFraction * (3.0 - 2.0 * coordFraction);
    
    float bl = getHash(coordWhole);
    float br = getHash(float2(coordWhole.x + 1.0, coordWhole.y));
    float tl = getHash(float2(coordWhole.x, coordWhole.y + 1.0));
    float tr = getHash(float2(coordWhole.x + 1.0, coordWhole.y + 1.0));
    
    float b = lerp(bl, br, smoothFraction.x);
    float t = lerp(tl, tr, smoothFraction.x);
    return lerp(b, t, smoothFraction.y);
}

float getNoiseFBM(float2 coord, float time, float frequency, float amplitude) {
    float fractalSum = 0.0;
    float PI = 3.14159265359;
    time = time * PI * 2.0 + PI;
    
    [unroll]
    for (int i = 0; i < 2; i++) {
        float2 offset = float2(cos(time + frequency), sin(time + frequency)) * (frequency * 0.1);
        float noise = getNoise(coord * frequency + offset);
        fractalSum += noise * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return fractalSum;
}

float4 main(PSInput input) : SV_Target0 {
    float4 color = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    
    float2 noiseCenterDiff = input.TexCoord - float2(0.5, 0.5);
    float noiseCenterOffset = u_time + 0.25;
    noiseCenterOffset = (noiseCenterOffset * noiseCenterOffset * noiseCenterOffset * noiseCenterOffset) + 0.25;
    float noiseCenterDist = length(noiseCenterDiff) * 5.0 * noiseCenterOffset + noiseCenterOffset;
    noiseCenterDist = noiseCenterDist * noiseCenterDist * noiseCenterDist * 
                      noiseCenterDist * noiseCenterDist * noiseCenterDist;
    float noiseVignette = max(0.0, 1.0 - noiseCenterDist) * u_vignetteStrength;
    
    float2 seedOffset = float2(u_seed, u_seed * 1.5);
    float noise = getNoiseFBM(seedOffset + input.TexCoord, 0.0, u_frequency, 
                              max(0.0, u_amplitude + u_time - noiseVignette));
    
    float noiseRangeMin = 0.99 - u_time;
    float noiseRangeMax = 1.0 - u_time;
    float noiseSmooth = 1.0 - smoothstep(noiseRangeMin, noiseRangeMax, noise);
    float noiseEdge = frac(1.0 - smoothstep(noiseRangeMin, noiseRangeMax + u_edgeFalloff, noise));
    
    float3 finalRgb = max(color.rgb, ceil(noiseEdge).xxx);
    float finalAlpha = color.a * (noiseSmooth + noiseEdge) * u_opacity;
    
    return float4(finalRgb, finalAlpha);
}
