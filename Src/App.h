#pragma once
#include "Common.h"
#include "View.h"
#include "Scene.h"
#include "UploadBuffer.h"
#include "ResourceHeap.h"

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
	void InitDescriptorHeaps();
	void InitUploadBuffer();
	void InitResourceHeaps();
	void InitView();
	void InitScene();
	void InitSurfaces();
	void InitRaytracePipelines();

	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvUavDescriptorCPU(SrvUav::Id srvId) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvUavDescriptorGPU(SrvUav::Id srvId) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorCPU(RTV::Id rtvId) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetRtvDescriptorGPU(RTV::Id rtvId) const;

	void AdvanceGfxFrame();

private:

	// Base D3D
#if defined(DEBUG) || defined (_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device5> m_d3dDevice;

	Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> m_pixAnalysis;

	// Command objects
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, k_gfxBufferCount> m_gfxCmdAllocators;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_gfxCmdList;
	uint32_t m_gfxBufferIndex = 0;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_gfxFence;
	std::array<uint64_t, k_gfxBufferCount> m_gfxFenceValues = {};
	HANDLE m_gfxFenceEvent;

	// Swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_swapChainBuffers;

	// Output UAV
	Microsoft::WRL::ComPtr<ID3D12Resource> m_dxrOutput;

	// Descriptor heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;

	// Resource heaps
	ResourceHeap m_geometryDataHeap;
	ResourceHeap m_materialConstantsHeap;

	// Descriptor size
	uint32_t m_rtvDescriptorSize = 0;
	uint32_t m_dsvDescriptorSize = 0;
	uint32_t m_cbvSrvUavDescriptorSize = 0;
	uint32_t m_samplerDescriptorSize = 0;

	// For uploading resources to GPU
	UploadBuffer m_uploadBuffer;

	// Scene
	Scene m_scene;

	// View
	View m_view;

	// Raytracing Pipeline
	std::unique_ptr<RaytraceMaterialPipeline> m_raytracePipeline;

	// Mouse
	WPARAM m_buttonState = {};
	POINT m_currentMousePos = { 0, 0 };
	POINT m_lastMousePos = { 0, 0 };

	// Programmatic capture
	bool m_pixAttached;
	Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> m_pixCapture;
};

// Singleton
App* AppInstance();
