#include "App.h"
#include <cassert>

bool App::Init()
{
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

	return true;
}

bool App::Destroy()
{
	return true;
}

void App::Update()
{

}

void App::Render()
{

}