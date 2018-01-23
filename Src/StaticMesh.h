#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>

class StaticMesh
{
public:
	struct VertexType
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;

		VertexType() {}
		VertexType(DirectX::XMFLOAT3 inPos, DirectX::XMFLOAT3 inNormal) :
			position(inPos), normal(inNormal) {}

		struct InputLayout
		{
			static const uint32_t s_num = 2;
			static D3D12_INPUT_ELEMENT_DESC s_desc[s_num];
		};
	};

	using IndexType = uint16_t;

	StaticMesh();
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<StaticMesh::VertexType> vertexData, std::vector<StaticMesh::IndexType> indexData);
	void Render(ID3D12GraphicsCommandList* cmdList);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_numIndices;
};

class StaticMeshEntity
{
public:
	StaticMeshEntity() = delete;
	StaticMeshEntity(const uint64_t meshIndex, const DirectX::XMFLOAT4X4& localToWorld);
	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix() const;
	uint64_t GetMeshIndex() const;
private:
	uint64_t m_meshIndex;
	DirectX::XMFLOAT4X4 m_localToWorld;
};
