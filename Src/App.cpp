#include "stdafx.h"
#include "App.h"
#include "StackAllocator.h"

App* AppInstance()
{
	static App instance;
	return &instance;
}

void App::InitBaseD3D()
{
	// Debug layer
#if defined(DEBUG) || defined(_DEBUG)
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(m_debugController.GetAddressOf()));
	if (hr == S_OK)
	{
		m_debugController->EnableDebugLayer();
	}
#endif

	// DXGI
	CHECK(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.GetAddressOf())));

	DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_pixAnalysis.GetAddressOf()));

	// Enumerate adapters and create device
	uint32_t i = 0;
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
	bool deviceCreated = false;
	while(m_dxgiFactory->EnumAdapters(i++, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring out = L"*** Adapter : ";
		out += desc.Description;

		if (desc.VendorId == 0x10DE /*NV*/)
		{
			HRESULT hr = D3D12CreateDevice(
				adapter.Get(),
				D3D_FEATURE_LEVEL_12_1,
				IID_PPV_ARGS(m_d3dDevice.GetAddressOf())
			);
			assert(SUCCEEDED(hr));

			// Check for ray tracing support
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};
			hr = m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features));
			assert(SUCCEEDED(hr));
			if (features.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
			{
				m_d3dDevice.Reset();
				OutputDebugString(L"ERROR: Failed to find DXR capable HW");
				exit(-1);
			}

			// Check for SM6 support
			D3D12_FEATURE_DATA_SHADER_MODEL shaderModelSupport{ D3D_SHADER_MODEL_6_3 };
			if (FAILED(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelSupport, sizeof(shaderModelSupport))))
			{
				m_d3dDevice.Reset();
				OutputDebugString(L"ERROR: Failed to find SM6_3 support");
				exit(-1);
			}

			out += L" ... OK\n";
			OutputDebugString(out.c_str());

			deviceCreated = true;
			break;
		}
		else
		{
			out += L" ... SKIP\n";
			OutputDebugString(out.c_str());
		}
	}

	// Cached descriptor size
	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplerDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_pixCapture.GetAddressOf()));
	m_pixAttached = SUCCEEDED(hr);
}

void App::InitCommandObjects()
{
	// Command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK(m_d3dDevice->CreateCommandQueue(
		&cmdQueueDesc,
		IID_PPV_ARGS(m_cmdQueue.GetAddressOf())
	));

	// Command Allocator
	for (auto n = 0; n < k_gfxBufferCount; ++n)
	{
		CHECK(m_d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_gfxCmdAllocators.at(n).GetAddressOf())
		));
	}

	// Command List
	CHECK(m_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(),
		nullptr,
		IID_PPV_ARGS(m_gfxCmdList.GetAddressOf())
	));

	// Fence
	CHECK(m_d3dDevice->CreateFence(m_gfxFenceValues.at(m_gfxBufferIndex), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_gfxFence.GetAddressOf())));
	m_gfxFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

void App::InitSwapChain(HWND windowHandle)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = k_screenWidth;
	swapChainDesc.Height = k_screenHeight;
	swapChainDesc.Format = k_backBufferFormat;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = k_gfxBufferCount;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(),
		windowHandle,
		&swapChainDesc,
		nullptr, 
		nullptr,
		swapChain.GetAddressOf()
	);
	assert(SUCCEEDED(hr));

	hr = swapChain->QueryInterface(IID_PPV_ARGS(m_swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

	for (auto bufferIdx = 0; bufferIdx < k_gfxBufferCount; bufferIdx++)
	{
		hr = m_swapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(m_swapChainBuffers.at(bufferIdx).GetAddressOf()));
		assert(SUCCEEDED(hr));

		const D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = GetRtvDescriptorCPU(static_cast<RTV::Id>(RTV::Id::SwapChain + bufferIdx));
		m_d3dDevice->CreateRenderTargetView(m_swapChainBuffers.at(bufferIdx).Get(), nullptr, rtvDescriptor);
	}

	m_gfxBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void App::InitSurfaces()
{
	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// UAVs
	{
		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC dxrOutResourceDesc{};
		dxrOutResourceDesc.DepthOrArraySize = 1;
		dxrOutResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dxrOutResourceDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		dxrOutResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		dxrOutResourceDesc.Width = k_screenWidth;
		dxrOutResourceDesc.Height = k_screenHeight;
		dxrOutResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		dxrOutResourceDesc.MipLevels = 1;
		dxrOutResourceDesc.SampleDesc.Count = 1;

		HRESULT hr = m_d3dDevice->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&dxrOutResourceDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			nullptr,
			IID_PPV_ARGS(m_dxrOutput.GetAddressOf())
		);
		assert(SUCCEEDED(hr));

		m_dxrOutput->SetName(L"output_uav");


		D3D12_UNORDERED_ACCESS_VIEW_DESC dxrOutUavDesc = {};
		dxrOutUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		m_d3dDevice->CreateUnorderedAccessView(
			m_dxrOutput.Get(),
			nullptr, 
			&dxrOutUavDesc,
			GetSrvUavDescriptorCPU(SrvUav::DxrOutputUAV)
		);
	}
}

void App::InitRaytracePipelines()
{
	StackAllocator stackAlloc{ 4096 };

	// Add any new materials to the list below
	std::vector<D3D12_STATE_SUBOBJECT> pipelineSubObjects;
	size_t payloadIndex{};

	auto rtPipeline = std::make_unique<RaytraceMaterialPipeline>(stackAlloc, pipelineSubObjects, payloadIndex, m_d3dDevice.Get());
	rtPipeline->BuildFromMaterial<DefaultOpaqueMaterial>(stackAlloc, pipelineSubObjects, payloadIndex, m_d3dDevice.Get());
	rtPipeline->BuildFromMaterial<DefaultMaskedMaterial>(stackAlloc, pipelineSubObjects, payloadIndex, m_d3dDevice.Get());
	rtPipeline->BuildFromMaterial<UntexturedMaterial>(stackAlloc, pipelineSubObjects, payloadIndex, m_d3dDevice.Get());

	// Finalize and build
	rtPipeline->Commit(stackAlloc, pipelineSubObjects, m_d3dDevice.Get());

	m_raytracePipeline = std::move(rtPipeline);
}

void App::InitDescriptorHeaps()
{
	// srv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
	cbvSrvUavHeapDesc.NumDescriptors = SrvUav::Count;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_d3dDevice->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(m_cbvSrvUavHeap.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// rtv heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void App::InitUploadBuffer()
{
	m_uploadBuffer.Init(m_d3dDevice.Get(), k_uploadBufferSize);
}

void App::InitResourceHeaps()
{
	m_geometryDataHeap.Init(m_d3dDevice.Get(), k_geometryDataSize);
	m_materialConstantsHeap.Init(m_d3dDevice.Get(), k_materialConstantsSize);
}

void App::InitScene()
{
	ResourceHeap scratchHeap;
	scratchHeap.Init(m_d3dDevice.Get(), k_scratchDataSize);

	m_scene.InitResources(
		m_d3dDevice.Get(), 
		m_cmdQueue.Get(), 
		m_gfxCmdList.Get(), 
		&m_uploadBuffer,
		&scratchHeap,
		&m_geometryDataHeap,
		&m_materialConstantsHeap,
		m_cbvSrvUavHeap.Get(),
		m_cbvSrvUavDescriptorSize
	);

	// make sure all resources have finished copying
	m_uploadBuffer.Flush();
}

void App::InitView()
{
	m_view.Init(m_d3dDevice.Get(), k_gfxBufferCount, k_screenWidth, k_screenHeight);
}

void App::Init(HWND windowHandle)
{
	InitBaseD3D();
	InitCommandObjects();
	InitDescriptorHeaps();
	InitSwapChain(windowHandle);
	InitSurfaces();
	InitUploadBuffer();
	InitResourceHeaps();
	InitScene();
	InitView();
	InitRaytracePipelines();

	// Finalize init and flush 
	m_gfxCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_gfxCmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(std::extent<decltype(cmdLists)>::value, cmdLists);
	FlushCmdQueue();
}

void App::Destroy()
{
	FlushCmdQueue();
	CloseHandle(m_gfxFenceEvent);
}

void App::Update(float dt)
{
	POINT mouseDelta = { m_currentMousePos.x - m_lastMousePos.x, m_currentMousePos.y - m_lastMousePos.y };
	m_lastMousePos = m_currentMousePos;

	m_view.Update(dt, m_buttonState,  mouseDelta);
	m_scene.Update(dt);
}

void App::Render()
{
	if (m_pixAttached)
	{
		m_pixCapture->BeginCapture();
	}

	// Reset command list
	m_gfxCmdAllocators.at(m_gfxBufferIndex)->Reset();
	m_gfxCmdList->Reset(m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(), nullptr);

	{
		PIXScopedEvent(m_gfxCmdList.Get(), 0, L"render_frame");

		// Set descriptor heaps
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvUavHeap.Get() };
		m_gfxCmdList->SetDescriptorHeaps(std::extent<decltype(descriptorHeaps)>::value, descriptorHeaps);

		D3D12_RESOURCE_BARRIER barriers[2] = {};

		// Transition back buffer from present to copy dest
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = m_swapChainBuffers.at(m_gfxBufferIndex).Get();
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// Transition the DXR output buffer from copy source to UAV
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = m_dxrOutput.Get();
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		m_gfxCmdList->ResourceBarrier(2, barriers);

		{
			PIXScopedEvent(m_gfxCmdList.Get(), 0, L"scene");
			m_scene.Render(
				m_d3dDevice.Get(), 
				m_gfxCmdList.Get(), 
				m_gfxBufferIndex, 
				m_view, 
				m_raytracePipeline.get(), 
				GetSrvUavDescriptorGPU(SrvUav::DxrOutputUAV)
			);
		}

		// Transition DXR output from UAV to copy source
		D3D12_RESOURCE_BARRIER uavToCopyBarrier = {};
		uavToCopyBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		uavToCopyBarrier.Transition.pResource = m_dxrOutput.Get();
		uavToCopyBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		uavToCopyBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		uavToCopyBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(1, &uavToCopyBarrier);

		// Blit to back buffer
		m_gfxCmdList->CopyResource(
			m_swapChainBuffers.at(m_gfxBufferIndex).Get(),
			m_dxrOutput.Get()
		);

		// Transition back buffer from copy dest to present
		D3D12_RESOURCE_BARRIER copyToPresentBarrier = {};
		copyToPresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		copyToPresentBarrier.Transition.pResource = m_swapChainBuffers.at(m_gfxBufferIndex).Get();
		copyToPresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		copyToPresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		copyToPresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(1, &copyToPresentBarrier);
	}

	// Execute
	m_gfxCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_gfxCmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(std::extent<decltype(cmdLists)>::value, cmdLists);

	// Present
	{
		PIXScopedEvent(m_cmdQueue.Get(), 0, L"present");
		HRESULT hr = m_swapChain->Present(1, 0);

		if (m_pixAttached)
		{
			m_pixCapture->EndCapture();
		}

		if (FAILED(hr))
		{
			hr = m_d3dDevice->GetDeviceRemovedReason();
			assert(SUCCEEDED(hr));
		}

	}

	// Swap buffers
	AdvanceGfxFrame();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetSrvUavDescriptorCPU(SrvUav::Id srvId) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr + srvId * m_cbvSrvUavDescriptorSize;
	return hnd;
}

D3D12_GPU_DESCRIPTOR_HANDLE App::GetSrvUavDescriptorGPU(SrvUav::Id srvId) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr + srvId * m_cbvSrvUavDescriptorSize;
	return hnd;
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetRtvDescriptorCPU(RTV::Id rtvId) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + rtvId * m_rtvDescriptorSize;
	return hnd;
}

D3D12_GPU_DESCRIPTOR_HANDLE App::GetRtvDescriptorGPU(RTV::Id rtvId) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_rtvHeap->GetGPUDescriptorHandleForHeapStart().ptr + rtvId * m_rtvDescriptorSize;
	return hnd;
}

void App::SubmitCommands()
{
	m_gfxCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_gfxCmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(std::extent<decltype(cmdLists)>::value, cmdLists);
	m_gfxCmdList->Reset(m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(), nullptr);
}

void App::FlushCmdQueue()
{
	PIXScopedEvent(m_cmdQueue.Get(), 0, L"gfx_wait_for_GPU");
	auto& currentFenceValue = m_gfxFenceValues.at(m_gfxBufferIndex);
	currentFenceValue++;

	m_cmdQueue->Signal(m_gfxFence.Get(), currentFenceValue);
	m_gfxFence->SetEventOnCompletion(currentFenceValue, m_gfxFenceEvent);
	WaitForSingleObject(m_gfxFenceEvent, INFINITE);
}

void App::AdvanceGfxFrame()
{
	// Signal current frame is done
	auto currentFenceValue = m_gfxFenceValues.at(m_gfxBufferIndex);
	m_cmdQueue->Signal(m_gfxFence.Get(), currentFenceValue);

	// Cycle to next buffer index
	m_gfxBufferIndex = m_swapChain->GetCurrentBackBufferIndex();;

	// If the buffer that was swapped in hasn't finished rendering on the GPU (from a previous submit), then wait!
	if (m_gfxFence->GetCompletedValue() < m_gfxFenceValues.at(m_gfxBufferIndex))
	{
		PIXBeginEvent(0, L"gfx_wait_on_previous_frame");

		HRESULT hr = m_gfxFence->SetEventOnCompletion(m_gfxFenceValues.at(m_gfxBufferIndex), m_gfxFenceEvent);
		assert(SUCCEEDED(hr));

		WaitForSingleObjectEx(m_gfxFenceEvent, INFINITE, FALSE);
	}

	// Update fence value for the next frame
	m_gfxFenceValues[m_gfxBufferIndex] = currentFenceValue + 1;

	// We now know that this is not being used by the GPU. So, update any render resources!
	m_view.UpdateRenderResources(m_gfxBufferIndex);
	m_scene.UpdateRenderResources(m_gfxBufferIndex);
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
	m_buttonState = btnState;
	m_currentMousePos = { x, y };
}
