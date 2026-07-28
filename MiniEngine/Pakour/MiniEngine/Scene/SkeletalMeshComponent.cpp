#include "pch.h"
#include "Scene/SkeletalMeshComponent.h"
#include "Core/Graphics.h"
#include "Animation/Animator.h"
#include "Asset/IK.h"
#include "Core/Log.h"

using namespace MiniEngine::Graphics;

namespace MiniEngine
{
    SkeletalMeshComponent::SkeletalMeshComponent() { }
    void SkeletalMeshComponent::SetMesh(const std::shared_ptr<SkinnedMesh>& _mesh)
    {
        m_mesh = _mesh;
        
        // 새로 변경하며 기존 것은 레퍼런스 카운트 0으로 변할 것
        m_anim = std::make_shared<Animator>(std::dynamic_pointer_cast<SkeletalMeshComponent>(shared_from_this()));
        m_anim->ResetForMesh();
    }

    void SkeletalMeshComponent::Tick(float _dt)
    {
        SceneComponent::Tick(_dt);

        if (!m_mesh || !m_anim)
            return;

        m_anim->Update(_dt);
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

        _context.m_perFrame.albedo = m_color;

        if (SUCCEEDED(pContext->Map(
            _context.m_perFrameCB, 0,
            D3D11_MAP_WRITE_DISCARD, 0,
            &mapped)))
        {
            memcpy(mapped.pData, &_context.m_perFrame, sizeof(_context.m_perFrame));
            pContext->Unmap(_context.m_perFrameCB, 0);
        }

        // b2: 본 최종 행렬 업로드 (부족분은 identity 패딩 — WRITE_DISCARD 라 전체를 채운다).
        if (SUCCEEDED(pContext->Map(
            _context.m_boneCB, 0,
            D3D11_MAP_WRITE_DISCARD, 0, 
            &mapped)))
        {
            BoneCB* boneCB = static_cast<BoneCB*>(mapped.pData);
            const size_t count = (m_boneMatrices.size() < static_cast<size_t>(MAX_BONES)) ? m_boneMatrices.size() : MAX_BONES;

            for (size_t i = 0; i < count; ++i)
                boneCB->boneMatrices[i] = m_boneMatrices[i];

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

    RootMotionDelta SkeletalMeshComponent::ConsumeRootMotionDelta() 
    {
        return m_anim->ConsumeRootMotionDelta();
    }

    bool SkeletalMeshComponent::IsRootMotionEnabled() const
    {
        return m_anim->GetIsEnableRootMotion();
    }


    void SkeletalMeshComponent::SetIKGoalWorld(TwoBoneIKBinding _boneBinding, const Vector3& _worldPos,
                                               const Quaternion& _worldRot, float _posAlpha, float _rotAlpha)
    {
        const Matrix WORLD = GetWorldMatrix();
        const Matrix INV_WORLD = WORLD.Invert();

        IKGoal goal;
        goal.position = Vector3::Transform(_worldPos, INV_WORLD);
        goal.positionAlpha = _posAlpha;
        goal.rotationAlpha = _rotAlpha;

        if (_rotAlpha > 0.0f) 
        {
            // 스케일이 비균등일 시 경고
            const Vector3& SCALE = localTransform.scale;
            if (std::fabs(SCALE.x - SCALE.y) > 1e-4f || std::fabs(SCALE.y - SCALE.z) > 1e-4f)
                MG_LOG_INFO("[SkeletalComponent] non-uniform scale. IK rotation goal will be skewed");

            Matrix rotWorld = Matrix::CreateFromQuaternion(_worldRot) * INV_WORLD;
            rotWorld.Translation(Vector3(0.0f));

            Vector3 sc, tr;
            Quaternion rot;

            if (rotWorld.Decompose(sc, rot, tr))
                goal.rotation = rot;
            else
                goal.rotation = _worldRot; // 월드 회전 그대로
        }

        m_anim->SetIKGoal(_boneBinding, goal);
    }


    void SkeletalMeshComponent::SetIKGoalWorld(TwoBoneIKBinding _boneBinding, const Vector3& _worldPos, float _posAlpha)
    {
        SetIKGoalWorld(_boneBinding, _worldPos, Quaternion(0.0f, 0.0f, 0.0f, 1.0f), _posAlpha, 0.0f);
    }

    void SkeletalMeshComponent::SetIKPoleTargetWorld(TwoBoneIKBinding _boneBinding, const Vector3& _worldPole)
    {
        const IKGoal* CUR = m_anim->GetIKGoal(_boneBinding.end);
        if (!CUR)
            return;

        IKGoal goal = *CUR;
        goal.bUsePoleTarget = true;
        goal.poleTarget = Vector3::Transform(_worldPole, GetWorldMatrix().Invert());
        m_anim->SetIKGoal(_boneBinding, goal);
    }

    void SkeletalMeshComponent::ClearIKGoal(HumanoidBone _end) { m_anim->ClearIKGoal(_end); }

    void SkeletalMeshComponent::ClearAllIKGoal() { m_anim->ClearAllIKGoals(); }

    const IKGoal* SkeletalMeshComponent::GetIKGoal(HumanoidBone _end) const { return m_anim->GetIKGoal(_end); }

    void SkeletalMeshComponent::SetIKPelvisOffsetWorld(const Vector3& _worldOffset)
    {
        const Matrix INV_WORLD = GetWorldMatrix().Invert();
        m_anim->SetIKPelvisOffset(Vector3::TransformNormal(_worldOffset, INV_WORLD));
    }

    bool SkeletalMeshComponent::GetBoneWorldMatrix(int _boneIdx, Matrix& _out) const
    {
        const std::vector<Matrix>& global = m_anim->GetGlobalPose();
        if (_boneIdx < 0 || _boneIdx >= static_cast<int>(global.size())) return false;

        _out = global[static_cast<size_t>(_boneIdx)] * GetWorldMatrix();
        return true;
    }

    void SkeletalMeshComponent::SolveIK()
    {
        m_anim->SolveIKAndRefresh(m_mesh.get());
    }

}