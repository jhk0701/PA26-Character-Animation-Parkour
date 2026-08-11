#include "pch.h"
#include "CharacterConfigData.h"

void CharacterConfigData::Load(const json& _data)
{
	Config.minObstacleDetectDist		= _data["minObstacleDetectDist"];
	Config.maxObstacleDetectDist		= _data["maxObstacleDetectDist"];

	Config.heightRadius					= _data["heightRadius"];
	Config.heightStep					= _data["heightStep"];
	Config.maxHeightStep				= _data["maxHeightStep"];
	Config.heightSearchtDist			= _data["heightSearchtDist"];
	Config.heightLift					= _data["heightLift"];

	Config.depthStep					= _data["depthStep"];
	Config.maxDepthStep					= _data["maxDepthStep"];
	Config.depthSearchDownDist			= _data["depthSearchDownDist"];
	Config.depthLift					= _data["depthLift"];
	Config.ledgeDetectRadius			= _data["ledgeDetectRadius"];

	Config.onHangingSearchDist			= _data["onHanging_SearchDist"];
	Config.onHangingSearchFwdDist		= _data["onHanging_SearchFwdDist"];
	Config.onHangingSearchRadius		= _data["onHanging_SearchRadius"];
	Config.onHangingMovableRange		= _data["onHanging_MovableRange"];

	Config.onLandingFallingCheckDist	= _data["onLanding_FallingCheckDist"];
}
