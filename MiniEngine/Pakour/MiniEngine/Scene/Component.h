#pragma once
#include <memory>

namespace MiniEngine
{
    class Actor;
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void OnAttach() {}
        virtual void Tick(float /*dt*/) {}
    
        std::weak_ptr<Actor> owner;
    };
}
