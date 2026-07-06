#pragma once

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float3 Color : COLOR;
    float2 TexturePosition : TEXCOORD;
};

float4 main(VertexShaderOutput _outputVertexShader) : SV_TARGET
{
    return Texture.Sample(Sampler, _outputVertexShader.TexturePosition);

}