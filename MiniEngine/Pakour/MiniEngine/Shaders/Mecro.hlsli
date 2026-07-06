// include용 파일이므로 빌드에 참여안함 속성을 부여해야 한다.
// 기존엔 hlsl 파일은 include가 없어서 각각의 셰이더 파일마다 별도의 프로젝트라 간주함.
// include 기능이 생기고 나서 다음과 같이 공통적으로 사용하는 요소들을 선언해서 사용

struct Material
{
    float3 Ambient;
    float shininess;
    float3 Diffuse;
    float tmp1; // 셰이더 구조체에서는 16Byte의 배수로 크기를 맞춰줘야 한다. 이를 맞추기 위해서 CPU에서도 tmp1, tmp2 와 같이 더미 변수를 생성
    float3 Specular;
    float tmp2;
};

struct Light
{
    float3 Strength;
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
    float3 Position;
    float SpotPower;
};

struct VertexShaderInput
{
    float4 Position : POSITION; //Vertex 좌표
    float3 NormalVector : NORMAL; //NormalVector
    float2 TexturePosition : TEXCOORD;
};

struct PixelShaderInput
{
    float4 PositionProjection : SV_POSITION; //Screen상의 좌표
    float3 PositionWorld : POSITION; //실제 좌표
    float3 NormalVector : NORMAL; //NormalVector
    float2 TexturePosition : TEXCOORD;
};