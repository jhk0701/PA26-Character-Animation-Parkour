#pragma once
#include "Scene/Scene.h"

class Character;

namespace MiniEngine { class StaticMesh;  }

class TestScene : public MiniEngine::Scene
{
public:
	TestScene();
	virtual ~TestScene();

	virtual void Construct(ID3D11Device* _device, ID3D11DeviceContext* _context) override;
	virtual void BeginPlay() override;
};