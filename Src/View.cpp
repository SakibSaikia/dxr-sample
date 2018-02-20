#include "stdafx.h"
#include "View.h"

View::~View()
{
	for (auto cbuffer : m_cbuffers)
	{
		cbuffer->Unmap(0, nullptr);
	}
}

void View::Init(ID3D12Device* device, size_t bufferCount, const size_t width, const size_t height)
{
	float aspectRatio = static_cast<float>(width) / height;
	m_camera.Init(aspectRatio);

	m_cbuffers.reserve(bufferCount);
	m_cbufferPtrs.reserve(bufferCount);

	// Constant Buffer
	{
		uint32_t viewConstantBufferSize = (sizeof(ViewConstants) + 0xff) & ~0xff; // 256 byte aligned

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
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		for (auto n = 0; n < bufferCount; ++n)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> buf;
			HRESULT hr = device->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(buf.GetAddressOf())
			);
			assert(hr == S_OK && L"Failed to create constant buffer");
			m_cbuffers.push_back(buf);

			// Get ptr to mapped resource
			ViewConstants* bufPtr;
			auto** ptr = reinterpret_cast<void**>(&bufPtr);
			buf->Map(0, nullptr, ptr);
			m_cbufferPtrs.push_back(bufPtr);
		}
	}
}

void View::Update(const float dt, const POINT mouseDelta, const uint32_t bufferIndex)
{
	// Update camera
	m_camera.Update(dt, mouseDelta);

	// Update constant buffer
	m_cbufferPtrs.at(bufferIndex)->viewMatrix = m_camera.GetViewMatrix();
	m_cbufferPtrs.at(bufferIndex)->viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
}

ID3D12Resource* View::GetConstantBuffer(uint32_t index) const
{
	return m_cbuffers.at(index).Get();
}