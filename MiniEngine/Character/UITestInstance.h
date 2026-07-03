#pragma once
#include "UIBase.h"

class UITestInstance : public UIBase
{
public:
	void SetText(const std::string& text) { m_curText = text; };
	void BindBlendValue(float* const var) { m_pBlendVal = var; };

protected:
	virtual void ConstructUI() override;

private:
	std::string m_curText{"Not Set"};
	float* m_pBlendVal{nullptr};
};