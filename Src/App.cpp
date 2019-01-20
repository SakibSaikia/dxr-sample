#include "stdafx.h"
#include "App.h"

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
			CHECK(D3D12CreateDevice(
				adapter.Get(),
				D3D_FEATURE_LEVEL_12_1,
				IID_PPV_ARGS(m_d3dDevice.GetAddressOf())
			));

			// Check for ray tracing support
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};
			CHECK(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features)));
			if (features.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
			{
				m_d3dDevice.Reset();
				OutputDebugString(L"ERROR: Failed to find DXR capable HW");
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

	CHECK(m_dxgiFactory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(),
		windowHandle,
		&swapChainDesc,
		nullptr, 
		nullptr,
		m_swapChain.GetAddressOf()
	));
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

		CHECK(m_d3dDevice->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&dxrOutResourceDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(m_dxrOutput.GetAddressOf()
		));


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
	for (int pass = 0; pass < RenderPass::Count; pass++)
	{
		// Holds temp resources such as blobs and root signatures until the pipeline is committed
		std::vector<IUnknown*> pendingResources;

		// Add any new materials to the list below
		auto rtPipeline = std::make_unique<RaytraceMaterialPipeline>(m_d3dDevice.Get(), pass, pendingResources);
		rtPipeline->BuildFromMaterial(m_d3dDevice.Get(), L"DefaultOpaue",  DefaultOpaqueMaterial::GetRaytracePipelineDesc(pass), pendingResources);
		rtPipeline->BuildFromMaterial(m_d3dDevice.Get(), L"DefaultMasked", DefaultMaskedMaterial::GetRaytracePipelineDesc(pass), pendingResources);
		rtPipeline->BuildFromMaterial(m_d3dDevice.Get(), L"Untextured", UntexturedMaterial::GetRaytracePipelineDesc(pass), pendingResources);

		// Finalize and build
		rtPipeline->Commit(m_d3dDevice.Get(), pendingResources);

		m_raytracePipelines[pass] = std::move(rtPipeline);
	}
}

void App::InitDescriptorHeaps()
{
	// srv heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
	cbvSrvUavHeapDesc.NumDescriptors = SrvUav::Count;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK(m_d3dDevice->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(m_cbvSrvUavHeap.GetAddressOf())));
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
	InitSwapChain(windowHandle);
	InitDescriptorHeaps();
	InitSurfaces();
	InitUploadBuffer();
	InitResourceHeaps();
	InitScene();
	InitView();

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
			m_scene.Render(RenderPass::Geometry, m_d3dDevice.Get(), m_gfxCmdList.Get(), m_gfxBufferIndex, m_view, GetSrvUavDescriptorGPU(SrvUav::RenderSurfaceBegin));
		}

		// Transition DXR output from UAV to copy source
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = m_dxrOutput.Get();
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(1, &barriers[1]);

		// Blit to back buffer
		m_gfxCmdList->CopyResource(
			m_swapChainBuffers.at(m_gfxBufferIndex).Get(),
			m_dxrOutput.Get()
		);

		// Transition back buffer from copy dest to present
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = m_swapChainBuffers.at(m_gfxBufferIndex).Get();
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(1, &barriers[0]);
	}

	// Execute
	m_gfxCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_gfxCmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(std::extent<decltype(cmdLists)>::value, cmdLists);

	// Present
	{
		PIXScopedEvent(m_cmdQueue.Get(), 0, L"present");
		m_swapChain->Present(1, 0);
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
	m_gfxBufferIndex = (m_gfxBufferIndex + 1) % k_gfxBufferCount;

	// If the buffer that was swapped in hasn't finished rendering on the GPU (from a previous submit), then wait!
	if (m_gfxFence->GetCompletedValue() < m_gfxFenceValues.at(m_gfxBufferIndex))
	{
		PIXBeginEvent(0, L"gfx_wait_on_previous_frame");
		m_gfxFence->SetEventOnCompletion(m_gfxFenceValues.at(m_gfxBufferIndex), m_gfxFenceEvent);
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
