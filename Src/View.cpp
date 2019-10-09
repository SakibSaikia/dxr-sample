#include "stdafx.h"
#include "Common.h"
#include "View.h"

View::~View()
{
	m_cbuffer->Unmap(0, nullptr);
}

void View::Init(ID3D12Device5* device, size_t bufferCount, const size_t width, const size_t height)
{
	float aspectRatio = static_cast<float>(width) / height;
	m_camera.Init(0.25f * DirectX::XM_PI, aspectRatio);

	// Constant Buffer
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(ViewConstants) * k_gfxBufferCount; // n copies where n = buffer count
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		CHECK(device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_cbuffer.GetAddressOf())
		));

		// Get ptr to mapped resource
		auto** ptr = reinterpret_cast<void**>(&m_cbufferPtr);
		m_cbuffer->Map(0, nullptr, ptr);
	}
}

void View::Update(const float dt, const WPARAM mouseBtnState, const POINT mouseDelta)
{
	m_camera.Update(dt, mouseBtnState, mouseDelta);
}

void View::UpdateRenderResources(const uint32_t bufferIndex)
{
	// Update constant buffer
	ViewConstants* ptr = m_cbufferPtr + bufferIndex;
	ptr->viewMatrix = m_camera.GetViewMatrix();
	ptr->fovScale = m_camera.GetFovScale();
}

ID3D12Resource* View::GetConstantBuffer() const
{
	return m_cbuffer.Get();
}