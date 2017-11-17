#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <array>

// Double buffered
constexpr size_t m_bufferCount = 2;

class App
{
public:
	virtual bool Init();
	virtual bool Destroy();
	virtual void Update();
	virtual void Render();
private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, m_bufferCount> m_swapChainBuffer;
	int32_t m_currentBackBuffer = 0;

	size_t m_rtvDescriptorSize;
	size_t m_dsvDescriptorSize;
	size_t m_cbvSrvUavDescriptorSize;
	size_t m_samplerDescriptorSize;
};
