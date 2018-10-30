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
	void Init(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, std::vector<VertexType> vertexData, std::vector<IndexType> indexData, uint32_t matIndex);
	void Render(ID3D12GraphicsCommandList* cmdList);

	uint32_t GetMaterialIndex() const;
	VertexFormat::Type GetVertexFormat() const;
	const DirectX::BoundingBox& GetBounds() const;

private:
	void CreateVertexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<VertexType>& vertexData);
	void CreateIndexBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<IndexType>& indexData);
	void CreateBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* resourceHeap, const std::vector<VertexType>& vertexData);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_materialIndex;
	uint32_t m_numIndices;
};

class StaticMeshEntity
{
public:
	StaticMeshEntity() = delete;
	StaticMeshEntity(std::string&& name, const uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld);

	void FillConstants(ObjectConstants* objConst) const;
	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
	uint64_t GetMeshIndex() const;
	std::string GetName() const;

private:
	std::string m_name;
	uint64_t m_meshIndex;
	DirectX::XMFLOAT4X4 m_localToWorld;
};
