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

		struct InputLayout
		{
			static const uint32_t s_num = 2;
			static D3D12_INPUT_ELEMENT_DESC s_desc[s_num];
		};
	};

	using keep_alive_type = std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, Microsoft::WRL::ComPtr<ID3D12Resource>>;

	StaticMesh();
	DirectX::XMFLOAT4X4 GetLocalToWorldMatrix();
	keep_alive_type Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Render(ID3D12GraphicsCommandList* cmdList);

private:
	DirectX::XMFLOAT4X4 m_localToWorld;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_numIndices;
};
