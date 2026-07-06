#include "Mecro.hlsli"

// 입력받을 버텍스 정보에 대한 명시
// 파이프라인 순서상 가장 먼저 실행될 셰이더
// 셰이더는 각각의 스테이지 별로 독립적으로 동작함
// 단지 결과물을 리소스 영역에 넘겨주는 것으로 공유

// cpu에서 넘겨준 행렬을 vertex shader 단에서 b0(버퍼 0번 슬롯)을 통해 받음
cbuffer MVPConstantBuffer : register(b0)
{
    matrix m_Model;
    matrix m_NormalModel;
    matrix m_View;
    matrix m_Projection;
}

PixelShaderInput main(VertexShaderInput _inputVertexShader)
{
    PixelShaderInput inputPixelShader;
    
    // cpu에서 한 변환행렬 연산을 적용
    inputPixelShader.PositionProjection = mul(_inputVertexShader.Position, m_Model);
    inputPixelShader.PositionWorld = inputPixelShader.PositionProjection.xyz; //모델만 반영된 월드 좌표의 xyz 만 별도로 저장한다.
    inputPixelShader.PositionProjection = mul(inputPixelShader.PositionProjection, m_View);
    inputPixelShader.PositionProjection = mul(inputPixelShader.PositionProjection, m_Projection);

    float4 normalPosition = float4(_inputVertexShader.NormalVector, 1.0f);
    inputPixelShader.NormalVector = mul(normalPosition, m_NormalModel); //NormalPosition Model 적용
    inputPixelShader.NormalVector = normalize(inputPixelShader.NormalVector);
    inputPixelShader.TexturePosition = _inputVertexShader.TexturePosition;
    
    return inputPixelShader;
}