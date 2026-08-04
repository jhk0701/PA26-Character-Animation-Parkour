#pragma once
#include "Scene/Actor.h"
#include "Scene/IObstacle.h"
#include "Physics/CollsionLayer.h"

namespace MiniEngine 
{ 
	class StaticMesh; 
	class Scene;
};

class Obstacle : public MiniEngine::Actor, public MiniEngine::IObstacle
{
public:
	enum ELedgeOption 
	{
		None,
		Single,
		Vertical,
		Horizontal,
		All
	};

	struct ObstacleDesc 
	{
		std::shared_ptr<MiniEngine::StaticMesh> pMesh;
		MiniEngine::Vector3 color = MiniEngine::Vector3(0.7f, 0.5f, 0.2f);
		MiniEngine::Vector3 pos;
		MiniEngine::Vector3 scale;
		MiniEngine::Quaternion rot = MiniEngine::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		std::vector<uint8_t> detailTags;
		uint8_t priority{0};
		MiniEngine::Physics::Layer layer = MiniEngine::Physics::Layer::Obstacle;
		ELedgeOption ledgeOpt = ELedgeOption::All;
	};

	virtual ~Obstacle() {};
	void Construct(const ObstacleDesc& _desc);

private:
	void AddLedge(const ObstacleDesc& _desc);
	void AddLedge(
		const MiniEngine::Vector3& _localPos,
		const MiniEngine::Vector3& _halfExtent,
		const MiniEngine::Quaternion& _localRot);

	std::vector<std::weak_ptr<MiniEngine::SceneComponent>> m_pLedges;

public:
	// IObstacle을(를) 통해 상속됨
	virtual float GetNearestLedgeHeight(const MiniEngine::Vector3& _pos) const override;
	virtual bool TryGetTag(uint8_t _idx, uint8_t& _outTag) override;
	virtual const MiniEngine::Transform& GetTransform() const override;
	virtual const std::string& DebugName() override;
};

class ObstacleFactory 
{
public:
	static std::shared_ptr<MiniEngine::Actor> Create(std::shared_ptr<MiniEngine::Scene> _pScene,
		const Obstacle::ObstacleDesc& _desc);
};