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
        virtual void Tick(float _dt) {}
        virtual void FixedTick(float _dt) {}
        virtual void Render(Graphics::RenderContext& _context) {}
    
        std::weak_ptr<Actor> owner;
    };
}
