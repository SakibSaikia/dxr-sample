#include "App.h"
#include <cassert>

bool Engine::App::Init(HWND windowHandle)
{
	// Debug layer
	{
#if defined(DEBUG) || defined(_DEBUG)
		// Must be called before D3D12 device is created
		D3D12GetDebugInterface(IID_PPV_ARGS(m_debugController.GetAddressOf()));
		m_debugController->EnableDebugLayer();
#endif
	}

	// DXGI
	{
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create DXGI factory");
	}

	// Device
	{
		HRESULT hr = D3D12CreateDevice(
			nullptr, // default adapter
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(m_d3dDevice.GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create D3D Device");
	}

	// Cached descriptor size
	{
		m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_cbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_samplerDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	// Command Queue
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		HRESULT hr = m_d3dDevice->CreateCommandQueue(
			&cmdQueueDesc,
			IID_PPV_ARGS(m_cmdQueue.GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create command queue");
	}

	// Command Allocator
	{
		HRESULT hr = m_d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_cmdAllocator.GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create command allocator");
	}

	// Command List
	{
		HRESULT hr = m_d3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_cmdAllocator.Get(),
			nullptr, // Initial PSO
			IID_PPV_ARGS(m_cmdList.GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create command list");

		m_cmdList->Close();
	}

	// Swap Chain
	{
		DXGI_SWAP_CHAIN_DESC scDesc = {};
		scDesc.BufferDesc.Width = k_screenWidth;
		scDesc.BufferDesc.Height = k_screenHeight;
		scDesc.BufferDesc.RefreshRate.Numerator = 60;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferDesc.Format = k_backBufferFormat;
		scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scDesc.SampleDesc.Count = 1; // No MSAA
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = k_frameCount;
		scDesc.OutputWindow = windowHandle;
		scDesc.Windowed = true;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT hr = m_dxgiFactory->CreateSwapChain(
			m_cmdQueue.Get(),
			&scDesc,
			m_swapChain.GetAddressOf()
		);

		assert(hr == S_OK && L"Failed to create swap chain");
	}

	// Descriptor Heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = k_frameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT hr = m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create RTV heap");

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = k_frameCount;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hr = m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create DSV heap");
	}

	return true;
}

bool Engine::App::Destroy()
{
	return true;
}

void Engine::App::Update()
{

}

void Engine::App::Render() const
{

}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::App::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_currentBackBuffer,
		m_rtvDescriptorSize
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::App::GetDepthStencilView() const
{
	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}
