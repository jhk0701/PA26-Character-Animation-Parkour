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
        // 헷갈리기 쉬우므로 액터에서 사용하는 것이 바람직
        void SetSortOrder(int _o) { m_sortOrder = _o; }; 
        // 우선순위가 높다 -> 먼저 tick에서 호출
        const uint8_t GetSortOrder() const { return m_sortOrder; }

        std::weak_ptr<Actor> owner;
    private:
        uint8_t m_sortOrder{ 0 }; 
    };
}

// 추가 : Order 설정 관계
// 원래는 SortOrder 없이 하는게 가장 좋지만, 달리 다른 해결방안이 생각나지 않아 이렇게 작성

/*
Update 필수 순서

SkeletalMeshComponent : 애니메이터를 갖고 있음. 루트모션이 있는 경우, 값을 보존해 줌
CharacterControllerComponent (CCT) : move, syncTransform 호출됨 -> 루트모션을 소비해서 위치를 보정함 (1 프레임 오차 발생 방지)
*/

/*
LateUpdate 필수 순서

IK 계열 컴포넌트
    - CCT 컴포넌트로 애니메이션, 캐릭터 트랜스폼 적용이 끝난 후에 IK를 적용해야 정상적으로 작동될 것 -> 그래서 아예 LateUpdate로 옮김
SkeletalMeshComponent : 설정된 IK를 적용시키기 위해서 LateUpdate에서 Solver 호출
*/