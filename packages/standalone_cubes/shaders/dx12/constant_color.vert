// DirectX 12 HLSL Vertex Shader
// Position (Float3) + Color (UByte4Norm) vertex format

cbuffer Uniforms : register(b0)
{
    float4x4 u_modelViewProj;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), u_modelViewProj);
    output.color = input.color;
    return output;
}
