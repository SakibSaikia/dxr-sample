#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "D3DX12\d3dx12.h"
#include <array>

namespace Engine
{
	constexpr size_t k_frameCount = 2;
	constexpr size_t k_screenWidth = 1280;
	constexpr size_t k_screenHeight = 720;
	constexpr DXGI_FORMAT k_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	class App
	{
	public:
		bool Init(HWND windowHandle);
		bool Destroy();
		void Update();
		void Render() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	private:

#if defined(DEBUG) || defined (_DEBUG)
		Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif

		Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;

		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_frameCount> m_swapChainBuffer;
		uint32_t m_currentBackBuffer = 0;

		uint32_t m_rtvDescriptorSize;
		uint32_t m_dsvDescriptorSize;
		uint32_t m_cbvSrvUavDescriptorSize;
		uint32_t m_samplerDescriptorSize;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	};
}
