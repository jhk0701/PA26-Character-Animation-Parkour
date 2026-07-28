#pragma once
#include <cstdint>
#include <vector>
#include "Core/Math.h"
#include "Core/DebugLine.h"

namespace MiniEngine::Debug 
{
	enum class EMarkerShape
	{
		Cross,
		Sphere,
		Box
	};

	void DrawPoint(const Vector3& _pos, uint32_t _colARGB = DebugColor::YELLOW, 
		float _size = 0.15f, EMarkerShape _shape = EMarkerShape::Cross, float _duration = -1.0f);

	void DrawLine(const Vector3& _start, const Vector3& _end, 
		uint32_t _colARGB = DebugColor::WHITE, float _duration = -1.0f);

	void Clear();

	void NewFrame(float _dt);

	void Collect(std::vector<DebugLine>& _out);
}