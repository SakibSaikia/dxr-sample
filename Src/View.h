#pragma once

#include "Camera.h"

__declspec(align(256))
struct ViewConstants
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 viewProjectionMatrix;
};

class View
{
public:
	~View();

	void Init(ID3D12Device* device, size_t bufferCount, size_t width, size_t height);
	void Update(float dt, POINT mouseDelta);
	void UpdateRenderResources(uint32_t bufferIndex);

	ID3D12Resource* GetConstantBuffer(uint32_t index) const;

private:
	Camera m_camera;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_cbuffers;
	std::vector<ViewConstants*> m_cbufferPtrs;
};
