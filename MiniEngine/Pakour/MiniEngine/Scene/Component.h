#pragma once
#include <memory>

namespace MiniEngine
{
    class Actor;

    // Actor에 기능/데이터를 붙이는 최소 단위. (CLAUDE.md §6)
    // owner(상위)는 weak_ptr로 참조해 순환참조를 방지한다. (§12)
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void OnAttach() {}
        virtual void Tick(float /*dt*/) {}

        std::weak_ptr<Actor> owner;
    };
}
