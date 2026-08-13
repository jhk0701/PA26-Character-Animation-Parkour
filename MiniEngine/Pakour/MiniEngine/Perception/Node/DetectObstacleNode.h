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
class DetectNode : public TaskNode
{
public:
	virtual ~DetectNode() {};

	void SetStartOffset(const Vector3& _pos) { m_startOffset = _pos; }
	void SetDirection(const Vector3& _dir) { m_dir = _dir; }
	void SetDistance(const float _dist) { m_dist = _dist; }
	void SetRadius(const float _r) { m_radius = _r; }

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
	const float GetRadius() const { return m_radius; }

private:
	Vector3 m_startOffset{ 0.0f };
	Vector3 m_dir{ 0.0f, 0.0f, 1.0f };
	float m_dist{ 1.0f };
	float m_radius{ 0.5f };
};

class DetectObstacleCapsuleNode : public DetectNode
{
public:
	virtual ~DetectObstacleCapsuleNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

	void SetCapsuleHeight(const float _h) { m_capsuleHeight = _h; }
	void SetHeightMultiplier(const float _mul) { m_heightMultiplier = _mul; }

private:
	float m_capsuleHeight{ 1.0f };
	float m_heightMultiplier{ 1.0f };
};

class DetectObstacleSphereNode : public DetectNode
{
public:
	virtual ~DetectObstacleSphereNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class DetectLedgeNode : public DetectNode
{
public:
	virtual ~DetectLedgeNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class DetectLedgeMultipleNode : public DetectNode
{
public:
	virtual ~DetectLedgeMultipleNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class CheckObstacleSphereNode : public DetectNode
{
public:
	virtual ~CheckObstacleSphereNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class DetectFloorNode : public DetectNode
{
public:
	virtual ~DetectFloorNode() {};
	virtual EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};
