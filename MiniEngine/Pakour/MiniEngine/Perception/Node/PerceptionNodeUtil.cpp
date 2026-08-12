#include "pch.h"
#include "Perception/Node/PerceptionNodeUtil.h"
#include "Perception/Interface/IObstacle.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Actor.h"

using namespace MiniEngine;
using namespace MiniEngine::Physics;

namespace PerceptionNodeUtil 
{
	IObstacle* ToIObstacle(void* _p)
	{
		return dynamic_cast<IObstacle*>(reinterpret_cast<Actor*>(_p));
	}

	void LocalizePosition(const Transform& _inTf, const Vector3& _inOffset, Vector3& _outResult)
	{
		_outResult = _inTf.position;

		_outResult += _inOffset.x * _inTf.Right();
		_outResult += _inOffset.y * _inTf.Up();
		_outResult += _inOffset.z * _inTf.Forward();
	}

	void LocalizeDirection(const Transform& _inTf, const Vector3& _inDir, Vector3& _outResult)
	{
		_outResult = Vector3(0.0f);
		
		_outResult += _inDir.x * _inTf.Right();
		_outResult += _inDir.y * _inTf.Up();
		_outResult += _inDir.z * _inTf.Forward();
		_outResult.Normalize();
	}
}

