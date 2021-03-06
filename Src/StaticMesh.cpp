#include "stdafx.h"
#include "StaticMesh.h"
#include "App.h"

void StaticMesh::Init(
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer, 
	ResourceHeap* scratchHeap,
	ResourceHeap* resourceHeap,
	std::vector<VertexType> vertexData, 
	std::vector<IndexType> indexData, 
	const uint32_t matIndex, 
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvOffset, 
	const size_t srvDescriptorSize)
{
	m_numIndices = static_cast<uint32_t>(indexData.size());
	m_materialIndex = matIndex;
	m_meshSRVHandle.ptr = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + srvOffset * srvDescriptorSize;

	CreateVertexBuffer(device, cmdList, uploadBuffer, resourceHeap, vertexData, srvHeap, srvOffset, srvDescriptorSize);
	CreateIndexBuffer(device, cmdList, uploadBuffer, resourceHeap, indexData, srvHeap, srvOffset + 1, srvDescriptorSize);
	CreateBLAS(device, cmdList, scratchHeap, resourceHeap, vertexData.size(), indexData.size());
}

void StaticMesh::CreateVertexBuffer(
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer, 
	ResourceHeap* resourceHeap,
	const std::vector<VertexType>& vertexData,
	ID3D12DescriptorHeap* srvHeap,
	const size_t srvOffset,
	const size_t srvDescriptorSize)
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

	const auto offsetInHeap = resourceHeap->GetAlloc(vbDesc.Width);

	HRESULT hr = device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&vbDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_vertexBuffer.GetAddressOf())
	);

	assert(SUCCEEDED(hr));
	m_vertexBuffer->SetName(L"vertex_buffer");

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
	vbBarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	vbBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(
		1,
		&vbBarrierDesc
	);

	// VB SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC vbSrvDesc{};
	vbSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vbSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	vbSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	vbSrvDesc.Buffer.StructureByteStride = 0;
	vbSrvDesc.Buffer.FirstElement = 0;
	vbSrvDesc.Buffer.NumElements = static_cast<UINT>(vertexData.size()) * sizeof(VertexType) / sizeof(float);
	vbSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHnd;
	cpuHnd.ptr = srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + srvOffset * srvDescriptorSize;

	device->CreateShaderResourceView(m_vertexBuffer.Get(), &vbSrvDesc, cpuHnd);
}

void StaticMesh::CreateIndexBuffer(
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer, 
	ResourceHeap* resourceHeap, 
	const std::vector<IndexType>& indexData, 
	ID3D12DescriptorHeap* srvHeap,
	const size_t srvOffset,
	const size_t srvDescriptorSize)
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

	const auto offsetInHeap = resourceHeap->GetAlloc(ibDesc.Width);

	HRESULT hr = device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&ibDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_indexBuffer.GetAddressOf())
	);

	assert(SUCCEEDED(hr));
	m_indexBuffer->SetName(L"index_buffer");

	// copy index data to upload buffer
	uint64_t ibSizeInBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT ibLayout;
	device->GetCopyableFootprints(
		&ibDesc,
		0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
		&ibLayout, nullptr, &ibSizeInBytes, nullptr);

	auto[destIbPtr, ibOffset] = uploadBuffer->GetAlloc(ibSizeInBytes);
	const auto* pSrc = reinterpret_cast<const uint8_t*>(indexData.data());
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
	ibBarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ibBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(
		1,
		&ibBarrierDesc
	);

	// IB SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC ibSrvDesc{};
	ibSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	ibSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	ibSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	ibSrvDesc.Buffer.StructureByteStride = 0;
	ibSrvDesc.Buffer.FirstElement = 0;
	ibSrvDesc.Buffer.NumElements = static_cast<UINT>(indexData.size()) * sizeof(UINT) / sizeof(float);
	ibSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHnd;
	cpuHnd.ptr = srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + srvOffset * srvDescriptorSize;

	device->CreateShaderResourceView(m_indexBuffer.Get(), &ibSrvDesc, cpuHnd);
}

void StaticMesh::CreateBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap, const size_t numVerts, const size_t numIndices)
{
	// Geometry description for bottom level acceleration structure
	D3D12_RAYTRACING_GEOMETRY_DESC geoDesc{};
	geoDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geoDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
	geoDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexType);
	geoDesc.Triangles.VertexCount = static_cast<UINT>(numVerts);
	geoDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geoDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
	geoDesc.Triangles.IndexFormat = (sizeof(IndexType) == sizeof(uint32_t) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT);
	geoDesc.Triangles.IndexCount = static_cast<UINT>(numIndices);

	// Compute size for bottom level acceleration structure buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs{};
	asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	asInputs.pGeometryDescs = &geoDesc;
	asInputs.NumDescs = 1;
	asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asPrebuildInfo{};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &asPrebuildInfo);

	const size_t alignedScratchSize = (asPrebuildInfo.ScratchDataSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	const size_t alignedBLASBufferSize = (asPrebuildInfo.ResultDataMaxSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;

	// Create scratch buffer
	D3D12_RESOURCE_DESC scratchBufDesc = {};
	scratchBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	scratchBufDesc.Alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	scratchBufDesc.Width = alignedScratchSize;
	scratchBufDesc.Height = 1;
	scratchBufDesc.DepthOrArraySize = 1;
	scratchBufDesc.MipLevels = 1;
	scratchBufDesc.Format = DXGI_FORMAT_UNKNOWN;
	scratchBufDesc.SampleDesc.Count = 1;
	scratchBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	scratchBufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	auto offsetInHeap = scratchHeap->GetAlloc(scratchBufDesc.Width);

	ID3D12Resource* scratchBuffer;
	HRESULT hr = device->CreatePlacedResource(
		scratchHeap->GetHeap(),
		offsetInHeap,
		&scratchBufDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&scratchBuffer)
	);

	assert(SUCCEEDED(hr));
	scratchBuffer->SetName(L"blas_scratch_buffer");

	// Create BLAS buffer
	D3D12_RESOURCE_DESC blasBufDesc = {};
	blasBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	blasBufDesc.Alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	blasBufDesc.Width = alignedBLASBufferSize;
	blasBufDesc.Height = 1;
	blasBufDesc.DepthOrArraySize = 1;
	blasBufDesc.MipLevels = 1;
	blasBufDesc.Format = DXGI_FORMAT_UNKNOWN;
	blasBufDesc.SampleDesc.Count = 1;
	blasBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	blasBufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	offsetInHeap = resourceHeap->GetAlloc(blasBufDesc.Width);

	hr = device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&blasBufDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_blasBuffer.GetAddressOf())
	);

	assert(SUCCEEDED(hr));
	m_blasBuffer->SetName(L"blas_buffer");

	// Now, build the bottom level acceleration structure
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = asInputs;
	buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Insert UAV barrier 
	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_blasBuffer.Get();

	cmdList->ResourceBarrier(1, &uavBarrier);
	
}

uint32_t StaticMesh::GetMaterialIndex() const
{
	return m_materialIndex;
}

VertexFormat::Type StaticMesh::GetVertexFormat() const
{
	return VertexFormat::Type::P3N3T3B3U2;
}

const D3D12_GPU_VIRTUAL_ADDRESS StaticMesh::GetBLASAddress() const
{
	return m_blasBuffer->GetGPUVirtualAddress();
}

const D3D12_GPU_DESCRIPTOR_HANDLE StaticMesh::GetVertexAndIndexBufferSRVHandle() const
{
	return m_meshSRVHandle;
}

StaticMeshEntity::StaticMeshEntity(
	std::string&& name, 
	const uint64_t meshIndex, 
	const DirectX::XMFLOAT4X4& localToWorld) 
	:
	m_name(name),
	m_meshIndex(meshIndex), 
	m_localToWorld(localToWorld)
{
}

void StaticMeshEntity::FillConstants(ObjectConstants* objConst) const
{
	objConst->vertexStride = sizeof(StaticMesh::VertexType);
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