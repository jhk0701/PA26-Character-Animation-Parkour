#include "pch.h"
#include "Scene/World.h"

namespace MiniEngine
{
    void World::BeginPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->BeginPlay();
    }

    void World::Tick(float _dt)
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Tick(_dt);
    }

    void World::Render()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->Render();
    }
}
