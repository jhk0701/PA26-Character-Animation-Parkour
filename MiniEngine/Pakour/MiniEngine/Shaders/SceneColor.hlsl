// 씬 컬러 셰이더 (위치+색 + MVP 변환). 라이트/텍스처 없음(Lambert는 3단계).
// 런타임 컴파일: VSMain(vs_5_0) / PSMain(ps_5_0).
//
// 정석 포인트 #3: SimpleMath는 row-major 저장 + row-vector 관례이므로
//  - cbuffer 를 row_major 로 선언하고 CPU에서 전치 없이 업로드
//  - VS 에서 mul(vector, matrix) (벡터가 왼쪽)
// mvp = world * view * proj (world 먼저).

cbuffer PerObject : register(b0)
{
    row_major float4x4 mvp;
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
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
