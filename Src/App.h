#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <pix3.h>
#include <array>

#include "Camera.h"

constexpr float k_Pi = 3.1415926535f;

constexpr size_t k_gfxBufferCount = 2;
constexpr size_t k_screenWidth = 1280;
constexpr size_t k_screenHeight = 720;
constexpr DXGI_FORMAT k_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT k_depthStencilFormatRaw = DXGI_FORMAT_R24G8_TYPELESS;
constexpr DXGI_FORMAT k_depthStencilFormatDsv = DXGI_FORMAT_D24_UNORM_S8_UINT;

__declspec(align(256))
struct ViewConstants
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 viewProjectionMatrix;
};

class App
{
public:

	void Init(HWND windowHandle);
	void Destroy();
	void Update(float dt);
	void Render();

	void OnMouseMove(WPARAM btnState, int x, int y);

private:

	void InitBaseD3D();
	void InitCommandObjects();
	void InitSwapChain(HWND windowHandle);
	void InitDescriptors();
	void InitShaders();
	std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, Microsoft::WRL::ComPtr<ID3D12Resource>> InitGeometry();
	void InitStateObjects();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDepthStencilView() const;

	void FlushCmdQueue();
	void AdvanceGfxFrame();

private:

	// Base D3D
#if defined(DEBUG) || defined (_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;

	// Command objects
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, k_gfxBufferCount> m_gfxCmdAllocators;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_gfxCmdList;
	uint32_t m_gfxBufferIndex = 0;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_gfxFence;
	std::array<uint64_t, k_gfxBufferCount> m_gfxFenceValues = {};
	HANDLE m_gfxFenceEvent;

	// Swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_swapChainBuffers;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_depthStencilBuffers;

	// Descriptors
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
	uint32_t m_rtvDescriptorSize = 0;
	uint32_t m_dsvDescriptorSize = 0;
	uint32_t m_cbvSrvUavDescriptorSize = 0;
	uint32_t m_samplerDescriptorSize = 0;

	// Constant buffers
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_viewConstantBuffers;
	std::array<ViewConstants*, k_gfxBufferCount> m_ViewConstantBufferPtr;

	// Shaders
	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode;

	// Geometry
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// State Objects
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	// View
	Camera m_camera;
	DirectX::XMFLOAT4X4 m_projMatrix;

	// Mouse
	POINT m_currentMousePos = { 0, 0 };
	POINT m_lastMousePos = { 0, 0 };
};

// Singleton
App* AppInstance();
