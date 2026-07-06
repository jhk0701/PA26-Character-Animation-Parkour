// 최소 삼각형 셰이더 (위치 + 색). MVP/라이트/텍스처 없음.
// 런타임에 D3DCompileFromFile 로 VSMain(vs_5_0) / PSMain(ps_5_0) 각각 컴파일.

struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
