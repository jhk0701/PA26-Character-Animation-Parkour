#pragma once
#include "Scene/Actor.h"
#include "Perception/Interface/IObstacle.h"
#include "Physics/CollsionLayer.h"

namespace MiniEngine 
{ 
	class StaticMesh; 
	class Scene;
};

struct ObstacleDesc
{
	std::shared_ptr<MiniEngine::StaticMesh> pMesh;
	MiniEngine::Vector3 color = MiniEngine::Vector3(0.7f, 0.5f, 0.2f);
	MiniEngine::Vector3 pos;
	MiniEngine::Vector3 scale;
	MiniEngine::Vector3 meshPos;
	MiniEngine::Quaternion rot = MiniEngine::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	MiniEngine::Physics::Layer layer = MiniEngine::Physics::Layer::Obstacle;

	uint8_t tagEnvDetail{ 0U };
	uint8_t tagEnvSubInfo{ 0U };
	uint8_t tagPriority{ 0U };
};

// 기본 장애물 구현
class Obstacle : public MiniEngine::Actor, public MiniEngine::IObstacle
{
public:
	virtual ~Obstacle() {};
	virtual void Construct(const ObstacleDesc& _desc);

	// IObstacle을(를) 통해 상속됨
	virtual float GetNearestLedgeHeight(const MiniEngine::Vector3& _pos) const override;
	bool TryGetTag(uint8_t _idx, uint8_t& _outTag) const override;
	const MiniEngine::Transform& GetTransform() const override;
	uint8_t GetPriority() const override;

protected:
	void AddLedge(
		std::shared_ptr<MiniEngine::SceneComponent> _parent,
		const MiniEngine::Vector3& _localPos,
		const MiniEngine::Vector3& _halfExtent
	);

	const std::vector<std::weak_ptr<MiniEngine::SceneComponent>>& GetLedges() const { return m_pLedges; }

private:
	std::vector<std::weak_ptr<MiniEngine::SceneComponent>> m_pLedges;

#ifdef MG_DEBUG_LOG
public:
	virtual const std::string& DebugName() override;
#endif
};

// 연출용 장애물 구현
class DirectingObstacle : public Obstacle, public MiniEngine::IDirectable
{
public:
	void Construct(const ObstacleDesc& _desc) override;

	float GetNearestLedgeHeight(const MiniEngine::Vector3& _pos) const override;

	void SetDirectTagAct(uint8_t _tag) override;
	uint8_t GetDirectTagAct() const override;
};

class ObstacleFactory 
{
public:
	static std::shared_ptr<MiniEngine::Actor> Create(
		std::shared_ptr<MiniEngine::Scene> _pScene,
		const ObstacleDesc& _desc
	);

	static std::shared_ptr<MiniEngine::Actor> Create(
		std::shared_ptr<MiniEngine::Scene> _pScene,
		const ObstacleDesc& _desc,
		uint8_t _tagAct
	);
};