#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

class MeasureObstacleNode : public TaskNode 
{
public:
	void SetDirection(const Vector3& _dir) { m_dir = _dir; }

protected:
	const Vector3& GetDirection() const { return m_dir; }

private:
	Vector3 m_dir;
};

class MeasureObstacleHeightNode : public MeasureObstacleNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

class MeasureObstacleDepthNode : public MeasureObstacleNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

// 여유 공간이 있는지 확인용도
class CheckRoomNode : public MeasureObstacleNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

	void SetStartOffset(const Vector3& _offset) { m_startOffset = _offset; }
	void SetDistance(const float _dist) { m_distance = _dist; }
	void SetRadius(const float _radius) { m_radius = _radius; }

private:
	Vector3 m_startOffset;
	float m_distance{ 1.0f };
	float m_radius{ 0.25f };
};
