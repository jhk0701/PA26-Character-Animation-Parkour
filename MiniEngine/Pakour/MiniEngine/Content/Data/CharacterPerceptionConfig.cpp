#include "pch.h"
#include "CharacterPerceptionConfig.h"

void CharacterPerceptionConfig::Load(const json& _data)
{
	Config.minObstacleDetectDist	= _data["minObstacleDetectDist"];
	Config.maxObstacleDetectDist	= _data["maxObstacleDetectDist"];

	Config.thresholdWallHeight		= _data["thresholdWallHeight"];
	Config.thresholdHighObstacle	= _data["thresholdHighObstacle"];
	Config.thresholdLowObstacle		= _data["thresholdLowObstacle"];

	Config.heightRadius				= _data["heightRadius"];
	Config.heightStep				= _data["heightStep"];
	Config.maxHeightStep			= _data["maxHeightStep"];
	Config.heightSearchtDist		= _data["heightSearchtDist"];

	Config.depthStep				= _data["depthStep"];
	Config.maxDepthStep				= _data["maxDepthStep"];
	Config.depthSearchDownDist		= _data["depthSearchDownDist"];
	Config.depthLift				= _data["depthLift"];
	Config.minMantleDepth			= _data["minMantleDepth"];

	Config.onHangingSearchVDist		= _data["onHangingSearchVDist"];
	Config.onHangingSearchHDist		= _data["onHangingSearchHDist"];
	Config.onHangingSearchRadius	= _data["onHangingSearchRadius"];

	Config.onLandingFallingCheckDist = _data["onLandingFallingCheckDist"];
}
