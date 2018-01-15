#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <array>
#include <vector>
#include <memory>
#include "StaticMesh.h"

constexpr size_t k_gfxBufferCount = 2;

class Scene
{
	__declspec(align(256)) struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 localToWorldMatrix;
	};

public:
	std::vector<StaticMesh::keep_alive_type> Init(const uint32_t cbvRootParamIndex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Update(uint32_t bufferIndex);
	void Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex);

private:
	std::vector<std::unique_ptr<StaticMesh>> m_meshes;

	uint32_t m_objectCBVRootParameterIndex;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_objectConstantBuffers;
	std::array<ObjectConstants*, k_gfxBufferCount> m_objectConstantBufferPtr;
};
