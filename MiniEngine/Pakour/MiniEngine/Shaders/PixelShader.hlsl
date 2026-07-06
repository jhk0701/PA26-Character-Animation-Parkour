#include "Mecro.hlsli"
// 셰이더는 include가 없기 때문에 VertexShader에서 명시했던 것과 동일한 구조체를 선언해줌

// 프로젝트의 픽셀 셰이더 설정부분에서 해당 슬롯으로 텍스쳐를 넘겨줌
Texture2D Texture : register(t0); 
SamplerState Sampler : register(s0); 

cbuffer LightConstantBuffer : register(b0)
{
    float3 m_EyePosition;
    bool m_Texture;
    Material m_Material;
    Light m_Light;
}


float4 Point(PixelShaderInput _inputPixelShader)
{
    // Point Light
    //물체의 월드좌표로부터 빛까지의 거리를 계산하기 위한 Vector를 구한다.
    float3 toLight = m_Light.Position - _inputPixelShader.PositionWorld;
    float Distance = length(toLight);
    
    if (Distance > m_Light.FallOffEnd)
        return m_Texture ? float4(m_Material.Ambient, 1.0f) * Texture.Sample(Sampler, _inputPixelShader.TexturePosition) : float4(m_Material.Ambient, 1.0f);
    
    toLight = normalize(toLight);
    float LightTheta = max(dot(toLight, _inputPixelShader.NormalVector), 0.0f);
    float3 LightStrength = m_Light.Strength * LightTheta;

    //빛의 End 에서 부터의 거리를  빛의 End에서 부터의 Start로 Interpolation한뒤 빛의 세기에 적용한다.
    float DistanceLigthStrength = (m_Light.FallOffEnd - Distance) / (m_Light.FallOffEnd - m_Light.FallOffStart);
    //값이 0 보다 작으면 0으로 1보다 크면 1로 고정한다.
    DistanceLigthStrength = saturate(DistanceLigthStrength);
    LightStrength *= DistanceLigthStrength;
    
    // pixel에서 화면으로
    float3 toEye = normalize(m_EyePosition - _inputPixelShader.PositionWorld);
    float3 HalfVector = normalize(toEye + toLight);
    float HalfTheta = dot(HalfVector, _inputPixelShader.NormalVector);
    float3 Specular = m_Material.Specular * pow(max(HalfTheta, 0.0f), m_Material.shininess);

    float3 PhongColor = m_Material.Ambient + (m_Material.Diffuse + Specular) * LightStrength;

    return m_Texture ?
        float4(PhongColor, 1.0f) * Texture.Sample(Sampler, _inputPixelShader.TexturePosition) :
        float4(PhongColor, 1.0f);
}

float4 Spot(PixelShaderInput _inputPixelShader)
{ 
    // Point Light
    //물체의 월드좌표로부터 빛까지의 거리를 계산하기 위한 Vector를 구한다.
    float3 toLight = m_Light.Position - _inputPixelShader.PositionWorld;
    float Distance = length(toLight);
    
    if (Distance > m_Light.FallOffEnd)
        return m_Texture ? float4(m_Material.Ambient, 1.0f) * Texture.Sample(Sampler, _inputPixelShader.TexturePosition) : float4(m_Material.Ambient, 1.0f);
    
    toLight = normalize(toLight);
    float LightTheta = max(dot(toLight, _inputPixelShader.NormalVector), 0.0f);
    float3 LightStrength = m_Light.Strength * LightTheta;

    //빛의 End 에서 부터의 거리를  빛의 End에서 부터의 Start로 Interpolation한뒤 빛의 세기에 적용한다.
    float DistanceLigthStrength = (m_Light.FallOffEnd - Distance) / (m_Light.FallOffEnd - m_Light.FallOffStart);
    //값이 0 보다 작으면 0으로 1보다 크면 1로 고정한다.
    DistanceLigthStrength = saturate(DistanceLigthStrength);
    LightStrength *= DistanceLigthStrength;

    float SpotPower = pow(max(dot(-toLight, m_Light.Direction), 0.0f), m_Light.SpotPower);
    LightStrength *= SpotPower;
    
    // pixel에서 화면으로
    float3 toEye = normalize(m_EyePosition - _inputPixelShader.PositionWorld);
    float3 HalfVector = normalize(toEye + toLight);
    float HalfTheta = dot(HalfVector, _inputPixelShader.NormalVector);
    float3 Specular = m_Material.Specular * pow(max(HalfTheta, 0.0f), m_Material.shininess);
    
    float3 PhongColor = m_Material.Ambient + (m_Material.Diffuse + Specular) * LightStrength;

    return m_Texture ?
        float4(PhongColor, 1.0f) * Texture.Sample(Sampler, _inputPixelShader.TexturePosition) :
        float4(PhongColor, 1.0f);
}

float4 Directional(PixelShaderInput _inputPixelShader)
{
    // Directional Light
    float3 toLight = -m_Light.Direction;
    float LightTheta = max(dot(toLight, _inputPixelShader.NormalVector), 0.0f);
    float3 LightStrength = m_Light.Strength * LightTheta;

    // Pixel에서 화면으로
    float3 toEye = normalize(m_EyePosition - _inputPixelShader.PositionWorld);
    float3 HalfVector = normalize(toEye + toLight);
    float HalfTheta = dot(HalfVector, _inputPixelShader.NormalVector);
    float3 Specular = m_Material.Specular * pow(max(HalfTheta, 0.0f), m_Material.shininess);

    float3 PhongColor = m_Material.Ambient + (m_Material.Diffuse + Specular) * LightStrength;

    return m_Texture ?
        float4(PhongColor, 1.0f) * Texture.Sample(Sampler, _inputPixelShader.TexturePosition) :
        float4(PhongColor, 1.0f);
}

float4 main(PixelShaderInput _inputPixelShader) : SV_TARGET
{
    return Spot(_inputPixelShader);
}