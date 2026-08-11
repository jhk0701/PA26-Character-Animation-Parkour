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