#include "stdafx.h"
#include "StaticMesh.h"
#include "App.h"

void StaticMesh::Init(
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList* cmdList, 
	UploadBuffer* uploadBuffer, 
	ResourceHeap* resourceHeap,
	std::vector<VertexType> vertexData, 
	std::vector<IndexType> indexData, 
	const uint32_t matIndex)
{
	m_numIndices = indexData.size();
	m_materialIndex = matIndex;

	CreateVertexBuffer(device, cmdList, uploadBuffer, resourceHeap, vertexData);
	CreateIndexBuffer(device, cmdList, uploadBuffer, resourceHeap, indexData);
	
}

void StaticMesh::CreateVertexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<VertexType>& vertexData)
{
	// vertex buffer
	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = vertexData.size() * sizeof(VertexType);
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	size_t offsetInHeap = resourceHeap->GetAlloc(vbDesc.Width);

	CHECK(device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&vbDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())
	));

	// copy vertex data to upload buffer
	uint64_t vbSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT vbLayout;
	device->GetCopyableFootprints(
		&vbDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&vbLayout, nullptr, &vbSizeInBytes, nullptr);

	auto[destVbPtr, vbOffset] = uploadBuffer->GetAlloc(vbSizeInBytes);
	const auto* pSrc = reinterpret_cast<const uint8_t*>(vertexData.data());
	memcpy(destVbPtr, pSrc, vbSizeInBytes);

	// schedule copy to default vertex buffer
	cmdList->CopyBufferRegion(
		m_vertexBuffer.Get(),
		0,
		uploadBuffer->GetResource(),
		vbOffset,
		vbLayout.Footprint.Width
	);

	// transition vertex buffer
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

	// VB descriptor
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(VertexType);
	m_vertexBufferView.SizeInBytes = vertexData.size() * sizeof(VertexType);
}

void StaticMesh::CreateIndexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<IndexType>& indexData)
{
	// index buffer
	D3D12_RESOURCE_DESC ibDesc = {};
	ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ibDesc.Width = indexData.size() * sizeof(IndexType);
	ibDesc.Height = 1;
	ibDesc.DepthOrArraySize = 1;
	ibDesc.MipLevels = 1;
	ibDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	ibDesc.SampleDesc.Count = 1;
	ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	offsetInHeap = resourceHeap->GetAlloc(ibDesc.Width);

	CHECK(device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&ibDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())
	));

	// copy index data to upload buffer
	uint64_t ibSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT ibLayout;
	device->GetCopyableFootprints(
		&ibDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&ibLayout, nullptr, &ibSizeInBytes, nullptr);

	auto[destIbPtr, ibOffset] = uploadBuffer->GetAlloc(ibSizeInBytes);
	pSrc = reinterpret_cast<const uint8_t*>(indexData.data());
	memcpy(destIbPtr, pSrc, ibSizeInBytes);

	// schedule copy to default index buffer
	cmdList->CopyBufferRegion(
		m_indexBuffer.Get(),
		0,
		uploadBuffer->GetResource(),
		ibOffset,
		ibLayout.Footprint.Width
	);

	// transition index buffer
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

	// IB descriptor
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = indexData.size() * sizeof(IndexType);
}

void StaticMesh::CreateBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<VertexType>& vertexData, const std::vector<IndexType>& indexData)
{
	D3D12_RAYTRACING_GEOMETRY_DESC desc{};
	desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	desc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
	desc.Triangles.VertexBuffer.StrideInBytes = m_vertexBufferView.StrideInBytes;
	desc.Triangles.VertexCount = vertexData.size();
	desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	desc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
	desc.Triangles.IndexFormat = m_indexBufferView.Format;
	desc.Triangles.IndexCount = indexData.size();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;


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