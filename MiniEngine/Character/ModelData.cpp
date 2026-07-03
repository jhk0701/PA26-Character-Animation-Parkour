#include "pch.h"
#include "ModelData.h"

#include "Utility.h"

#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <float.h>
#include <math.h>

using namespace DirectX;

namespace
{
    // FBX SDK 매니저 싱글턴 (프로세스당 1개).
    FbxManager* g_FbxManager = nullptr;

    FbxManager* GetFbxManager()
    {
        if (g_FbxManager == nullptr)
        {
            g_FbxManager = FbxManager::Create();
            FbxIOSettings* ios = FbxIOSettings::Create(g_FbxManager, IOSROOT);
            g_FbxManager->SetIOSettings(ios);
        }
        return g_FbxManager;
    }

    // FBX는 행 벡터(row-vector): TransformPoint(M,v) = v * M (점 변환, 이동 포함).
    XMFLOAT3 TransformPoint(const FbxAMatrix& m, double x, double y, double z)
    {
        return XMFLOAT3(
            (float)(x * m.Get(0, 0) + y * m.Get(1, 0) + z * m.Get(2, 0) + m.Get(3, 0)),
            (float)(x * m.Get(0, 1) + y * m.Get(1, 1) + z * m.Get(2, 1) + m.Get(3, 1)),
            (float)(x * m.Get(0, 2) + y * m.Get(1, 2) + z * m.Get(2, 2) + m.Get(3, 2)));
    }
    XMFLOAT3 TransformPoint(const FbxAMatrix& m, const FbxVector4& v) { return TransformPoint(m, v[0], v[1], v[2]); }
    XMFLOAT3 TransformPoint(const FbxAMatrix& m, const XMFLOAT3& v) { return TransformPoint(m, v.x, v.y, v.z); }

    // 방향 변환(이동 제외, 정규화 안 함 - 가중합용).
    XMFLOAT3 TransformDirRaw(const FbxAMatrix& m, double x, double y, double z)
    {
        return XMFLOAT3(
            (float)(x * m.Get(0, 0) + y * m.Get(1, 0) + z * m.Get(2, 0)),
            (float)(x * m.Get(0, 1) + y * m.Get(1, 1) + z * m.Get(2, 1)),
            (float)(x * m.Get(0, 2) + y * m.Get(1, 2) + z * m.Get(2, 2)));
    }
    XMFLOAT3 TransformDirRaw(const FbxAMatrix& m, const FbxVector4& v) { return TransformDirRaw(m, v[0], v[1], v[2]); }
    XMFLOAT3 TransformDirRaw(const FbxAMatrix& m, const XMFLOAT3& v) { return TransformDirRaw(m, v.x, v.y, v.z); }

    XMFLOAT3 TransformDir(const FbxAMatrix& m, const FbxVector4& v)
    {
        const double x = v[0], y = v[1], z = v[2];
        XMFLOAT3 r(
            (float)(x * m.Get(0, 0) + y * m.Get(1, 0) + z * m.Get(2, 0)),
            (float)(x * m.Get(0, 1) + y * m.Get(1, 1) + z * m.Get(2, 1)),
            (float)(x * m.Get(0, 2) + y * m.Get(1, 2) + z * m.Get(2, 2)));
        XMStoreFloat3(&r, XMVector3Normalize(XMLoadFloat3(&r)));
        return r;
    }

    // 노드의 지오메트리 오프셋(피벗)까지 포함한 최종 월드 변환.
    FbxAMatrix GetNodeWorldTransform(FbxNode* node)
    {
        FbxAMatrix world = node->EvaluateGlobalTransform();
        FbxVector4 gT = node->GetGeometricTranslation(FbxNode::eSourcePivot);
        FbxVector4 gR = node->GetGeometricRotation(FbxNode::eSourcePivot);
        FbxVector4 gS = node->GetGeometricScaling(FbxNode::eSourcePivot);
        FbxAMatrix geom(gT, gR, gS);
        return world * geom;
    }

    // 하나의 FbxMesh를 정점/인덱스로 전개(월드 공간 베이크).
    void ExtractMesh(FbxNode* node, FbxMesh* mesh,
        std::vector<ModelData::Vertex>& outVerts,
        std::vector<uint32_t>& outIndices,
        XMFLOAT3& minPos, XMFLOAT3& maxPos)
    {
        if (mesh->GetElementNormalCount() == 0)
            mesh->GenerateNormals();

        const FbxAMatrix world = GetNodeWorldTransform(node);
        const FbxVector4* controlPoints = mesh->GetControlPoints();
        const int polyCount = mesh->GetPolygonCount();

        for (int pi = 0; pi < polyCount; ++pi)
        {
            // 삼각화된 상태를 가정 (Load에서 Triangulate 수행).
            const int polySize = mesh->GetPolygonSize(pi);
            if (polySize != 3)
                continue;

            for (int vi = 0; vi < 3; ++vi)
            {
                const int cpIdx = mesh->GetPolygonVertex(pi, vi);

                ModelData::Vertex vtx;
                vtx.pos = TransformPoint(world, controlPoints[cpIdx]);

                FbxVector4 n(0, 1, 0, 0);
                mesh->GetPolygonVertexNormal(pi, vi, n);
                vtx.normal = TransformDir(world, n);

                minPos.x = min(minPos.x, vtx.pos.x);
                minPos.y = min(minPos.y, vtx.pos.y);
                minPos.z = min(minPos.z, vtx.pos.z);
                maxPos.x = max(maxPos.x, vtx.pos.x);
                maxPos.y = max(maxPos.y, vtx.pos.y);
                maxPos.z = max(maxPos.z, vtx.pos.z);

                outIndices.push_back((uint32_t)outVerts.size());
                outVerts.push_back(vtx);
            }
        }
    }
}

struct Influence 
{ 
    int bone; 
    float nrmTime; // 애니메이션 시간 정규화값
    XMFLOAT3 localPos; 
    XMFLOAT3 localNrm;
};

struct MeshSkin
struct AnimationClip 
{
    
};

struct Mesh
{
    std::vector<XMFLOAT3> meshPos; // 버텍스 위치
    std::vector<XMFLOAT3> meshNrm; // 노멀
};

struct MeshSkin : public Mesh
{
    std::vector<FbxNode*> bones;                      
    std::vector<FbxNode*> animBones;                  
    std::vector<std::vector<Influence>> cpInfluence;
    std::vector<int> pvCP; // 폴리곤-정점 -> 컨트롤포인트
    uint32_t pvBase = 0; // 전역 스킨드 배열 시작 오프셋
     
    std::vector<FbxAMatrix> boneGlobal; // 본 트랜스폼
};

struct ModelData::Impl
{
    FbxScene* scene = nullptr;
    FbxAnimStack* animStack = nullptr;
    FbxTime animStart;
    double animDuration = 0.0;
    double time = 0.0;

    std::vector<MeshSkin> meshes;
    std::vector<ModelData::Vertex> skinned;
};

ModelData::ModelData() : m_impl(std::make_unique<Impl>()) {}
ModelData::~ModelData() = default;

const ModelData::Vertex* ModelData::SkinnedVertices() const
{
    return m_impl->skinned.empty() ? nullptr : m_impl->skinned.data();
}

static void SkinAtTime(ModelData::Impl& impl, double timeSec)
{
    FbxTime ft;
    ft.SetSecondDouble(impl.animStart.GetSecondDouble() + timeSec);

    for (MeshSkin& ms : impl.meshes)
    {
        const int boneCount = (int)ms.bones.size();
        ms.boneGlobal.resize(boneCount);
        for (int b = 0; b < boneCount; ++b)
        {
            FbxNode* src = (b < (int)ms.animBones.size() && ms.animBones[b]) ? ms.animBones[b] : ms.bones[b];
            ms.boneGlobal[b] = src->EvaluateGlobalTransform(ft);
        }

        const int cpCount = (int)ms.cpInfluence.size();
        for (int cp = 0; cp < cpCount; ++cp)
        {
            XMFLOAT3 p(0, 0, 0), n(0, 0, 0);
            for (const Influence& in : ms.cpInfluence[cp])
            {
                XMFLOAT3 pp, nn;
                if (in.bone < 0)
                {
                    pp = in.localPos;
                    nn = in.localNrm;
                }
                else
                {
                    pp = TransformPoint(ms.boneGlobal[in.bone], in.localPos);
                    nn = TransformDirRaw(ms.boneGlobal[in.bone], in.localNrm);
                }

                p.x += pp.x * in.nrmTime;
                p.y += pp.y * in.nrmTime;
                p.z += pp.z * in.nrmTime;

                n.x += nn.x * in.nrmTime;
                n.y += nn.y * in.nrmTime;
                n.z += nn.z * in.nrmTime;
            }
            ms.meshPos[cp] = p;
            XMStoreFloat3(&ms.meshNrm[cp], XMVector3Normalize(XMLoadFloat3(&n)));
        }

        const uint32_t pvCount = (uint32_t)ms.pvCP.size();
        for (uint32_t i = 0; i < pvCount; ++i)
        {
            const int cp = ms.pvCP[i];
            ModelData::Vertex& out = impl.skinned[ms.pvBase + i];
            const XMFLOAT3& p = ms.meshPos[cp];
            const XMFLOAT3& n = ms.meshNrm[cp];
            out.pos = XMFLOAT3(p.x, p.y, -p.z);
            out.normal = XMFLOAT3(n.x, n.y, -n.z);
        }
    }
}

bool ModelData::Load(const std::wstring& filePath)
{
    FbxManager* manager = GetFbxManager();

    FbxImporter* importer = FbxImporter::Create(manager, "");
    const std::string pathUtf8 = Utility::WideStringToUTF8(filePath);

    if (!importer->Initialize(pathUtf8.c_str(), -1, manager->GetIOSettings()))
    {
        Utility::Printf("[Mesh] Failed to init importer: %s\nErr: %s\n",
            pathUtf8.c_str(), importer->GetStatus().GetErrorString());
        importer->Destroy();
        return false;
    }

    FbxScene* scene = FbxScene::Create(manager, "Scene");
    if (!importer->Import(scene))
    {
        Utility::Printf("[Mesh] Failed to import scene: %s\n",
            importer->GetStatus().GetErrorString());
        importer->Destroy();
        scene->Destroy();
        return false;
    }
    importer->Destroy();

    // 단위만 미터로 변환(방향 불변). 축은 변환하지 않고 스킨 출력에서 Z만 반전(RH->LH).
    FbxSystemUnit::m.ConvertScene(scene);
    FbxGeometryConverter geomConv(manager);
    geomConv.Triangulate(scene, true);

    m_impl->scene = scene;   

    // 스킨 메시 수집. DFS
    uint32_t totalPV = 0;
    std::stack<FbxNode*> stack;
    if (FbxNode* root = scene->GetRootNode())
        for (int i = 0; i < root->GetChildCount(); ++i)
            stack.push(root->GetChild(i));

    while (!stack.empty())
    {
        FbxNode* node = stack.top();
        stack.pop();
        for (int i = 0; i < node->GetChildCount(); ++i)
            stack.push(node->GetChild(i));

        FbxMesh* mesh = node->GetMesh();
        
        if (!mesh)
            continue;

        // 일반 메시
        if (mesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
        {
            std::vector<Vertex> verts;
            std::vector<uint32_t> indices;
            XMFLOAT3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
            XMFLOAT3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            ExtractMesh(node, mesh, verts, indices, minPos, maxPos);

            for (int i = 0; i < node->GetChildCount(); ++i)
                stack.push(node->GetChild(i));

            continue;
        }

        if (mesh->GetElementNormalCount() == 0)
            mesh->GenerateNormals();

        const int cpCount = mesh->GetControlPointsCount();
        const FbxVector4* bindCP = mesh->GetControlPoints();

        MeshSkin ms;
        // 클러스터 -> 본 + 바인드 행렬(M, L^-1) + 원시 영향(cp,nrmTime).
        FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
        const int clusterCount = skin->GetClusterCount();
        ms.bones.resize(clusterCount);
        std::vector<FbxAMatrix> clM(clusterCount), clLinv(clusterCount);
        std::vector<std::vector<std::pair<int, float>>> rawInfl(cpCount);
        FbxAMatrix meshBind; bool meshBindSet = false;

        for (int c = 0; c < clusterCount; ++c)
        {
            FbxCluster* cl = skin->GetCluster(c);
            ms.bones[c] = cl->GetLink();
            FbxAMatrix M, L;
            cl->GetTransformMatrix(M);       // 바인드 시 메시 노드 글로벌
            cl->GetTransformLinkMatrix(L);   // 바인드 시 본 글로벌
            clM[c] = M;
            clLinv[c] = L.Inverse();
            if (!meshBindSet) { meshBind = M; meshBindSet = true; }

            const int* idx = cl->GetControlPointIndices();
            const double* w = cl->GetControlPointWeights();
            const int n = cl->GetControlPointIndicesCount();
            for (int k = 0; k < n; ++k)
                if (idx[k] >= 0 && idx[k] < cpCount)
                    rawInfl[idx[k]].push_back({ c, (float)w[k] });
        }

        // 폴리곤-정점 전개 + 컨트롤포인트별 바인드 노멀 누적.
        std::vector<XMFLOAT3> cpNrm(cpCount, XMFLOAT3(0, 0, 0));
        const int polyCount = mesh->GetPolygonCount();
        for (int pi = 0; pi < polyCount; ++pi)
        {
            if (mesh->GetPolygonSize(pi) != 3)
                continue;
            for (int vi = 0; vi < 3; ++vi)
            {
                const int cp = mesh->GetPolygonVertex(pi, vi);
                ms.pvCP.push_back(cp);
                FbxVector4 nrm(0, 1, 0, 0);
                mesh->GetPolygonVertexNormal(pi, vi, nrm);
                cpNrm[cp].x += (float)nrm[0];
                cpNrm[cp].y += (float)nrm[1];
                cpNrm[cp].z += (float)nrm[2];
            }
        }
        for (XMFLOAT3& n : cpNrm)
            XMStoreFloat3(&n, XMVector3Normalize(XMLoadFloat3(&n)));

        // 영향별 바인드-로컬 위치/노멀 미리 계산(곱 순서 모호성 회피, 검증된 중첩 변환 사용).
        ms.cpInfluence.resize(cpCount);
        for (int cp = 0; cp < cpCount; ++cp)
        {
            std::vector<std::pair<int, float>>& raw = rawInfl[cp];
            float sum = 0.0f;
            for (auto& e : raw) sum += e.second;

            if (raw.empty() || sum <= 0.0f)
            {
                // 정적 폴백: 월드 바인드 위치/노멀을 그대로 사용.
                XMFLOAT3 wp = TransformPoint(meshBind, bindCP[cp]);
                XMFLOAT3 wn = TransformDirRaw(meshBind, cpNrm[cp]);
                ms.cpInfluence[cp].push_back({ -1, 1.0f, wp, wn });
                continue;
            }

            for (auto& e : raw)
            {
                const int c = e.first;
                const float wgt = e.second / sum;
                // cp -> 월드바인드(M) -> 본 로컬(L^-1)  (중첩 TransformPoint = v*M*L^-1)
                XMFLOAT3 worldBind = TransformPoint(clM[c], bindCP[cp]);
                XMFLOAT3 localPos = TransformPoint(clLinv[c], worldBind);
                XMFLOAT3 worldBindN = TransformDirRaw(clM[c], cpNrm[cp]);
                XMFLOAT3 localNrm = TransformDirRaw(clLinv[c], worldBindN);
                ms.cpInfluence[cp].push_back({ c, wgt, localPos, localNrm });
            }
        }

        ms.meshPos.resize(cpCount);
        ms.meshNrm.resize(cpCount);
        ms.pvBase = totalPV;
        totalPV += (uint32_t)ms.pvCP.size();
        m_impl->meshes.push_back(std::move(ms));
    }

    if (totalPV == 0)
    {
        Utility::Printf("[Mesh] No skinned geometry in %ws\n", filePath.c_str());
        return false;
    }

    // 애니메이션 스택 선택
    {
        const int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
        const std::vector<FbxNode*>& bones = m_impl->meshes[0].bones;
        FbxAnimStack* chosen = nullptr;
        FbxTimeSpan chosenSpan;

        for (int si = 0; si < stackCount; ++si)
        {
            FbxAnimStack* st = scene->GetSrcObject<FbxAnimStack>(si);
            const int layerCount = st->GetMemberCount<FbxAnimLayer>();
            int curved = 0;
            for (int li = 0; li < layerCount && curved == 0; ++li)
            {
                FbxAnimLayer* layer = st->GetMember<FbxAnimLayer>(li);
                for (FbxNode* bn : bones)
                    if (bn && 
                        (bn->LclRotation.GetCurve(layer) 
                        || bn->LclTranslation.GetCurve(layer)
                        || bn->LclScaling.GetCurve(layer)))
                        ++curved;
            }

            if (curved > 0) 
            { 
                chosen = st; 
                chosenSpan = st->GetLocalTimeSpan();
                break; 
            }
        }

        if (!chosen && stackCount > 0)   // 폴백
        {
            chosen = scene->GetSrcObject<FbxAnimStack>(0);
            chosenSpan = chosen->GetLocalTimeSpan();
        }

        if (chosen)
        {
            scene->SetCurrentAnimationStack(chosen);
            scene->GetAnimationEvaluator()->Reset();
            m_impl->animStart = chosenSpan.GetStart();
            m_impl->animDuration = (chosenSpan.GetStop() - chosenSpan.GetStart()).GetSecondDouble();

            Utility::Printf("[Animation] anim stack '%s' dur=%.2fs\n",
                chosen->GetName(), (float)m_impl->animDuration);
        }
    }

    m_impl->skinned.resize(totalPV);
    m_vertexCount = totalPV;

    // 첫 프레임(t=0) 스킨 후 바운딩 스피어 계산(카메라 프레이밍용).
    m_impl->time = 0.0;
    SkinAtTime(*m_impl, 0.0);

    XMFLOAT3 mn(FLT_MAX, FLT_MAX, FLT_MAX), mx(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (const Vertex& v : m_impl->skinned)
    {
        mn.x = min(mn.x, v.pos.x); mn.y = min(mn.y, v.pos.y); mn.z = min(mn.z, v.pos.z);
        mx.x = max(mx.x, v.pos.x); mx.y = max(mx.y, v.pos.y); mx.z = max(mx.z, v.pos.z);
    }
    XMVECTOR vmin = XMLoadFloat3(&mn), vmax = XMLoadFloat3(&mx);
    XMVECTOR vc = XMVectorScale(XMVectorAdd(vmin, vmax), 0.5f);
    float radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(vmax, vc)));
    XMFLOAT3 c; XMStoreFloat3(&c, vc);
    m_boundingSphere = Math::BoundingSphere(Math::Vector3(c.x, c.y, c.z), Math::Scalar(radius));

    Utility::Printf("[Mesh] Loaded %ws : %u verts, %u meshes\n",
        filePath.c_str(), m_vertexCount, (uint32_t)m_impl->meshes.size());

    return true;
}

void ModelData::Update(float deltaT)
{
    if (m_vertexCount == 0)
        return;

    if (m_impl->animDuration > 0.0)
        m_impl->time = fmod(m_impl->time + (double)deltaT, m_impl->animDuration);
    else
        m_impl->time = 0.0;

    SkinAtTime(*m_impl, m_impl->time);
}

void ModelData::Shutdown()
{
    if (g_FbxManager)
    {
        g_FbxManager->Destroy();   // 하위 씬도 함께 해제됨.
        g_FbxManager = nullptr;
    }
}

void ModelData::SetAnim(const ModelData& animModel)
{
    if (!animModel.IsLoaded() || !animModel.m_impl->scene)
        return;

    FbxScene* animScene = animModel.m_impl->scene;

    if (FbxAnimStack* st = animScene->GetCurrentAnimationStack())
    {
        animScene->SetCurrentAnimationStack(st);
        animScene->GetAnimationEvaluator()->Reset();
    }

    // animModel 씬의 모든 노드를 이름 -> 노드로 매핑.
    std::unordered_map<std::string, FbxNode*> nameToNode;

    std::stack<FbxNode*> stk;
    if (FbxNode* root = animScene->GetRootNode())
        for (int i = 0; i < root->GetChildCount(); ++i)
            stk.push(root->GetChild(i));

    while (!stk.empty())
    {
        FbxNode* n = stk.top(); stk.pop();
        nameToNode.emplace(n->GetName(), n);
        for (int i = 0; i < n->GetChildCount(); ++i)
            stk.push(n->GetChild(i));
    }

    // 각 메시의 본을 이름으로 매칭.
    int matched = 0, total = 0;
    for (MeshSkin& ms : m_impl->meshes)
    {
        const int boneCount = (int)ms.bones.size();
        ms.animBones.assign(boneCount, nullptr);
        for (int b = 0; b < boneCount; ++b)
        {
            ++total;
            auto it = nameToNode.find(ms.bones[b]->GetName());
            if (it != nameToNode.end())
            {
                ms.animBones[b] = it->second;
                ++matched;
            }
        }
    }

    // 타이밍은 애님 소스에서 가져옴.
    m_impl->animStart = animModel.m_impl->animStart;
    m_impl->animDuration = animModel.m_impl->animDuration;
    m_impl->time = 0.0;
}
