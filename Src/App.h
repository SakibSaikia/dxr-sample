#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "D3DX12\d3dx12.h"
#include <array>

namespace Engine
{
	constexpr size_t k_frameCount = 2;
	constexpr size_t k_screenWidth = 1280;
	constexpr size_t k_screenHeight = 720;
	constexpr DXGI_FORMAT k_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr DXGI_FORMAT k_depthStencilFormatRaw = DXGI_FORMAT_R24G8_TYPELESS;
	constexpr DXGI_FORMAT k_depthStencilFormatDsv = DXGI_FORMAT_D24_UNORM_S8_UINT;

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

		void FlushCommandQueue() const;


	private:

#if defined(DEBUG) || defined (_DEBUG)
		Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif
		// Base D3D
		Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

		// Command objects
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

		// Back buffer & Depth Stencil
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_frameCount> m_swapChainBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;
		mutable uint32_t m_currentBackBuffer = 0;

		// Cached descriptor size
		uint32_t m_rtvDescriptorSize = 0;
		uint32_t m_dsvDescriptorSize = 0;
		uint32_t m_cbvSrvUavDescriptorSize = 0;
		uint32_t m_samplerDescriptorSize = 0;

		// Descriptor heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

		// Fence
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		mutable uint64_t m_currentFenceValue = 0;

		// Constant buffers
		Microsoft::WRL::ComPtr<ID3D12Resource> m_viewConstantBuffer;

		// Shaders
		Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode;
		Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode;

		// Root signatures
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

		// PSO
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		// Viewport & Scissor
		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;
	};

	// Singleton
	App* AppInstance();
}
