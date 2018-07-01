#include "stdafx.h"
#include "DebugDraw.h"
#include "App.h"

constexpr auto k_maxDebugVerts = 10000u;
constexpr auto k_maxDebugIndices = 10000u;

constexpr uint32_t GetVertexBufferSize()
{
	return k_maxDebugVerts * sizeof(DebugLineMesh::VertexType);
}

constexpr uint32_t GetIndexBufferSize()
{
	return k_maxDebugIndices * sizeof(DebugLineMesh::IndexType);
}

void DebugLineMesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// CPU accessible
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// vertex buffer
	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = GetVertexBufferSize() * k_gfxBufferCount;
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	CHECK(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())
	));

	// Get mapped ptr
	auto** ptr = reinterpret_cast<void**>(&m_vbStartPtr);
	m_vertexBuffer->Map(0, nullptr, ptr);

	// index buffer
	D3D12_RESOURCE_DESC ibDesc = {};
	ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ibDesc.Width = GetIndexBufferSize() * k_gfxBufferCount;
	ibDesc.Height = 1;
	ibDesc.DepthOrArraySize = 1;
	ibDesc.MipLevels = 1;
	ibDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	ibDesc.SampleDesc.Count = 1;
	ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	CHECK(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())
	));

	// Get mapped ptr
	ptr = reinterpret_cast<void**>(&m_ibStartPtr);
	m_indexBuffer->Map(0, nullptr, ptr);

	// initialize current ptrs
	m_vbCurrentPtr = m_vbStartPtr;
	m_ibCurrentPtr = m_ibStartPtr;

	m_numVerts = 0;
	m_numIndices = 0;
}

void DebugLineMesh::AddBox(const DirectX::BoundingBox& box, const DirectX::XMFLOAT3 color)
{
	std::vector<DirectX::XMFLOAT3> boxCorners;
	boxCorners.resize(8);
	box.GetCorners(boxCorners.data());

	// add verts
	for (const auto& vert : boxCorners)
	{
		*m_vbCurrentPtr++ = { vert, color };
	}

	// add indices
	uint32_t indexStride = m_numVerts;
	*m_ibCurrentPtr++ = indexStride + 0; *m_ibCurrentPtr++ = indexStride + 1;
	*m_ibCurrentPtr++ = indexStride + 1; *m_ibCurrentPtr++ = indexStride + 2;
	*m_ibCurrentPtr++ = indexStride + 2; *m_ibCurrentPtr++ = indexStride + 3;
	*m_ibCurrentPtr++ = indexStride + 3; *m_ibCurrentPtr++ = indexStride + 0;
	*m_ibCurrentPtr++ = indexStride + 4; *m_ibCurrentPtr++ = indexStride + 5;
	*m_ibCurrentPtr++ = indexStride + 5; *m_ibCurrentPtr++ = indexStride + 6;
	*m_ibCurrentPtr++ = indexStride + 6; *m_ibCurrentPtr++ = indexStride + 7;
	*m_ibCurrentPtr++ = indexStride + 7; *m_ibCurrentPtr++ = indexStride + 4;
	*m_ibCurrentPtr++ = indexStride + 0; *m_ibCurrentPtr++ = indexStride + 4;
	*m_ibCurrentPtr++ = indexStride + 1; *m_ibCurrentPtr++ = indexStride + 5;
	*m_ibCurrentPtr++ = indexStride + 2; *m_ibCurrentPtr++ = indexStride + 6;
	*m_ibCurrentPtr++ = indexStride + 3; *m_ibCurrentPtr++ = indexStride + 7;

	m_numVerts += boxCorners.size();
	m_numIndices += 24;

}

void DebugLineMesh::Render(ID3D12GraphicsCommandList* cmdList, const uint32_t bufferIndex)
{
	// VB descriptor
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress() + bufferIndex * GetVertexBufferSize();
	vertexBufferView.StrideInBytes = sizeof(VertexType);
	vertexBufferView.SizeInBytes = m_numVerts * sizeof(VertexType);

	// IB descriptor
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress() + bufferIndex * GetIndexBufferSize();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = m_numIndices * sizeof(IndexType);

	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	cmdList->IASetIndexBuffer(&indexBufferView);
	cmdList->DrawIndexedInstanced(m_numIndices, 1, 0, 0, 0);
}

void DebugLineMesh::UpdateRenderResources(uint32_t bufferIndex)
{
	m_vbCurrentPtr = m_vbStartPtr + bufferIndex * k_maxDebugVerts;
	m_ibCurrentPtr = m_ibStartPtr + bufferIndex * k_maxDebugVerts;

	m_numVerts = 0;
	m_numIndices = 0;
}