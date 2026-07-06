#include "pch.h"
#include "Scene/World.h"

namespace MiniEngine
{
    void World::BeginPlay()
    {
        for (auto& actor : m_actors)
            actor->BeginPlay();
    }

    void World::Tick(float _dt)
    {
        for (auto& actor : m_actors)
            actor->Tick(_dt);
    }
}
