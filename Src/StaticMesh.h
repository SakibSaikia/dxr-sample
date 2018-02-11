#pragma once

class StaticMesh
{
public:
	struct VertexType
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;

		VertexType() = default;
		VertexType(DirectX::XMFLOAT3 inPos, DirectX::XMFLOAT3 inNormal, DirectX::XMFLOAT2 inUV) :
			position(inPos), normal(inNormal), uv(inUV) {}

		struct InputLayout
		{
			static const uint32_t s_num = 3;
			static D3D12_INPUT_ELEMENT_DESC s_desc[s_num];
		};
	};

	using IndexType = uint16_t;

	StaticMesh() = default;
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<StaticMesh::VertexType> vertexData, std::vector<StaticMesh::IndexType> indexData, uint32_t matIndex);
	void Render(ID3D12GraphicsCommandList* cmdList);

	const uint32_t GetMaterialIndex() const;

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
	StaticMeshEntity(uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld);

	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
	uint64_t GetMeshIndex() const;

	static uint32_t GetObjectConstantsRootParamIndex();
private:
	uint64_t m_meshIndex;
	DirectX::XMFLOAT4X4 m_localToWorld;
};
