#include "stdafx.h"
#include "StaticMesh.h"
#include "App.h"

void StaticMesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<VertexType> vertexData, std::vector<IndexType> indexData, const uint32_t matIndex)
{
	// bounding box
	const auto data = reinterpret_cast<DirectX::XMFLOAT3*>(vertexData.data());
	DirectX::BoundingBox::CreateFromPoints(m_bounds, vertexData.size(), data, sizeof(VertexType));


	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUpload;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUpload;

	m_numIndices = indexData.size();
	m_materialIndex = matIndex;

	// default vertex buffer
	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = vertexData.size() * sizeof(VertexType);
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	CHECK(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())
	));

	// default index buffer
	D3D12_RESOURCE_DESC ibDesc = {};
	ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ibDesc.Width = indexData.size() * sizeof(IndexType);
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
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())
	));

	// upload vertex buffer
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	CHECK(device->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertexBufferUpload.GetAddressOf())
	));

	// upload index buffer
	CHECK(device->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(indexBufferUpload.GetAddressOf())
	));

	// copy vertex data to upload buffer
	uint64_t vbRowSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT vbLayout;
	device->GetCopyableFootprints(
		&vbDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&vbLayout, nullptr, &vbRowSizeInBytes, nullptr);

	uint8_t* destPtr;
	vertexBufferUpload->Map(0, nullptr, reinterpret_cast<void**>(&destPtr));
	const auto* pSrc = reinterpret_cast<const uint8_t*>(vertexData.data());
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
	pSrc = reinterpret_cast<const uint8_t*>(indexData.data());
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
	m_vertexBufferView.SizeInBytes = vertexData.size() * sizeof(VertexType);

	// IB descriptor
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = indexData.size() * sizeof(IndexType);

	// Flush here so that the upload buffers do not go out of scope
	// TODO(sakib): do not create and destroy upload buffers for each mesh. 
	// Instead have a common upload buffer(s) that is large enough.
	AppInstance()->SubmitCommands();
	AppInstance()->FlushCmdQueue();
}

void StaticMesh::Render(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->DrawIndexedInstanced(m_numIndices, 1, 0, 0, 0);
}

uint32_t StaticMesh::GetMaterialIndex() const
{
	return m_materialIndex;
}

VertexFormat::Type StaticMesh::GetVertexFormat() const
{
	return VertexFormat::Type::P3N3T3B3U2;
}

const DirectX::BoundingBox& StaticMesh::GetBounds() const
{
	return m_bounds;
}

StaticMeshEntity::StaticMeshEntity(std::string&& name, const uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld) :
	m_name(name),
	m_meshIndex(meshIndex), 
	m_localToWorld(localToWorld)
{
}

void StaticMeshEntity::FillConstants(ObjectConstants* objConst) const
{
	objConst->localToWorldMatrix = m_localToWorld;
}

uint64_t StaticMeshEntity::GetMeshIndex() const
{
	return m_meshIndex;
}

DirectX::XMFLOAT4X4 StaticMeshEntity::GetLocalToWorldMatrix() const 
{
	return m_localToWorld;
}

std::string StaticMeshEntity::GetName() const
{
	return m_name;
}