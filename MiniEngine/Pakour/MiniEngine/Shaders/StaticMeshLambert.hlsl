// 정적 메시 Lambert 셰이더 (Directional 라이트 1개 + ambient). 텍스처 없음(단색 알베도).
// 런타임 컴파일: VSMain(vs_5_0) / PSMain(ps_5_0).
//
// 규약(SceneColor.hlsl과 동일):
//  - SimpleMath는 row-major 저장 + row-vector 관례 → cbuffer 를 row_major 로 선언하고
//    CPU에서 전치 없이 업로드. VS 에서 mul(vector, matrix)(벡터 왼쪽).
//  - mvp = world * view * proj (world 먼저).
//  - 법선은 world 로만 변환(균등 스케일 가정). 비균등 스케일은 추후 역전치 처리로 확장.

cbuffer PerObject : register(b0)
{
    row_major float4x4 mvp;
    row_major float4x4 world;
};

cbuffer PerFrame : register(b1)
{
    float3 lightDir;   // 표면 → 광원 반대 방향이 아닌, "빛이 나아가는" 방향(정규화). N·(-L) 사용.
    float  ambient;    // ambient 세기(0..1)
    float3 lightColor; // 라이트 컬러
    float  pad0;
    float3 albedo;     // 단색 디퓨즈 알베도
    float  pad1;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normalWS : NORMAL;
    float2 uv       : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    // 법선을 월드 공간으로(회전/균등 스케일). w=0 으로 방향 벡터 변환.
    output.normalWS = mul(float4(input.normal, 0.0f), world).xyz;
    output.uv = input.uv;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 n = normalize(input.normalWS);
    float3 l = normalize(lightDir);         // 빛 진행 방향
    float  ndotl = saturate(dot(n, -l));    // 표면→광원 = -l
    float3 diffuse = albedo * lightColor * ndotl;
    float3 color = albedo * ambient + diffuse;
    return float4(color, 1.0f);
}
