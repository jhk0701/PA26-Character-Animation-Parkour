#pragma once
#include "Perception/PerceptionComponent.h"

using namespace MiniEngine;

// 선행 조건 : Obstacle을 이미 감지해서 최초 접촉점이 있어야함
class MeasureDepthNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

// 선행 조건 : Obstacle을 이미 감지해서 최초 접촉점이 있어야함
class MeasureHeightNode : public TaskNode 
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

// 선행 조건 : Obstacle을 이미 감지해서 최초 접촉점이 있어야함
class MeasureDepth_SideNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};

// 선행 조건 : Obstacle을 이미 감지해서 최초 접촉점이 있어야함
class MeasureHeight_SideNode : public TaskNode
{
public:
	EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
};
