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

private:
	std::shared_ptr<MiniEngine::Actor> BuildObstacle(
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh,
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _scale);

	void AddLedgeToObstacle(std::shared_ptr<MiniEngine::Actor> _pTarget, 
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh,
		const MiniEngine::Vector3& _localPos, 
		const MiniEngine::Vector3& _halfExtent, 
		const MiniEngine::Quaternion& _localRot); // 0~360f
};