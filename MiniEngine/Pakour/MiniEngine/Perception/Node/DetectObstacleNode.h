#pragma once
#include "Perception/PerceptionComponent.h"

namespace MiniEngine 
{ 
	class IObstacle; 
	class IPerceptionProcessor;
}
namespace MiniEngine::Physics 
{ 
	struct RaycastMultipleResult; 
}

using namespace MiniEngine;

// 공통 작업용 부모 클래스
class DetectObstacle : public TaskNode
{
public:
	void SetStartOffset(const Vector3& _pos) { m_startOffset = _pos; }
	void SetDirection(const Vector3& _dir) { m_dir = _dir; }
	void SetDistance(const float _dist) { m_dist = _dist; }
	void SetHeightMultiplier(const float _mul) { m_heightMultiplier = _mul; }

protected:
	void SortResults(MiniEngine::Physics::RaycastMultipleResult& _result) const;
	void FilterResults(
		std::shared_ptr<IPerceptionProcessor>& _pProcessor,
		TravelContext& _context,
		MiniEngine::Physics::RaycastMultipleResult& _result) const;

	void ApplyOwnerTransform(const Transform& _inOwnerTf, Vector3& _outPos, Vector3& _outDir) const;

	const Vector3& GetStartOffset() const { return m_startOffset; }
	const Vector3& GetDirection() const { return m_dir; }
	const float GetDistance() const { return m_dist; }
	const float GetHeightMultiplier() const { return m_heightMultiplier; }

private:
	Vector3 m_startOffset{ 0.0f };
	Vector3 m_dir{ 0.0f, 0.0f, 1.0f };
	float m_dist{ 1.0f };
	float m_heightMultiplier{ 1.0f };
};

class DetectObstacleCapsule : public DetectObstacle 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

	void SetCapsuleRadius(const float _r) { m_capsuleRadius = _r; }
	void SetCapsuleHeight(const float _h) { m_capsuleHeight = _h; }

private:
	float m_capsuleRadius{ 0.5f };
	float m_capsuleHeight{ 1.0f };
};

class DetectObstacleSphere : public DetectObstacle 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

	void SetRadius(const float _r) { m_radius = _r; }

private:
	float m_radius{ 0.5f };
};