#pragma once
#include "Perception/PerceptionComponent.h"

namespace MiniEngine 
{
	class ProcessBeamNode : public TaskNode 
	{
	public:
		EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
	};

	class ProcessProtrudeNode : public TaskNode
	{
	public:
		EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;
	};

	class ProcessPoleNode : public TaskNode
	{
	public:
		EPerceptionResult InvokeTask(TravelContext& _context, TravelResult& _result) override;

		void SetHeight(const float _limit) { m_heightLimit = _limit; }
	private:
		float m_heightLimit{ 2.0f };
	};
}