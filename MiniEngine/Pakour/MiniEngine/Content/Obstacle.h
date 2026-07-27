#pragma once
#include "Scene/Actor.h"
#include "Scene/IObstacle.h"

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
		Vertical,
		Horizontal,
		All
	};

	virtual ~Obstacle() {};
	void Construct(
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _scale,
		const MiniEngine::Quaternion& _rot,
		const std::vector<uint8_t>& _detailTags,
		ELedgeOption _opt);

private:
	void AddLedge(std::shared_ptr<MiniEngine::Actor> _pTarget,
		const MiniEngine::Vector3& _localPos,
		const MiniEngine::Vector3& _halfExtent,
		const MiniEngine::Quaternion& _localRot);

	std::vector<std::weak_ptr<MiniEngine::SceneComponent>> m_pLedges;

public:
	// IObstacle을(를) 통해 상속됨
	virtual float GetNearestLedgeHeight(const MiniEngine::Vector3& _pos) const override;
	virtual bool TryGetTag(uint8_t _idx, uint8_t& _outTag) override;
	virtual const MiniEngine::Transform& GetTransform() const override;
};

class ObstacleFactory 
{
public:
	static std::shared_ptr<MiniEngine::Actor> Create(
		std::shared_ptr<MiniEngine::Scene> _pScene, 
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _scale,
		const MiniEngine::Quaternion& _rot,
		const std::vector<uint8_t>& _detailTags = { 0U },
		Obstacle::ELedgeOption _opt = Obstacle::ELedgeOption::All);
};