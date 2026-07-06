#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>

#include "Scene/Actor.h"

namespace MiniEngine
{
    class World
    {
    public:
        World() {};
        virtual ~World() {};

        template<typename T = Actor, typename... Args>
        std::shared_ptr<T> SpawnActor(Args&&... _args)
        {
            static_assert(std::is_base_of<Actor, T>::value, "T must derive from Actor");
            auto actor = std::make_shared<T>(std::forward<Args>(_args)...);
            m_actors.push_back(actor);
            return actor;
        }

        virtual void Construct() = 0;
        virtual void BeginPlay();
        virtual void Tick(float _dt);

        const std::vector<std::shared_ptr<Actor>>& GetActors() const { return m_actors; }

    private:
        std::vector<std::shared_ptr<Actor>> m_actors;
    };
}
