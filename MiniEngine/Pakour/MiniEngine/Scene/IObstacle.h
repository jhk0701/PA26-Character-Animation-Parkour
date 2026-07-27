#pragma once

namespace MiniEngine 
{
	struct Transform;
	class IObstacle
	{
	public:
		virtual float GetNearestLedgeHeight(const Vector3& _pos) const = 0;
		virtual bool TryGetTag(uint8_t _idx, uint8_t& _outTag) = 0;
		virtual const Transform& GetTransform() const = 0;
	};
}