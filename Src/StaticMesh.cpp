#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include "StaticMesh.h"

D3D12_INPUT_ELEMENT_DESC StaticMesh::VertexType::InputLayout::s_desc[StaticMesh::VertexType::InputLayout::s_num] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

StaticMesh::StaticMesh()
{

}

StaticMesh::keep_alive_type StaticMesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUpload;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUpload;

	VertexType cubeVertices[] =
	{
		{ { -1.f, -1.f, -1.f },{ 0.f, 0.f, -1.f } },
		{ { 1.f, -1.f, -1.f },{ 0.f, 0.f, -1.f } },
		{ { 1.f, 1.f, -1.f },{ 0.f, 0.f, -1.f } },
		{ { -1.f, 1.f, -1.f },{ 0.f, 0.f, -1.f } },

		{ { -1.f, -1.f, 1.f },{ 0.f, 0.f, 1.f } },
		{ { 1.f, -1.f, 1.f },{ 0.f, 0.f, 1.f } },
		{ { 1.f, 1.f, 1.f },{ 0.f, 0.f, 1.f } },
		{ { -1.f, 1.f, 1.f },{ 0.f, 0.f, 1.f } },

		{ { -1.f, -1.f, -1.f },{ -1.f, 0.f, 0.f } },
		{ { -1.f, -1.f, 1.f },{ -1.f, 0.f, 0.f } },
		{ { -1.f, 1.f, 1.f },{ -1.f, 0.f, 0.f } },
		{ { -1.f, 1.f, -1.f },{ -1.f, 0.f, 0.f } },

		{ { 1.f, -1.f, -1.f },{ 1.f, 0.f, 0.f } },
		{ { 1.f, -1.f, 1.f },{ 1.f, 0.f, 0.f } },
		{ { 1.f, 1.f, 1.f },{ 1.f, 0.f, 0.f } },
		{ { 1.f, 1.f, -1.f },{ 1.f, 0.f, 0.f } },

		{ { -1.f, -1.f, -1.f },{ 0.f, -1.f, 0.f } },
		{ { 1.f, -1.f, -1.f },{ 0.f, -1.f, 0.f } },
		{ { 1.f, -1.f, 1.f },{ 0.f, -1.f, 0.f } },
		{ { -1.f, -1.f, 1.f },{ 0.f, -1.f, 0.f } },

		{ { -1.f, 1.f, -1.f },{ 0.f, 1.f, 0.f } },
		{ { 1.f, 1.f, -1.f },{ 0.f, 1.f, 0.f } },
		{ { 1.f, 1.f, 1.f },{ 0.f, 1.f, 0.f } },
		{ { -1.f, 1.f, 1.f },{ 0.f, 1.f, 0.f } },
	};

	uint16_t cubeIndices[] =
	{
		0, 3, 2,
		2, 1, 0,
		7, 4, 6,
		6, 4, 5,
		8, 9, 11,
		11, 9, 10,
		13, 12, 15,
		15, 14, 13,
		16, 17, 19,
		19, 17, 18,
		21, 20, 23,
		23, 22, 21
	};

	m_numIndices = std::extent<decltype(cubeIndices)>::value;

	// default vertex buffer
	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = sizeof(cubeVertices);
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	HRESULT hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create default vertex buffer");

	// default index buffer
	D3D12_RESOURCE_DESC ibDesc = {};
	ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ibDesc.Width = sizeof(cubeIndices);
	ibDesc.Height = 1;
	ibDesc.DepthOrArraySize = 1;
	ibDesc.MipLevels = 1;
	ibDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	ibDesc.SampleDesc.Count = 1;
	ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create default index buffer");

	// upload vertex buffer
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	hr = device->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertexBufferUpload.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create upload vertex buffer");

	// upload index buffer
	hr = device->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(indexBufferUpload.GetAddressOf())
	);

	assert(hr == S_OK && L"Failed to create upload vertex buffer");

	// copy vertex data to upload buffer
	uint64_t vbRowSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT vbLayout;
	device->GetCopyableFootprints(
		&vbDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&vbLayout, nullptr, &vbRowSizeInBytes, nullptr);

	uint8_t* destPtr;
	vertexBufferUpload->Map(0, nullptr, reinterpret_cast<void**>(&destPtr));
	const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(cubeVertices);
	memcpy(destPtr, pSrc, vbRowSizeInBytes);
	vertexBufferUpload->Unmap(0, nullptr);

	// copy index data to upload buffer
	uint64_t ibRowSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT ibLayout;
	device->GetCopyableFootprints(
		&ibDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&ibLayout, nullptr, &ibRowSizeInBytes, nullptr);

	indexBufferUpload->Map(0, nullptr, reinterpret_cast<void**>(&destPtr));
	pSrc = reinterpret_cast<const uint8_t*>(cubeIndices);
	memcpy(destPtr, pSrc, ibRowSizeInBytes);
	indexBufferUpload->Unmap(0, nullptr);

	// schedule copy to default vertex buffer
	cmdList->CopyBufferRegion(
		m_vertexBuffer.Get(),
		0,
		vertexBufferUpload.Get(),
		vbLayout.Offset,
		vbLayout.Footprint.Width
	);

	// schedule copy to default index buffer
	cmdList->CopyBufferRegion(
		m_indexBuffer.Get(),
		0,
		indexBufferUpload.Get(),
		ibLayout.Offset,
		ibLayout.Footprint.Width
	);

	// transition default vertex buffer
	D3D12_RESOURCE_BARRIER vbBarrierDesc = {};
	vbBarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	vbBarrierDesc.Transition.pResource = m_vertexBuffer.Get();
	vbBarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	vbBarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	vbBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(
		1,
		&vbBarrierDesc
	);

	// transition default index buffer
	D3D12_RESOURCE_BARRIER ibBarrierDesc = {};
	ibBarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ibBarrierDesc.Transition.pResource = m_indexBuffer.Get();
	ibBarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	ibBarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ibBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(
		1,
		&ibBarrierDesc
	);

	// VB descriptor
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(VertexType);
	m_vertexBufferView.SizeInBytes = sizeof(cubeVertices);

	// IB descriptor
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = sizeof(cubeIndices);

	return std::make_pair(vertexBufferUpload, indexBufferUpload);
}

void StaticMesh::Render(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->DrawIndexedInstanced(m_numIndices, 1, 0, 0, 0);
}