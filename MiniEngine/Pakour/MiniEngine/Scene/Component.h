#pragma once
#include <memory>

namespace MiniEngine
{
    namespace Graphics { struct RenderContext; }
    
    class Actor;
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void OnAttach() {}
        virtual void FixedTick(float _dt) {}
        virtual void Tick(float _dt) {}
        virtual void LateTick(float _dt) {}
        virtual void Render(Graphics::RenderContext& _context) {}
    
        // 컴포넌트 초기화 시점에서 사용할 것
        // actor의 beginplay 시점에 컴포넌트들을 정렬하는데 사용
        // tick 순서에 영향 
        // 컴포넌트 정렬 기준은 오름차순
        void SetSortOrder(int _o) { m_sortOrder = _o; }

        // 컴포넌트 정렬 기준은 오름차순
        const int8_t GetSortOrder() const { return m_sortOrder; }

        std::weak_ptr<Actor> owner;
    private:
        int8_t m_sortOrder{ 0 }; 
    };
}

// Order 설정 관계 : m_sortOrder 값이 작으면 먼저 호출됨 (오름차순임)
// 원래는 SortOrder 없이 하는게 가장 좋지만, 달리 다른 해결방안이 생각나지 않아 이렇게 작성

/*
Tick : 게임로직 처리 시점

SkeletalMeshComponent : 애니메이터를 갖고 있음. 루트모션이 있는 경우, 값을 보존해 줌
*/

/*
LateTick : 게임로직 처리 이후, 적용될 사항들 처리
CharacterControllerComponent (CCT) : move, syncTransform 호출됨 -> 루트모션을 소비해서 위치를 보정함 (1 프레임 오차 발생 방지)

IK 계열 컴포넌트
    - CCT 컴포넌트로 애니메이션, 캐릭터 트랜스폼 적용이 끝난 후에 IK를 적용해야 정상적으로 작동될 것 -> 그래서 아예 LateUpdate로 옮김

SkeletalMeshComponent : 설정된 IK를 적용시키기 위해서 LateUpdate에서 Solver 호출
*/