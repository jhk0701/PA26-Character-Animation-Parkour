#include "pch.h"
#include "Core/Log.h"
#include "Scene/World.h"


namespace MiniEngine
{
    void World::BeginPlay()
    {
        if (m_physics.Init() == false)
        {
            MG_LOG_ERROR("World : PhysX Init failed");
            return;
        }

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

    void World::EndPlay()
    {
        for (std::shared_ptr<Actor>& actor : m_actors)
            actor->EndPlay();

        m_physics.Shutdown();
    }
}
