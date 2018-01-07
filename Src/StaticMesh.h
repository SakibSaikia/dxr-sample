#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>

class StaticMesh
{
public:
	using keep_alive_type = std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, Microsoft::WRL::ComPtr<ID3D12Resource>>;

	StaticMesh();
	keep_alive_type Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Render(ID3D12GraphicsCommandList* cmdList);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t m_numIndices;
};
