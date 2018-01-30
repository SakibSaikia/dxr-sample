#include "stdafx.h"
#include "App.h"

Microsoft::WRL::ComPtr<ID3DBlob> LoadShaderFromFile(const std::wstring& filename)
{
	std::ifstream fileHandle(filename, std::ios::binary);
	assert(fileHandle.good() && L"Error opening file");

	// file size
	fileHandle.seekg(0, std::ios::end);
	std::ifstream::pos_type size = fileHandle.tellg();
	fileHandle.seekg(0, std::ios::beg);

	// serialize bytecode
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
	HRESULT hr = D3DCreateBlob(size, shaderBlob.GetAddressOf());
	assert(hr == S_OK && L"Failed to create blob");
	fileHandle.read(static_cast<char*>(shaderBlob->GetBufferPointer()), size);

	fileHandle.close();

	return shaderBlob;
}

App* AppInstance()
{
	static App instance;
	return &instance;
}

void App::InitBaseD3D()
{
	HRESULT hr;

	// Debug layer
#if defined(DEBUG) || defined(_DEBUG)
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(m_debugController.GetAddressOf()));
	if (hr == S_OK)
	{
		m_debugController->EnableDebugLayer();
	}
#endif

	// DXGI
	hr = CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.GetAddressOf()));
	assert(hr == S_OK && L"Failed to create DXGI factory");

	// Device
	hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(m_d3dDevice.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create D3D Device");
}

void App::InitCommandObjects()
{
	// Command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT hr = m_d3dDevice->CreateCommandQueue(
		&cmdQueueDesc,
		IID_PPV_ARGS(m_cmdQueue.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create command queue");

	// Command Allocator
	for (auto n = 0; n < k_gfxBufferCount; ++n)
	{
		hr = m_d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_gfxCmdAllocators.at(n).GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create command allocator");
	}

	// Command List
	hr = m_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(),
		nullptr,
		IID_PPV_ARGS(m_gfxCmdList.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create command list");

	// Fence
	hr = m_d3dDevice->CreateFence(m_gfxFenceValues.at(m_gfxBufferIndex), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_gfxFence.GetAddressOf()));
	assert(hr == S_OK && L"Failed to create fence");
	m_gfxFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

void App::InitSwapChain(HWND windowHandle)
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
	scDesc.BufferCount = k_gfxBufferCount;
	scDesc.OutputWindow = windowHandle;
	scDesc.Windowed = TRUE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr = m_dxgiFactory->CreateSwapChain(
		m_cmdQueue.Get(),
		&scDesc,
		m_swapChain.GetAddressOf()
	);

	assert(hr == S_OK && L"Failed to create swap chain");
}

void App::InitDescriptors()
{
	// Descriptor Heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = k_gfxBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT hr = m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create RTV heap");

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = k_gfxBufferCount;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hr = m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create DSV heap");

		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = k_cbvSrvUavDescriptorCount;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = m_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf()));
		assert(hr == S_OK && L"Failed to create CBV heap");
	}

	// Cached descriptor size
	{
		m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_cbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_samplerDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	// RTVs
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (auto bufferIdx = 0; bufferIdx < k_gfxBufferCount; bufferIdx++)
		{
			HRESULT hr = m_swapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(m_swapChainBuffers.at(bufferIdx).GetAddressOf()));
			assert(hr == S_OK && L"Failed to initialize back buffer");

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
			HRESULT hr = m_d3dDevice->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&zClear,
				IID_PPV_ARGS(m_depthStencilBuffers.at(bufferIdx).GetAddressOf())
			);

			assert(hr == S_OK && L"Failed to create DepthStencil buffer");

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

	// View Constant Buffer
	{
		// 256 byte aligned
		uint32_t viewConstantBufferSize = (sizeof(ViewConstants) + 0xff) & ~0xff;

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = viewConstantBufferSize;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD; // must be CPU accessible
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

		for (auto n = 0; n < k_gfxBufferCount; ++n)
		{
			HRESULT hr = m_d3dDevice->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_viewConstantBuffers.at(n).GetAddressOf())
			);
			assert(hr == S_OK && L"Failed to create constant buffer");

			// Get ptr to mapped resource
			auto** ptr = reinterpret_cast<void**>(&m_ViewConstantBufferPtr.at(n));
			m_viewConstantBuffers.at(n)->Map(0, nullptr, ptr);
		}
	}
}

void App::InitShaders()
{
	m_vsByteCode = LoadShaderFromFile(L"CompiledShaders\\VertexShader.cso");
	m_psByteCode = LoadShaderFromFile(L"CompiledShaders\\PixelShader.cso");
}

void App::InitScene()
{
	m_scene.Init(1, m_d3dDevice.Get(), m_cmdQueue.Get(), m_gfxCmdList.Get());
}

void App::InitCamera()
{
	float aspectRatio = static_cast<float>(k_screenWidth) / k_screenHeight;
	m_camera.Init(aspectRatio);
}

void App::InitStateObjects()
{
	// Root signature
	{
		D3D12_ROOT_PARAMETER rootParameters[2];

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		D3D12_ROOT_SIGNATURE_DESC sigDesc;
		sigDesc.NumParameters = std::extent<decltype(rootParameters)>::value;
		sigDesc.pParameters = rootParameters;
		sigDesc.NumStaticSamplers = 0;
		sigDesc.pStaticSamplers = nullptr;
		sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // can be omitted if IA is not used

		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> error = nullptr;
		D3D12SerializeRootSignature(
			&sigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSignature.GetAddressOf(),
			error.GetAddressOf()
		);

		if (error != nullptr)
		{
			::OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		}

		HRESULT hr = m_d3dDevice->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(m_rootSignature.GetAddressOf())
		);

		assert(hr == S_OK && L"Failed to create root signature");
	}

	// Pipeline State
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// input layout
		psoDesc.InputLayout.pInputElementDescs = StaticMesh::VertexType::InputLayout::s_desc;
		psoDesc.InputLayout.NumElements = StaticMesh::VertexType::InputLayout::s_num;

		// root sig
		psoDesc.pRootSignature = m_rootSignature.Get();

		// shaders
		psoDesc.VS.pShaderBytecode = m_vsByteCode.Get()->GetBufferPointer();
		psoDesc.VS.BytecodeLength = m_vsByteCode.Get()->GetBufferSize();
		psoDesc.PS.pShaderBytecode = m_psByteCode.Get()->GetBufferPointer();
		psoDesc.PS.BytecodeLength = m_psByteCode.Get()->GetBufferSize();

		// rasterizer state
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// blend state
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		for (auto& rt : psoDesc.BlendState.RenderTarget)
		{
			rt.BlendEnable = FALSE;
			rt.LogicOpEnable = FALSE;
			rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		// depth stencil state
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.StencilEnable = FALSE;

		// RTV
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = k_backBufferFormat;
		psoDesc.DSVFormat = k_depthStencilFormatDsv;

		// misc
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleDesc.Count = 1;

		HRESULT hr = m_d3dDevice->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pso.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create PSO");
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
	InitDescriptors();
	InitShaders();
	InitScene();
	InitStateObjects();
	InitCamera();

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

	for (auto cbuffer : m_viewConstantBuffers)
	{
		cbuffer->Unmap(0, nullptr);
	}
}

void App::Update(float dt)
{
	POINT mouseDelta = { m_currentMousePos.x - m_lastMousePos.x, m_currentMousePos.y - m_lastMousePos.y };
	m_lastMousePos = m_currentMousePos;
	m_camera.Update(dt, mouseDelta);

	// Update view constant buffer
	m_ViewConstantBufferPtr.at(m_gfxBufferIndex)->viewMatrix = m_camera.GetViewMatrix();
	m_ViewConstantBufferPtr.at(m_gfxBufferIndex)->viewProjectionMatrix = m_camera.GetViewProjectionMatrix();

	// Update scene 
	m_scene.Update(m_gfxBufferIndex);
}

void App::Render()
{
	// Reset command list
	m_gfxCmdAllocators.at(m_gfxBufferIndex)->Reset();
	m_gfxCmdList->Reset(m_gfxCmdAllocators.at(m_gfxBufferIndex).Get(), nullptr);

	{
		PIXScopedEvent(0, L"render_frame");

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
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
		m_gfxCmdList->SetDescriptorHeaps(std::extent<decltype(descriptorHeaps)>::value, descriptorHeaps);

		// Set root sig
		m_gfxCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

		// Set state
		m_gfxCmdList->SetPipelineState(m_pso.Get());
		m_gfxCmdList->RSSetViewports(1, &m_viewport);
		m_gfxCmdList->RSSetScissorRects(1, &m_scissorRect);

		// Set rendertarget
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetCurrentDepthStencilView();
		m_gfxCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Render scene
		m_gfxCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_gfxCmdList->SetGraphicsRootConstantBufferView(0, m_viewConstantBuffers.at(m_gfxBufferIndex)->GetGPUVirtualAddress());
		m_scene.Render(m_gfxCmdList.Get(), m_gfxBufferIndex);

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
		PIXScopedEvent(0, L"present");
		m_swapChain->Present(1, 0);
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
	hnd.ptr = m_cbvHeap->GetCPUDescriptorHandleForHeapStart().ptr + descriptorOffset * m_cbvSrvUavDescriptorSize;
	return hnd;
}

D3D12_GPU_DESCRIPTOR_HANDLE App::GetConstantBufferDescriptorGPU(ConstantBufferId id, const uint32_t offset) const
{
	uint32_t descriptorOffset = static_cast<uint32_t>(id) + offset;

	D3D12_GPU_DESCRIPTOR_HANDLE hnd;
	hnd.ptr = m_cbvHeap->GetGPUDescriptorHandleForHeapStart().ptr + descriptorOffset * m_cbvSrvUavDescriptorSize;
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
	PIXScopedEvent(0, L"gfx_wait_for_GPU");
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
