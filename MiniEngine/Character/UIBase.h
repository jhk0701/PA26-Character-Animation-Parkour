#pragma once

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

class UIBase
{
public:
	UIBase();
	virtual ~UIBase();
	
	void Update();

protected:
	virtual void ConstructUI() = 0;
};