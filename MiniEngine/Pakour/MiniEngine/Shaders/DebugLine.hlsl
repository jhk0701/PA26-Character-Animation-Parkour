// 디버그 라인 셰이더 (LINELIST). 조명 없이 정점 색을 그대로 출력한다.
// 런타임 컴파일: VSMain(vs_5_0) / PSMain(ps_5_0).
//
// 규약(StaticMeshLambert.hlsl과 동일):
//  - SimpleMath는 row-major 저장 + row-vector 관례 → cbuffer 를 row_major 로 선언하고
//    CPU에서 전치 없이 업로드. VS 에서 mul(vector, matrix)(벡터 왼쪽).
//  - 정점 위치는 이미 월드 공간이라 world 행렬 없이 viewProj 만 곱한다.
//  - color 는 DXGI_FORMAT_B8G8R8A8_UNORM 로 언팩된 float4(RGBA, 0..1).

cbuffer PerFrame : register(b0)
{
    row_major float4x4 viewProj;
};

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
    output.position = mul(float4(input.position, 1.0f), viewProj);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
