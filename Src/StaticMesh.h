#pragma once

#include "Common.h"

class StaticMesh
{
public:
	using VertexType = VertexFormat::P3N3T3B3U2;
	using IndexType = uint16_t;

	StaticMesh() = default;
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<VertexType> vertexData, std::vector<IndexType> indexData, uint32_t matIndex);
	void Render(ID3D12GraphicsCommandList* cmdList);

	uint32_t GetMaterialIndex() const;
	VertexFormat::Type GetVertexFormat() const;
	const DirectX::BoundingBox& GetBounds() const;


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_materialIndex;
	uint32_t m_numIndices;
	DirectX::BoundingBox m_bounds;
};

class StaticMeshEntity
{
public:
	StaticMeshEntity() = delete;
	StaticMeshEntity(uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld);

	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
	uint64_t GetMeshIndex() const;

	static uint32_t GetObjectConstantsRootParamIndex();
private:
	uint64_t m_meshIndex;
	DirectX::XMFLOAT4X4 m_localToWorld;
};
