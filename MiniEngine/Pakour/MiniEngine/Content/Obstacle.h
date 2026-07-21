#pragma once
#include "Scene/Actor.h"

namespace MiniEngine 
{ 
	class StaticMesh; 
	class Scene;
};

class Obstacle : public MiniEngine::Actor
{
public:
	virtual ~Obstacle() {};
	void Construct(
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh, 
		const MiniEngine::Vector3& _pos, 
		const MiniEngine::Vector3& _scale,
		const MiniEngine::Quaternion& _rot,
		uint8_t _detailTag,
		bool _addLedge);

private:
	void AddLedge(std::shared_ptr<MiniEngine::Actor> _pTarget,
		std::shared_ptr<MiniEngine::StaticMesh> _pStaticMesh,
		const MiniEngine::Vector3& _localPos,
		const MiniEngine::Vector3& _halfExtent,
		const MiniEngine::Quaternion& _localRot);
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
		uint8_t _detailTag = 0U, // (uint8_t)Content::Config::ETagEnvDetail::Default
		bool _addLedge = true);
};