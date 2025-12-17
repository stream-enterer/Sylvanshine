Texture2D<float4> SpriteTexture : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

cbuffer Uniforms : register(b0, space3) {
    float u_opacity;
    float u_dissolve_time;
    float u_seed;
    float u_padding;
};

struct PSInput {
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0 {
    float4 color = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    color.a *= u_opacity;
    return color;
}
