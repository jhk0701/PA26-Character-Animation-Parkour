#pragma once
#include "UI/UIBase.h"

namespace MiniEngine { class Scene; }
using namespace MiniEngine;

class UIDebugMarkerOption : public UIBase
{
public:
	void Construct() override;
	
	void SetScene(std::shared_ptr<Scene> _pScene) { m_pScene = _pScene; }

protected:
	void DrawUI() override;

private:
	std::weak_ptr<Scene> m_pScene;

	bool m_bApplyPhysic{ false };
	bool m_bApplyMarker{ false };
};