#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>
#include "Scene/Actor.h"

namespace MiniEngine
{
    // Actor들을 소유하고 매 프레임 Tick. (CLAUDE.md §6)
    class World
    {
    public:
        // Actor를 생성해 월드에 등록하고 핸들을 반환(소유권은 World, shared). (§12)
        template<typename T = Actor, typename... Args>
        std::shared_ptr<T> SpawnActor(Args&&... _args)
        {
            static_assert(std::is_base_of<Actor, T>::value, "T must derive from Actor");
            auto actor = std::make_shared<T>(std::forward<Args>(_args)...);
            m_actors.push_back(actor);
            return actor;
        }

        void BeginPlay();
        void Tick(float _dt);

        const std::vector<std::shared_ptr<Actor>>& GetActors() const { return m_actors; }

    private:
        std::vector<std::shared_ptr<Actor>> m_actors;
    };
}
