#include "pch.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Core/Graphics.h"

using namespace MiniEngine::Graphics;
namespace MiniEngine
{
    void SkeletalMeshComponent::SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh)
    {
        m_mesh = _mesh;
        m_playTime = 0.0f;
        // 페이드 상태도 리셋 — 이전 메시의 클립 인덱스/시간은 새 메시에 무의미.
        m_targetClip   = -1;
        m_targetTime   = 0.0f;
        m_fadeDuration = 0.0f;
        m_fadeElapsed  = 0.0f;
        RefreshBoneMatrices();
    }

    void SkeletalMeshComponent::PlayClip(int _clipIndex, float _fadeSec)
    {
        // 현재가 정지(-1)이거나 페이드 시간이 없으면 즉시 전환(기존 SetActiveClip 동작).
        if (_fadeSec <= 0.0f || m_activeClip < 0)
        {
            m_activeClip   = _clipIndex;
            m_playTime     = 0.0f;
            m_targetClip   = -1;
            m_targetTime   = 0.0f;
            m_fadeDuration = 0.0f;
            m_fadeElapsed  = 0.0f;
            RefreshBoneMatrices();
            return;
        }

        // 크로스페이드 시작(-1 대상이면 바인드 포즈로 페이드).
        // 페이드 중 재호출이면 현재 진행 포즈 기준으로 새 페이드를 시작하는 대신
        // 대상만 교체(단순화 — 시각적으로 충분히 부드러움).
        m_targetClip   = _clipIndex;
        m_targetTime   = 0.0f;
        m_fadeDuration = _fadeSec;
        m_fadeElapsed  = 0.0f;
    }

    void SkeletalMeshComponent::Tick(float _dt)
    {
        SceneComponent::Tick(_dt);

        if (!m_mesh)
            return;

        const bool playing = (m_activeClip >= 0
            && m_activeClip < static_cast<int>(m_mesh->GetClips().size()));
        if (!playing && !IsFading())
            return; // 정지 상태 — 바인드 포즈 유지(행렬은 이미 계산됨)

        m_playTime += _dt;

        if (IsFading())
        {
            m_targetTime  += _dt;
            m_fadeElapsed += _dt;
            if (m_fadeElapsed >= m_fadeDuration)
            {
                // 페이드 완료 — 대상을 현재로 승격.
                m_activeClip   = m_targetClip;
                m_playTime     = m_targetTime;
                m_targetClip   = -1;
                m_targetTime   = 0.0f;
                m_fadeDuration = 0.0f;
                m_fadeElapsed  = 0.0f;
            }
        }

        RefreshBoneMatrices();
    }

    void SkeletalMeshComponent::Render(Graphics::RenderContext& _context)
    {
        SceneComponent::Render(_context);

        if (!m_mesh || !m_mesh->HasGpuResources())
            return;

        ID3D11DeviceContext*& pContext = _context.m_context;

        Graphics::PerObjectCB perObject = {};
        perObject.world = GetWorldMatrix();
        perObject.mvp = perObject.world * _context.m_camView * _context.m_camProj; // model view projection 연산

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(pContext->Map(
            _context.m_perObjectCB, 0,
            D3D11_MAP_WRITE_DISCARD, 0,
            &mapped)))
        {
            memcpy(mapped.pData, &perObject, sizeof(perObject));
            pContext->Unmap(_context.m_perObjectCB, 0);
        }

        // TODO : 객체별 색상 받아오기
        _context.m_perFrame.albedo = Vector3(0.7f);

        if (SUCCEEDED(pContext->Map(
            _context.m_perFrameCB, 0,
            D3D11_MAP_WRITE_DISCARD, 0,
            &mapped)))
        {
            memcpy(mapped.pData, &_context.m_perFrame, sizeof(_context.m_perFrame));
            pContext->Unmap(_context.m_perFrameCB, 0);
        }

        // b2: 본 최종 행렬 업로드 (부족분은 identity 패딩 — WRITE_DISCARD 라 전체를 채운다).
        const std::vector<Matrix>& bones = GetBoneMatrices();
        if (SUCCEEDED(pContext->Map(
            _context.m_boneCB, 0,
            D3D11_MAP_WRITE_DISCARD, 0, 
            &mapped)))
        {
            BoneCB* boneCB = static_cast<BoneCB*>(mapped.pData);
            const size_t count = (bones.size() < static_cast<size_t>(MAX_BONES)) ? bones.size() : MAX_BONES;
            for (size_t i = 0; i < count; ++i)
                boneCB->boneMatrices[i] = bones[i];
            for (size_t i = count; i < static_cast<size_t>(MAX_BONES); ++i)
                boneCB->boneMatrices[i] = Matrix(); // identity
            pContext->Unmap(_context.m_boneCB, 0);
        }

        UINT stride = m_mesh->GetVertexStride();
        UINT offset = 0;
        ID3D11Buffer* vb = m_mesh->GetVertexBuffer();

        pContext->IASetInputLayout(_context.m_skinnedMeshInputLayout);
        pContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        pContext->IASetIndexBuffer(m_mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->VSSetShader(_context.m_skinnedMeshVS, nullptr, 0);
        pContext->VSSetConstantBuffers(0, 1, &_context.m_perObjectCB);
        pContext->VSSetConstantBuffers(2, 1, &_context.m_boneCB);
        pContext->PSSetShader(_context.m_skinnedMeshPS, nullptr, 0);
        pContext->PSSetConstantBuffers(1, 1, &_context.m_perFrameCB);
        pContext->DrawIndexed(m_mesh->GetIndexCount(), 0, 0);
    }

    void SkeletalMeshComponent::SamplePose(int _clipIndex, float _timeSec, LocalPoseTRS& _outPose) const
    {
        if (_clipIndex >= 0 && _clipIndex < static_cast<int>(m_mesh->GetClips().size()))
            m_mesh->GetClips()[_clipIndex].SampleTRS(_timeSec, m_mesh->GetSkeleton(), _outPose);
        else
            SampleBindPoseTRS(m_mesh->GetSkeleton(), _outPose); // 클립 없음 — 바인드 포즈
    }

    void SkeletalMeshComponent::RefreshBoneMatrices()
    {
        if (!m_mesh)
        {
            m_boneMatrices.clear();
            return;
        }

        const Skeleton& skeleton = m_mesh->GetSkeleton();

        SamplePose(m_activeClip, m_playTime, m_poseA);
        if (IsFading())
        {
            // 현재 → 대상 크로스페이드: TRS 가중 보간(pos/scale Lerp, rot Slerp). (§9)
            const float weight = m_fadeElapsed / m_fadeDuration;
            SamplePose(m_targetClip, m_targetTime, m_poseB);
            BlendPose(m_poseA, m_poseB, weight, m_poseA);
        }
        ComposePose(m_poseA, m_localPose);

        skeleton.ComputeBoneMatrices(m_localPose, m_boneMatrices);
    }
}



// 선택된 카메라로 스키닝 메시를 GPU 스키닝 + Lambert 로 그린다.
//void GameCore::DrawSkinnedMesh(MiniEngine::CameraComponent& _camera, MiniEngine::SkeletalMeshComponent& _meshComp)
//{
//    auto mesh = _meshComp.GetMesh();
//
//    // b0/b1 은 DrawMesh 와 동일 규약 (row-vector, 무전치).
//    const Matrix world = _meshComp.GetWorldMatrix();
//    const Matrix view = _camera.GetViewMatrix();
//    const Matrix proj = _camera.GetProjectionMatrix();
//
//    PerObjectCB perObject = {};
//    perObject.mvp = world * view * proj;
//    perObject.world = world;
//
//    D3D11_MAPPED_SUBRESOURCE mapped = {};
//    if (SUCCEEDED(m_context->Map(m_perObjectCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
//    {
//        memcpy(mapped.pData, &perObject, sizeof(perObject));
//        m_context->Unmap(m_perObjectCB.Get(), 0);
//    }
//
//    PerFrameCB perFrame = {};
//    Vector3 dir(-0.4f, -1.0f, -0.6f);
//    dir.Normalize();
//    perFrame.lightDir = dir;
//    perFrame.ambient = 0.15f;
//    perFrame.lightColor = Vector3(1.0f, 1.0f, 1.0f);
//    perFrame.albedo = Vector3(0.55f, 0.75f, 0.85f); // 큐브와 구분되는 한색 계열
//
//    if (SUCCEEDED(m_context->Map(m_perFrameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
//    {
//        memcpy(mapped.pData, &perFrame, sizeof(perFrame));
//        m_context->Unmap(m_perFrameCB.Get(), 0);
//    }
//
//    // b2: 본 최종 행렬 업로드 (부족분은 identity 패딩 — WRITE_DISCARD 라 전체를 채운다).
//    const std::vector<Matrix>& bones = _meshComp.GetBoneMatrices();
//    if (SUCCEEDED(m_context->Map(m_boneCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
//    {
//        BoneCB* boneCB = static_cast<BoneCB*>(mapped.pData);
//        const size_t count = (bones.size() < static_cast<size_t>(MAX_BONES)) ? bones.size() : MAX_BONES;
//        for (size_t i = 0; i < count; ++i)
//            boneCB->boneMatrices[i] = bones[i];
//        for (size_t i = count; i < static_cast<size_t>(MAX_BONES); ++i)
//            boneCB->boneMatrices[i] = Matrix(); // identity
//        m_context->Unmap(m_boneCB.Get(), 0);
//    }
//
//    UINT stride = mesh.lock()->GetVertexStride();
//    UINT offset = 0;
//    ID3D11Buffer* vb = mesh.lock()->GetVertexBuffer();
//    ID3D11Buffer* objectCB = m_perObjectCB.Get();
//    ID3D11Buffer* frameCB = m_perFrameCB.Get();
//    ID3D11Buffer* boneCB = m_boneCB.Get();
//
//    m_context->IASetInputLayout(m_skinnedInputLayout.Get());
//    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
//    m_context->IASetIndexBuffer(mesh.lock()->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
//    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    m_context->VSSetShader(m_skinnedVertexShader.Get(), nullptr, 0);
//    m_context->VSSetConstantBuffers(0, 1, &objectCB);
//    m_context->VSSetConstantBuffers(2, 1, &boneCB);
//    m_context->PSSetShader(m_skinnedPixelShader.Get(), nullptr, 0);
//    m_context->PSSetConstantBuffers(1, 1, &frameCB);
//    m_context->DrawIndexed(mesh.lock()->GetIndexCount(), 0, 0);
//}
