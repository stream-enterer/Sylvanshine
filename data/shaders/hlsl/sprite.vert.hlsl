struct VSInput {
    float2 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct VSOutput {
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.Position = float4(input.Position.x, input.Position.y, 0.0, 1.0);
    output.TexCoord = input.TexCoord;
    return output;
}
