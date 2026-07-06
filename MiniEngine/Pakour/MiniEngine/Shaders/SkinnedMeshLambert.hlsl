// 스키닝 메시 Lambert 셰이더 (Directional 라이트 1개 + ambient). 텍스처 없음(단색 알베도).
// 런타임 컴파일: VSMain(vs_5_0) / PSMain(ps_5_0).
//
// 규약(StaticMeshLambert.hlsl과 동일):
//  - SimpleMath는 row-major 저장 + row-vector 관례 → cbuffer 를 row_major 로 선언하고
//    CPU에서 전치 없이 업로드. VS 에서 mul(vector, matrix)(벡터 왼쪽).
//  - mvp = world * view * proj (world 먼저).
//  - 법선은 skin → world 로만 변환(균등 스케일 가정).
//
// GPU 스키닝(선형 블렌드): 본 최종 행렬(inverseBindPose * global)을 b2 로 받아
// 정점을 4-본 가중 합 행렬로 변환한 뒤 기존 mvp/world 파이프를 태운다. (CLAUDE.md §9)

#define MAX_BONES 128

cbuffer PerObject : register(b0)
{
    row_major float4x4 mvp;
    row_major float4x4 world;
};

cbuffer PerFrame : register(b1)
{
    float3 lightDir;   // "빛이 나아가는" 방향(정규화). N·(-L) 사용.
    float  ambient;    // ambient 세기(0..1)
    float3 lightColor; // 라이트 컬러
    float  pad0;
    float3 albedo;     // 단색 디퓨즈 알베도
    float  pad1;
};

cbuffer Bones : register(b2)
{
    row_major float4x4 boneMatrices[MAX_BONES];
};

struct VSInput
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 uv          : TEXCOORD;
    uint4  boneIndices : BONEINDICES;
    float4 boneWeights : BONEWEIGHTS;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normalWS : NORMAL;
    float2 uv       : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    // 4-본 선형 블렌드 스키닝 행렬.
    float4x4 skin = input.boneWeights.x * boneMatrices[input.boneIndices.x]
                  + input.boneWeights.y * boneMatrices[input.boneIndices.y]
                  + input.boneWeights.z * boneMatrices[input.boneIndices.z]
                  + input.boneWeights.w * boneMatrices[input.boneIndices.w];

    const float4 posSkinned    = mul(float4(input.position, 1.0f), skin);
    const float3 normalSkinned = mul(float4(input.normal, 0.0f), skin).xyz;

    PSInput output;
    output.position = mul(posSkinned, mvp);
    // 법선을 월드 공간으로(회전/균등 스케일). w=0 으로 방향 벡터 변환.
    output.normalWS = mul(float4(normalSkinned, 0.0f), world).xyz;
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
