// DirectX 12 HLSL Pixel Shader (PS 5.0)
// Constant color shader for cube rendering

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Return the interpolated color from vertex shader
    return input.color;
}
