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
		virtual uint8_t GetPriority() const = 0;

#ifdef MG_DEBUG_LOG
		virtual const std::string& DebugName() = 0;
#endif // MG_DEBUG_LOG
	};
}