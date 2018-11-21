#pragma once

#include "Common.h"
#include "UploadBuffer.h"
#include "ResourceHeap.h"

__declspec(align(256)) struct ObjectConstants
{
	DirectX::XMFLOAT4X4 localToWorldMatrix;
};

class StaticMesh
{
public:
	using VertexType = VertexFormat::P3N3T3B3U2;
	using IndexType = uint32_t;

	StaticMesh() = default;
	void Init(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap, std::vector<VertexType> vertexData, std::vector<IndexType> indexData, uint32_t matIndex, ID3D12DescriptorHeap* srvHeap, const size_t offsetInHeap, const size_t srvDescriptorSize);
	void Render(ID3D12GraphicsCommandList4* cmdList);

	uint32_t GetMaterialIndex() const;
	VertexFormat::Type GetVertexFormat() const;
	const DirectX::BoundingBox& GetBounds() const;
	const D3D12_GPU_VIRTUAL_ADDRESS GetBLASAddress() const;

private:
	void CreateVertexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<VertexType>& vertexData, ID3D12DescriptorHeap* srvHeap, const size_t offsetInHeap, const size_t srvDescriptorSize);
	void CreateIndexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<IndexType>& indexData, ID3D12DescriptorHeap* srvHeap, const size_t offsetInHeap, const size_t srvDescriptorSize);
	void CreateBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap, const size_t numVerts, const size_t numIndices);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_blasBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	D3D12_GPU_DESCRIPTOR_HANDLE m_vertexBufferSrv;
	D3D12_GPU_DESCRIPTOR_HANDLE m_indexBufferSrv;
	uint32_t m_materialIndex;
	uint32_t m_numIndices;
};

class StaticMeshEntity
{
public:
	StaticMeshEntity() = delete;
	StaticMeshEntity(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap, , ID3D12DescriptorHeap* srvHeap, const size_t offsetInHeap, const size_t srvDescriptorSize, const D3D12_GPU_VIRTUAL_ADDRESS blasGpuAddr, std::string&& name, const uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld);

	void FillConstants(ObjectConstants* objConst) const;
	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
	uint64_t GetMeshIndex() const;
	std::string GetName() const;

private:
	void CreateTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap, const D3D12_GPU_VIRTUAL_ADDRESS blasGpuAddr, const DirectX::XMFLOAT4X4& localToWorld);

private:
	std::string m_name;
	uint64_t m_meshIndex;
	DirectX::XMFLOAT4X4 m_localToWorld;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_tlasBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_tlasSrv;
};
