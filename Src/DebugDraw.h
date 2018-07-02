#pragma once

#include "Common.h"

class DebugLineMesh
{
public:
	using VertexType = VertexFormat::P3C3;
	using IndexType = uint16_t;

	DebugLineMesh() = default;
	void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void AddBox(const DirectX::BoundingBox& box, const DirectX::XMFLOAT3 color);
	void AddTransformedBox(const DirectX::BoundingBox& box, const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT3 color);
	void AddAxes(const DirectX::XMMATRIX& transform, float scale);
	void Render(ID3D12GraphicsCommandList* cmdList, const uint32_t bufferIndex);
	void UpdateRenderResources(uint32_t bufferIndex);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	VertexType* m_vbStartPtr, *m_vbCurrentPtr;
	IndexType* m_ibStartPtr, *m_ibCurrentPtr;
	uint32_t m_numVerts;
	uint32_t m_numIndices;
};