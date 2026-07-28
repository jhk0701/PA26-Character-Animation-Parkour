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
        void SetSortOrder(int _o) { m_sortOrder = _o; };
        // 우선순위가 높다 -> 먼저 tick에서 호출
        const uint8_t GetSortOrder() const { return m_sortOrder; }

        std::weak_ptr<Actor> owner;
    private:
        uint8_t m_sortOrder{ 0 }; 
    };
}
