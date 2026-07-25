#include "pch.h"
#include "Scene/Pawn.h"

using namespace MiniEngine;

void Pawn::OnPossessed(std::shared_ptr<Controller> _pController, Input& _input)
{
	m_pController = _pController;
}
