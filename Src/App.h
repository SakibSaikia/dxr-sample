#pragma once
#include "View.h"
#include "Scene.h"

constexpr float k_Pi = 3.1415926535f;

constexpr size_t k_screenWidth = 1280;
constexpr size_t k_screenHeight = 720;
constexpr size_t k_cbvCount = 16;
constexpr size_t k_srvCount = 64;
constexpr DXGI_FORMAT k_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT k_backBufferRTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
constexpr DXGI_FORMAT k_depthStencilFormatRaw = DXGI_FORMAT_R24G8_TYPELESS;
constexpr DXGI_FORMAT k_depthStencilFormatDsv = DXGI_FORMAT_D24_UNORM_S8_UINT;

enum class ConstantBufferId : uint32_t
{
	View = 0, // double-buffered
	Scene = 2,
	//
	// .... reserved for scene constants
	//
	Max = 11
};

class App
{
public:

	void Init(HWND windowHandle);
	void Destroy();
	void Update(float dt);
	void Render();

	void OnMouseMove(WPARAM btnState, int x, int y);

	void SubmitCommands();
	void FlushCmdQueue();

private:

	void InitBaseD3D();
	void InitCommandObjects();
	void InitSwapChain(HWND windowHandle);
	void InitDescriptors();
	void InitView();
	void InitScene();
	void InitStateObjects();
	void InitMaterialPipelines();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDepthStencilView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetConstantBufferDescriptorCPU(ConstantBufferId id, uint32_t offset) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantBufferDescriptorGPU(ConstantBufferId id, uint32_t offset) const;

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
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	uint32_t m_rtvDescriptorSize = 0;
	uint32_t m_dsvDescriptorSize = 0;
	uint32_t m_cbvSrvUavDescriptorSize = 0;
	uint32_t m_samplerDescriptorSize = 0;

	// Scene
	Scene m_scene;

	// View
	View m_view;

	// State Objects
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_basePassPSODesc;

	// Mouse
	POINT m_currentMousePos = { 0, 0 };
	POINT m_lastMousePos = { 0, 0 };
};

// Singleton
App* AppInstance();
