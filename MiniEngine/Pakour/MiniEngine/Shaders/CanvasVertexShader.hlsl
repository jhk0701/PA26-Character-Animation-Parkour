#pragma once

struct VertexShaderInput
{
    float4 Position : POSITION;
    float3 Color : COLOR;
    float2 TexturePosition : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float3 Color : COLOR;
    float2 TexturePosition : TEXCOORD;
};

VertexShaderOutput main(VertexShaderInput _inputVertexShader)
{
    VertexShaderOutput outputVertexShader;
    
    outputVertexShader.Position = _inputVertexShader.Position;
    outputVertexShader.Color = _inputVertexShader.Color;
    outputVertexShader.TexturePosition = _inputVertexShader.TexturePosition;
    
    return outputVertexShader;
}