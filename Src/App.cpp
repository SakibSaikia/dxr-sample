#include "stdafx.h"
#include "App.h"

std::unordered_map<MaterialType, std::unique_ptr<MaterialPipeline>> k_materialPipelines;

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

	// Enable SM6
	static const GUID D3D12ExperimentalShaderModelsID = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
		0x76f5573e,
		0xf13a,
		0x40f5,
		{ 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
	};

	D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr);

	// Device
	CHECK(D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(m_d3dDevice.GetAddressOf())
	));

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
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	CHECK(m_dxgiFactory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(),
		windowHandle,
		&swapChainDesc,
		nullptr, 
		nullptr,
		m_swapChain.GetAddressOf()
	));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = k_gfxBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	CHECK(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = k_gfxBufferCount;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	CHECK(m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));

	// RTVs
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (auto bufferIdx = 0; bufferIdx < k_gfxBufferCount; bufferIdx++)
		{
			CHECK(m_swapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(m_swapChainBuffers.at(bufferIdx).GetAddressOf())));

			m_d3dDevice->CreateRenderTargetView(m_swapChainBuffers.at(bufferIdx).Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.ptr += m_rtvDescriptorSize;
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

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		for (auto bufferIdx = 0; bufferIdx < k_gfxBufferCount; bufferIdx++)
		{
			CHECK(m_d3dDevice->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&zClear,
				IID_PPV_ARGS(m_depthStencilBuffers.at(bufferIdx).GetAddressOf())
			));

			// Descriptor to mip level 0
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = k_depthStencilFormatDsv;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // Modify if you want a read-only DSV
			m_d3dDevice->CreateDepthStencilView(
				m_depthStencilBuffers.at(bufferIdx).Get(),
				&dsvDesc,
				dsvHeapHandle
			);

			dsvHeapHandle.ptr += m_dsvDescriptorSize;
		}
	}
}

void App::InitDescriptors()
{
	// CBV/SRV/UAV heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
		cbvSrvUavHeapDesc.NumDescriptors = k_cbvCount + k_srvCount;
		cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		CHECK(m_d3dDevice->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(m_cbvSrvUavHeap.GetAddressOf())));
	}
}

void App::InitScene()
{
	m_scene.InitResources(
		m_d3dDevice.Get(), 
		m_cmdQueue.Get(), 
		m_gfxCmdList.Get(), 
		m_cbvSrvUavHeap.Get(),
		k_cbvCount,
		m_cbvSrvUavDescriptorSize,
		m_basePassPSODesc
	);
}

void App::InitView()
{
	m_view.Init(m_d3dDevice.Get(), k_gfxBufferCount, k_screenWidth, k_screenHeight);
}

void App::InitMaterialPipelines()
{
	for (const auto& pipelineDesc : k_supportedMaterialPipelineDescriptions)
	{
		auto newMaterialPipeline = std::make_unique<MaterialPipeline>(pipelineDesc, m_d3dDevice.Get(), m_basePassPSODesc);
		k_materialPipelines.emplace(pipelineDesc.type, std::move(newMaterialPipeline));
	}
}

void App::InitStateObjects()
{
	// PSO
	{
		// input layout
		m_basePassPSODesc.InputLayout.pInputElementDescs = StaticMesh::VertexType::InputLayout::s_desc;
		m_basePassPSODesc.InputLayout.NumElements = StaticMesh::VertexType::InputLayout::s_num;

		// rasterizer state
		m_basePassPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		m_basePassPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		m_basePassPSODesc.RasterizerState.FrontCounterClockwise = FALSE;
		m_basePassPSODesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		m_basePassPSODesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		m_basePassPSODesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		m_basePassPSODesc.RasterizerState.DepthClipEnable = TRUE;
		m_basePassPSODesc.RasterizerState.MultisampleEnable = FALSE;
		m_basePassPSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
		m_basePassPSODesc.RasterizerState.ForcedSampleCount = 0;
		m_basePassPSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// blend state
		m_basePassPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
		m_basePassPSODesc.BlendState.IndependentBlendEnable = FALSE;
		for (auto& rt : m_basePassPSODesc.BlendState.RenderTarget)
		{
			rt.BlendEnable = FALSE;
			rt.LogicOpEnable = FALSE;
			rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		// depth stencil state
		m_basePassPSODesc.DepthStencilState.DepthEnable = TRUE;
		m_basePassPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		m_basePassPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		m_basePassPSODesc.DepthStencilState.StencilEnable = FALSE;

		// RTV
		m_basePassPSODesc.NumRenderTargets = 1;
		m_basePassPSODesc.RTVFormats[0] = k_backBufferRTVFormat;
		m_basePassPSODesc.DSVFormat = k_depthStencilFormatDsv;

		// misc
		m_basePassPSODesc.SampleMask = UINT_MAX;
		m_basePassPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_basePassPSODesc.SampleDesc.Count = 1;
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
}

void App::Init(HWND windowHandle)
{
	InitBaseD3D();
	InitCommandObjects();
	InitSwapChain(windowHandle);
	InitStateObjects();
	InitMaterialPipelines();
	InitDescriptors();
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

	m_view.Update(dt, mouseDelta, m_gfxBufferIndex);

	m_scene.Update(m_gfxBufferIndex);
}

void App::Render()
{
	// Reset command list
	m_gfxCmdAllocators.at(m_gfxBufferIndex)->Reset();
	m_gfxCmdList->Reset(m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(), nullptr);

	{
		PIXScopedEvent(m_gfxCmdList.Get(), 0, L"render_frame");

		// Transition back buffer from present to render target
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_swapChainBuffers.at(m_gfxBufferIndex).Get();
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(
			1,
			&barrierDesc
		);

		// Clear
		float clearColor[] = { .8f, .8f, 1.f, 0.f };
		m_gfxCmdList->ClearRenderTargetView(GetCurrentBackBufferView(), clearColor, 0, nullptr);
		m_gfxCmdList->ClearDepthStencilView(GetCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

		// Set descriptor heaps
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvUavHeap.Get() };
		m_gfxCmdList->SetDescriptorHeaps(std::extent<decltype(descriptorHeaps)>::value, descriptorHeaps);

		// Set state
		m_gfxCmdList->RSSetViewports(1, &m_viewport);
		m_gfxCmdList->RSSetScissorRects(1, &m_scissorRect);

		// Set rendertarget
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetCurrentDepthStencilView();
		m_gfxCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Render scene
		m_gfxCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_scene.Render(m_gfxCmdList.Get(), m_gfxBufferIndex, m_view);

		// Transition back buffer from render target to present
		barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_swapChainBuffers.at(m_gfxBufferIndex).Get();
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_gfxCmdList->ResourceBarrier(
			1,
			&barrierDesc
		);
	}

	// Execute
	m_gfxCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_gfxCmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(std::extent<decltype(cmdLists)>::value, cmdLists);

	// Present
	{
		PIXScopedEvent(m_cmdQueue.Get(), 0, L"present");
		m_swapChain->Present(0, 0);
	}

	// Swap buffers
	AdvanceGfxFrame();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetCurrentBackBufferView() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_gfxBufferIndex * m_rtvDescriptorSize;
	return hnd;
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetCurrentDepthStencilView() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_gfxBufferIndex * m_dsvDescriptorSize;
	return hnd;
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetConstantBufferDescriptorCPU(ConstantBufferId id, const uint32_t offset) const
{
	uint32_t descriptorOffset = static_cast<uint32_t>(id) + offset;

	D3D12_CPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart().ptr + descriptorOffset * m_cbvSrvUavDescriptorSize;
	return hnd;
}

D3D12_GPU_DESCRIPTOR_HANDLE App::GetConstantBufferDescriptorGPU(ConstantBufferId id, const uint32_t offset) const
{
	uint32_t descriptorOffset = static_cast<uint32_t>(id) + offset;

	D3D12_GPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr + descriptorOffset * m_cbvSrvUavDescriptorSize;
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
	auto currentFenceValue = m_gfxFenceValues.at(m_gfxBufferIndex);
	m_cmdQueue->Signal(m_gfxFence.Get(), currentFenceValue);

	m_gfxBufferIndex = (m_gfxBufferIndex + 1) % k_gfxBufferCount;

	if (m_gfxFence->GetCompletedValue() < m_gfxFenceValues.at(m_gfxBufferIndex))
	{
		PIXBeginEvent(0, L"gfx_wait_on_previous_frame");
		m_gfxFence->SetEventOnCompletion(m_gfxFenceValues.at(m_gfxBufferIndex), m_gfxFenceEvent);
		WaitForSingleObjectEx(m_gfxFenceEvent, INFINITE, FALSE);
	}

	m_gfxFenceValues[m_gfxBufferIndex] = currentFenceValue + 1;
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
	m_currentMousePos = { x, y };
}
