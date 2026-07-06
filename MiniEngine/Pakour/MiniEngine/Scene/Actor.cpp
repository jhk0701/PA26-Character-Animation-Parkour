#include "pch.h"
#include "Scene/Actor.h"

namespace MiniEngine
{
    void Actor::Tick(float _dt)
    {
        for (auto& comp : m_components)
            comp->Tick(_dt);
    }
}
