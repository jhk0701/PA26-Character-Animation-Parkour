#pragma once

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

class UIBase;
class GUIManager
{
	SINGLETON(GUIManager)

public:
	bool Init(HWND hWnd, ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue, float w, float h);
	void Clear();

	void UpdateGUI();
	void RenderGUI(GraphicsContext& gfxContext);

	template<typename T>
	std::weak_ptr<T> CreateUI();

private:
	ID3D12DescriptorHeap* m_pGuiDesc;
	std::list<std::shared_ptr<UIBase>> m_UIInstances;

	void InitGuiDesc(ID3D12Device* pDevice);
};

template<typename T>
inline std::weak_ptr<T> GUIManager::CreateUI()
{
	m_UIInstances.push_back(std::make_shared<T>());
	return std::dynamic_pointer_cast<T>(m_UIInstances.back());
}
