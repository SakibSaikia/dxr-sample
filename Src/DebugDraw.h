#pragma once

class DebugLineMesh
{
public:
	struct VertexType
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 color;

		VertexType() = default;
		VertexType(DirectX::XMFLOAT3 inPos, DirectX::XMFLOAT3 inColor) :
			position(inPos), color(inColor) {}

		struct InputLayout
		{
			static const uint32_t s_num = 2;
			static D3D12_INPUT_ELEMENT_DESC s_desc[s_num];
		};
	};

	using IndexType = uint16_t;

	DebugLineMesh() = default;
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::vector<DirectX::BoundingBox>& boundingBoxes, const DirectX::XMFLOAT3 color);
	void Render(ID3D12GraphicsCommandList* cmdList);


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_numIndices;
};