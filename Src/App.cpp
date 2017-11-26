#include "App.h"
#include <cassert>

namespace Engine
{

	struct PerObjectConstants
	{
		DirectX::XMFLOAT4X4 viewProjectionMatrix;
	};

	App* AppInstance()
	{
		static App instance;
		return &instance;
	}

	bool App::Init(HWND windowHandle)
	{
#pragma region InitD3D
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

			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			hr = m_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf()));
			assert(hr == S_OK && L"Failed to create CBV heap");
		}

		// Render Target Views
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
			for (auto bufferIdx = 0; bufferIdx < k_frameCount; bufferIdx++)
			{
				HRESULT hr = m_swapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(m_swapChainBuffer.at(bufferIdx).GetAddressOf()));
				assert(hr == S_OK && L"Failed to initialize back buffer");

				m_d3dDevice->CreateRenderTargetView(m_swapChainBuffer.at(bufferIdx).Get(), nullptr, rtvHeapHandle);
				rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
			}
		}

		// Depth Stencil View
		{
			D3D12_RESOURCE_DESC resDesc = {};
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resDesc.Width = k_screenWidth;
			resDesc.Height = k_screenHeight;
			resDesc.DepthOrArraySize = 1;
			resDesc.MipLevels = 1;
			resDesc.Format = k_depthStencilFormatRaw;
			resDesc.SampleDesc.Count = 1; // No MSAA
			resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_HEAP_PROPERTIES heapDesc = {};
			heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

			D3D12_CLEAR_VALUE zClear;
			zClear.Format = k_depthStencilFormatDsv;
			zClear.DepthStencil.Depth = 1.f;
			zClear.DepthStencil.Stencil = 0;

			HRESULT hr = m_d3dDevice->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&zClear,
				IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())
			);

			assert(hr == S_OK && L"Failed to create DepthStencil buffer");

			// Descriptor to mip level 0
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = k_depthStencilFormatDsv;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // Modify if you want a read-only DSV
			m_d3dDevice->CreateDepthStencilView(
				m_depthStencilBuffer.Get(),
				&dsvDesc,
				m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
			);
		}

		// Constant Buffer
		{
			// 256 byte aligned
			uint32_t objConstantBufferSize = (sizeof(PerObjectConstants) + 0xff) & ~0xff;

			D3D12_RESOURCE_DESC resDesc = {};
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resDesc.Width = objConstantBufferSize;
			resDesc.Height = 1;
			resDesc.DepthOrArraySize = 1;
			resDesc.MipLevels = 1;
			resDesc.Format = DXGI_FORMAT_UNKNOWN;
			resDesc.SampleDesc.Count = 1; 
			resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			D3D12_HEAP_PROPERTIES heapDesc = {};
			heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

			HRESULT hr = m_d3dDevice->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_objectConstantBuffer.GetAddressOf())
			);
			assert(hr == S_OK && L"Failed to create constant buffer");

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_objectConstantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = objConstantBufferSize;

			m_d3dDevice->CreateConstantBufferView(
				&cbvDesc, 
				m_cbvHeap->GetCPUDescriptorHandleForHeapStart()
			);
		}

		// Fence
		{
			HRESULT hr = m_d3dDevice->CreateFence(m_currentFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
			assert(hr == S_OK && L"Failed to create fence");
		}

		// Viewport
		{
			m_viewport.TopLeftX = 0.f;
			m_viewport.TopLeftY = 0.f;
			m_viewport.Width = static_cast<float>(k_screenWidth);
			m_viewport.Height = static_cast<float>(k_screenHeight);
			m_viewport.MinDepth = 0.f;
			m_viewport.MaxDepth = 1.f;
		}

		// Scissor
		{
			m_scissorRect = { 0, 0, k_screenWidth, k_screenHeight };
		}
#pragma endregion InitD3D

#pragma region InitScene
		
#pragma endregion InitScene

		return true;
	}

	bool App::Destroy()
	{
		return true;
	}

	void App::Update()
	{

	}

	void App::Render() const
	{
		// Reset command list
		m_cmdAllocator->Reset();
		m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);

		// Rasterizer state
		m_cmdList->RSSetViewports(1, &m_viewport);
		m_cmdList->RSSetScissorRects(1, &m_scissorRect);

		// Transition back buffer from present to render target
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_swapChainBuffer.at(m_currentBackBuffer).Get();
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_cmdList->ResourceBarrier(
			1,
			&barrierDesc
		);

		// Clear
		float clearColor[] = { 1.f, 0.f, 0.f, 0.f };
		m_cmdList->ClearRenderTargetView(GetCurrentBackBufferView(), clearColor, 0, nullptr);
		m_cmdList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

		// Transition back buffer from render target to present
		barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_swapChainBuffer.at(m_currentBackBuffer).Get();
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_cmdList->ResourceBarrier(
			1,
			&barrierDesc
		);

		// Done recording
		m_cmdList->Close();

		// Execute
		ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
		m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

		// Present and swap buffers
		m_swapChain->Present(0, 0);
		m_currentBackBuffer = (m_currentBackBuffer + 1) % k_frameCount;

		// Flush GPU queue
		FlushCommandQueue();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE App::GetCurrentBackBufferView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_currentBackBuffer,
			m_rtvDescriptorSize
		);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE App::GetDepthStencilView() const
	{
		return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	void App::FlushCommandQueue() const
	{
		m_cmdQueue->Signal(m_fence.Get(), m_currentFenceValue);

		HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		m_fence->SetEventOnCompletion(m_currentFenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);

		CloseHandle(fenceEvent);

		m_currentFenceValue++;
	}

}
