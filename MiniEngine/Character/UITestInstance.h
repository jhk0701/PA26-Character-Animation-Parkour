#pragma once
#include "UIBase.h"

class UITestInstance : public UIBase
{
public:
	void SetText(const std::string& text) { curText = text; };

protected:
	virtual void ConstructUI() override;

private:
	std::string curText{"Not Set"};
};